#pragma once
// FreeCamera viewmodel-only render filter; no ConVar, sv_cheats, player entity
// mutation, server.dll operations or extra model draws.
#include "free_camera_runtime.hpp"
#include "free_camera_viewmodel_policy.hpp"
#include "local_player.hpp"
#include "dango/source_entities.hpp"
#include <atomic>
#include <cstdint>
#include <memory>

namespace visual::freecamera::viewmodel {
struct Target {
 std::uint32_t epoch=0,handle=0,serial=0,index=0,base=0,renderable=0;
 std::uint64_t tick=0;
};
inline std::shared_ptr<const Target> target=std::make_shared<const Target>();
inline std::atomic<unsigned long> hiddenDraws{0},targetAccepts{0},targetMisses{0};
inline std::atomic<const char*> status{"Waiting for local viewmodel"};
inline void clear(){std::atomic_store(&target,std::make_shared<const Target>());}
inline void refresh(const dango::GameCameraFrame& camera){
 if(!camera.active||!requested.load(std::memory_order_acquire)||
    !hideViewmodel.load(std::memory_order_acquire)){
  clear();status="Viewmodel filter inactive";return;
 }
 const auto f=std::atomic_load(&source_entities::snapshot);
 if(!source_entities::bound||source_entities::faulted||!source_entities::list||
    f->epoch!=camera.epoch||camera.tick<f->tick||camera.tick-f->tick>350){
  clear();status="Waiting for verified entity inventory";return;
 }
 const auto player=visual::localplayer::handle(*f);
 if(player==0xffffffffu){
  clear();status="Local player handle unavailable";return;
 }
 std::uint32_t vm=0xffffffffu;
 for(const auto& record:f->records)
  if(record.handle==player){vm=record.viewModel.value_or(0xffffffffu);break;}
 for(const auto& r:f->records){
  const auto owner=r.weaponOwner.value_or(r.owner.value_or(0xffffffffu));
  if(!policy::ownedByLocal(player,vm,r.handle,owner,r.className))continue;
  if(r.index>=source_entities::profile::MaxNetworked||
     r.serial!=(r.handle>>12))continue;
  const auto list=std::uint32_t(std::uintptr_t(source_entities::list));
  const auto entries=std::uint32_t(std::int64_t(list)+source_entities::profile::EntryBaseDelta);
  source_entities::wire::Entry e{};
  if(!source_entities::wire::load(source_entities::reader,
        entries+r.index*sizeof(e),e)||!e.entity||e.serial!=r.serial)continue;
  // Verified Source client entity renderable interface (entity+4).
  const auto renderable=e.entity+4u;
  if(renderable<e.entity||
     !source_camera::readable(reinterpret_cast<const void*>(std::uintptr_t(renderable)),4))continue;
  std::atomic_store(&target,std::make_shared<const Target>(
      Target{camera.epoch,r.handle,r.serial,r.index,e.entity,renderable,f->tick}));
  ++targetAccepts;
  status="Verified local CBaseViewModel renderable";
  return;
 }
 ++targetMisses;clear();status="Local viewmodel not found; drawing retained";
}
inline bool shouldHide(std::uint32_t renderable){
 if(!renderable)return false;
 const auto accepted=std::atomic_load(&target);
 // No entity reads on ordinary world draws.
 if(!accepted||accepted->renderable!=renderable||!enabled()||
    !hideViewmodel.load(std::memory_order_acquire))return false;
 const auto now=GetTickCount64();
 const bool fresh=now>=accepted->tick&&now-accepted->tick<=350;
 const bool sameMap=source_camera::epoch==accepted->epoch;
 if(!fresh||!sameMap||!source_entities::list)return false;
 const auto list=std::uint32_t(std::uintptr_t(source_entities::list));
 const auto entries=std::uint32_t(std::int64_t(list)+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry e{};
 const bool valid=source_entities::wire::load(source_entities::reader,
   entries+accepted->index*sizeof(e),e)&&
   e.entity==accepted->base&&e.serial==accepted->serial;
 const bool skip=policy::suppress(true,true,accepted->renderable,
                                  renderable,sameMap,fresh,valid);
 if(skip)++hiddenDraws;
 return skip;
}
}
