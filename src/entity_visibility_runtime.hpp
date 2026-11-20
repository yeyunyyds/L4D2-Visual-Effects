#pragma once
#include "entity_occlusion.hpp"
#include "special_glow_settings.hpp"
#include "tank_runtime.hpp"
#include "source_trace.hpp"
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <atomic>
namespace visual::visibility {
struct Sample {std::uint64_t tick=0;bool visible=false;};
struct Snapshot {std::uint32_t epoch=0;std::map<std::uint32_t,Sample> samples;};
inline std::shared_ptr<const Snapshot> published=std::make_shared<const Snapshot>();
inline scenetrace::Binding trace;
inline std::uint32_t epoch=0;
inline std::uint64_t polled=0;
inline std::size_t cursor=0;
inline std::map<std::uint32_t,Sample> cached;
inline std::atomic<const char*> status{"disabled"};
inline bool requested(){
 auto tank=std::atomic_load(&tank::options);
 auto visual=std::atomic_load(&tank::visualSettings);
 auto specials=std::atomic_load(&specialglow::options);
 return (tank->enabled&&tank->draw&&tank->occludeHud&&(visual->showBar||visual->showDirection||visual->showRange))||specials->usesVisibility();
}
inline void reset(std::uint32_t next=0){
 epoch=next;polled=0;cursor=0;trace.reset();cached.clear();
 std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{next,{}}));
}
inline bool visible(std::uint32_t map,std::uint32_t handle,std::uint64_t now){
 auto view=std::atomic_load(&published);
 if(!view||view->epoch!=map)return false;
 auto it=view->samples.find(handle);
 return it!=view->samples.end()&&now>=it->second.tick&&now-it->second.tick<=350&&it->second.visible;
}
// This is called only by the Source camera frame callback. D3D Present reads
// the published immutable results and never calls engine TraceRay.
inline void onFrame(const dango::GameCameraFrame& camera)try{
 if(!requested()){if(epoch||!cached.empty())reset();status="disabled";return;}
 if(camera.epoch!=epoch)reset(camera.epoch);
 if(!camera.active){cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));status="camera inactive";return;}
 if(camera.tick<polled||camera.tick-polled<50)return;
 polled=camera.tick;
 auto inventory=std::atomic_load(&source_entities::snapshot);
 if(source_entities::faulted||!inventory||inventory->epoch!=camera.epoch||
    camera.tick<inventory->tick||camera.tick-inventory->tick>350){
  cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));status="entity snapshot stale";return;
 }
 dango::CameraPose pose;
 if(!dango::cameraPose(camera,pose)){cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));status="camera pose unavailable";return;}
 auto special=std::atomic_load(&specialglow::options);
 auto cfg=std::atomic_load(&tank::options);
 auto style=std::atomic_load(&tank::visualSettings);
 bool tankHud=cfg->enabled&&cfg->draw&&cfg->occludeHud&&(style->showBar||style->showDirection||style->showRange);
 std::vector<const dango::inventory::Record*> wanted;std::set<std::uint32_t> handles;
 for(const auto& r:inventory->records){
  int role=specialCharacter(r);if(role<0||!specialglow::eligible(r))continue;
  if(!((tankHud&&role==6)||(special->types[unsigned(role)].enabled&&special->types[unsigned(role)].occlude)))continue;
  wanted.push_back(&r);handles.insert(r.handle);
  if(wanted.size()>=64)break;
 }
 for(auto it=cached.begin();it!=cached.end();)if(!handles.count(it->first))it=cached.erase(it);else ++it;
 if(wanted.empty()){std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));status="no sampled entities";return;}
 if(!trace.object&&!trace.faulted&&!scenetrace::connectBinding(trace))trace.faulted=true;
 if(trace.faulted){cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));status="TraceRay binding refused";return;}
 // Round-robin at most 12 body candidates / 50ms. Each uses up to 3 rays.
 unsigned budget=12;
 for(std::size_t offset=0;offset<wanted.size()&&budget;++offset){
  const auto& record=*wanted[(cursor+offset)%wanted.size()];
  auto old=cached.find(record.handle);
  if(old!=cached.end()&&camera.tick>=old->second.tick&&camera.tick-old->second.tick<90)continue;
  auto result=occlusion::bodyVisible(pose.eye,record,[](dango::Vec3 from,dango::Vec3 to,dango::trace::Result& out){
   return scenetrace::castBinding(trace,from,to,out);
  },specialCharacter(record)==6);
  cached[record.handle]={camera.tick,result.value_or(false)};
  --budget;
 }
 cursor=(cursor+std::max<std::size_t>(1,budget?wanted.size():12))%wanted.size();
 std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,cached}));
 status=trace.faulted?"TraceRay faulted":"world geometry visibility active";
}catch(...){cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));trace.faulted=true;status="visibility exception; fail closed";}
} // namespace visual::visibility
