#pragma once
#include "dango/source_entities.hpp"
#include "rock_settings.hpp"
#include "rock_landing.hpp"
#include "image_trace.hpp"
namespace visual::rockruntime {
inline std::shared_ptr<const rocks::Settings> options=std::make_shared<const rocks::Settings>();
inline std::atomic<bool> faulted{false};
struct Snapshot {uint32_t epoch=0;uint64_t tick=0;std::map<uint32_t,rocks::Track> tracks;std::map<uint32_t,rocks::Landing> landings;};
inline std::shared_ptr<const Snapshot> emptySnapshot=std::make_shared<const Snapshot>();
inline std::shared_ptr<const Snapshot> snapshot=emptySnapshot;
inline rocks::Tracker tracker;
inline picturetrace::Binding mapTrace;
inline std::shared_ptr<const source_entities::Frame> inventory;
inline std::map<uint32_t,rocks::Landing> landings;
inline uint32_t epoch=0;inline uint64_t lastTick=0,nextLanding=0;inline size_t cursor=0;inline bool running=false;
inline bool active(){auto cfg=std::atomic_load(&options);return cfg->enabled&&!faulted&&(cfg->trajectory||cfg->landingCircle);}
inline void reset(uint32_t map){tracker.reset(map);inventory.reset();landings.clear();mapTrace.reset();lastTick=nextLanding=0;cursor=0;epoch=map;std::atomic_store(&snapshot,emptySnapshot);}
inline std::optional<source_entities::Record> directed(uint32_t handle,bool& gone){
 using namespace source_entities;
 if(!bound||source_entities::faulted||!list)return {};
 unsigned index=handle&4095u;if(index>=source_entities::profile::MaxNetworked)return {};
 uint32_t object=uint32_t(reinterpret_cast<uintptr_t>(list));auto entries=uint32_t(int64_t(object)+source_entities::profile::EntryBaseDelta);
 wire::Entry before{};uint32_t net=0;if(!wire::load(reader,entries+index*sizeof(before),before))return {};
 if(!before.entity||before.serial!=(handle>>12)){gone=true;return {};}
 if(!wire::load(reader,object+source_entities::profile::CacheOffset+index*8,net)||!net)return {};
 Frame temporary;auto record=readRecord(index,before,net,temporary);wire::Entry after{};
 if(!record||!wire::load(reader,entries+index*sizeof(after),after)||after.entity!=before.entity||after.serial!=before.serial)return {};
 return record;
}
inline void updateLandings(uint64_t now){
 for(auto it=landings.begin();it!=landings.end();){auto t=tracker.tracks().find(it->first);if(t==tracker.tracks().end()||t->second.trajectory.empty()||now<it->second.computedMs||now-it->second.computedMs>rocks::SampleLifetimeMs)it=landings.erase(it);else ++it;}
 if(now<nextLanding)return;nextLanding=now+50;
 std::vector<const rocks::Track*> flying;for(const auto& item:tracker.tracks())if(item.second.motion&&!item.second.trajectory.empty())flying.push_back(&item.second);
 if(flying.empty())return;
 if(!mapTrace.object&&!mapTrace.faulted&&!picturetrace::connectBinding(mapTrace))mapTrace.faulted=true;
 if(mapTrace.faulted){landings.clear();return;}
 const auto& t=*flying[cursor++%flying.size()];unsigned budget=64;
 auto hit=rocks::predictLanding(*t.motion,rocks::Gravity,now,budget,[](auto a,auto b,auto& out){return picturetrace::castBoxBinding(mapTrace,a,b,rocks::HalfSize,out);},rocks::HalfSize);
 if(hit.valid())landings[t.record.handle]=hit;else landings.erase(t.record.handle);
}
inline void onFrame(const dango::GameCameraFrame& camera)noexcept{
 try{
  auto cfg=std::atomic_load(&options);
  if(!cfg->enabled||!camera.active||(!cfg->trajectory&&!cfg->landingCircle)){if(running)reset(camera.epoch);running=false;if(!cfg->enabled)faulted=false;return;}
  if(faulted)return;
  if(!running||epoch!=camera.epoch){reset(camera.epoch);running=true;}
  if(camera.tick==lastTick)return;lastTick=camera.tick;
  if(source_entities::faulted){faulted=true;reset(camera.epoch);return;}
  auto latest=std::atomic_load(&source_entities::snapshot);
  if(latest!=inventory){inventory=latest;if(latest->epoch==camera.epoch&&camera.tick>=latest->tick&&camera.tick-latest->tick<=500)tracker.discover(*latest);}
  std::vector<uint32_t> handles;for(const auto& item:tracker.tracks())if(item.second.state!=rocks::State::Destroyed)handles.push_back(item.first);
  for(auto handle:handles){bool gone=false;auto record=directed(handle,gone);if(record)tracker.observe(*record,camera.tick,cfg->horizonMs);else tracker.unavailable(handle,gone);}
  tracker.expire(camera.tick);updateLandings(camera.tick);
  auto frame=std::make_shared<Snapshot>();frame->epoch=camera.epoch;frame->tick=camera.tick;frame->tracks=tracker.tracks();frame->landings=landings;std::atomic_store(&snapshot,std::shared_ptr<const Snapshot>(std::move(frame)));
 }catch(...){faulted=true;reset(camera.epoch);}
}
}
