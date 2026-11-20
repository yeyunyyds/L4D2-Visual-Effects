#pragma once
// World-only LOS for the Tank HUD. No native model, Glow writes, or new hooks.
#include "entity_occlusion.hpp"
#include "tank_runtime.hpp"
#include "source_trace.hpp"
#include <map>
#include <memory>
#include <atomic>
namespace visual::visibility {
struct Sample{std::uint64_t tick=0;bool visible=false;};
struct Snapshot{std::uint32_t epoch=0;std::map<std::uint32_t,Sample> samples;};
inline std::shared_ptr<const Snapshot> published=std::make_shared<const Snapshot>();
inline scenetrace::Binding trace;
inline std::uint32_t epoch=0;
inline std::uint64_t polled=0;
inline std::map<std::uint32_t,Sample> cached;
inline std::atomic<const char*> status{"disabled"};
inline bool requested(){
 auto cfg=std::atomic_load(&tank::options);
 if(!cfg->enabled||!cfg->draw||!cfg->occludeHud)return false;
 auto visual=std::atomic_load(&tank::visualSettings);
 return visual->showBar||visual->showDirection||visual->showRange;
}
inline void reset(std::uint32_t next=0){
 epoch=next;polled=0;trace.reset();cached.clear();
 std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{next,{}}));
}
inline bool visible(std::uint32_t map,std::uint32_t handle,std::uint64_t now){
 auto snap=std::atomic_load(&published);
 if(!snap||snap->epoch!=map)return false;
 auto hit=snap->samples.find(handle);
 return hit!=snap->samples.end()&&now>=hit->second.tick&&now-hit->second.tick<=350&&hit->second.visible;
}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 if(!requested()){if(epoch||!cached.empty())reset();status="disabled";return;}
 if(camera.epoch!=epoch)reset(camera.epoch);
 if(!camera.active){
  cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
  status="camera inactive";return;
 }
 if(camera.tick<polled||camera.tick-polled<50)return;
 polled=camera.tick;
 auto inventory=std::atomic_load(&source_entities::snapshot);
 if(source_entities::faulted||!inventory||inventory->epoch!=camera.epoch||
    camera.tick<inventory->tick||camera.tick-inventory->tick>350){
  cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
  status="Tank snapshot stale";return;
 }
 dango::CameraPose pose;
 if(!dango::cameraPose(camera,pose)){
  cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
  status="camera pose invalid";return;
 }
 std::map<std::uint32_t,const dango::inventory::Record*> tanks;
 for(const auto& record:inventory->records){
  if(!tank::tank(record)||record.handle==0xffffffffu||
     (record.handle&4095u)!=record.index||record.handle>>12!=record.serial)continue;
  tanks.emplace(record.handle,&record);
  if(tanks.size()>=8)break;
 }
 for(auto it=cached.begin();it!=cached.end();)if(!tanks.count(it->first))it=cached.erase(it);else ++it;
 if(tanks.empty()){
  std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
  status="no Tank";return;
 }
 if(!trace.object&&!trace.faulted&&!scenetrace::connectBinding(trace))trace.faulted=true;
 if(trace.faulted){
  cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
  status="TraceRay unavailable";return;
 }
 for(const auto& item:tanks){
  auto old=cached.find(item.first);
  if(old!=cached.end()&&camera.tick>=old->second.tick&&camera.tick-old->second.tick<90)continue;
  auto sample=occlusion::bodyVisible(pose.eye,*item.second,[](dango::Vec3 from,dango::Vec3 to,dango::trace::Result& out){
   return scenetrace::castBinding(trace,from,to,out);
  },true);
  cached[item.first]={camera.tick,sample.value_or(false)};
 }
 std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,cached}));
 status="Tank world-only visibility active";
}catch(...){
 cached.clear();std::atomic_store(&published,std::make_shared<const Snapshot>(Snapshot{epoch,{}}));
 trace.faulted=true;status="Tank visibility error; fail closed";
}
} // namespace visual::visibility
