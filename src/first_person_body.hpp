#pragma once
// FPB validates local CTerrorPlayer identity then opens the verified native
// DrawModel branch. There is no observational ShouldDraw[3] hook.
#include <atomic>
#include <memory>
#include <cstdint>
#include "first_person_body_config.hpp"
#include "free_camera_body_policy.hpp"
#include "first_person_native_gate.hpp"
#include "first_person_native_visibility.hpp"
#include "view_compat_runtime.hpp"
#include "local_player.hpp"
#include "survivors.hpp"
namespace visual::firstperson {
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::atomic<const char*> status{u8"关闭：原生可见性保持不变"};
inline std::atomic<std::uint32_t> currentHandle{0xffffffffu},currentEpoch{0},localBase{0};
inline std::atomic<std::uint64_t> localTick{0};
inline std::atomic<bool> worldBodyActive{false},validatedLocal{false};
inline freecamerabody::Transition cameraTransition;
// Only the game-thread class lookup publishes offsets. Draw-time code uses
// the identity/serial-validated entity and safe reader, never the class map.
inline std::atomic<int> flagsOffset{-1},originOffset{-1},velocityOffset{-1},probeShoulder{-1};
inline std::atomic<unsigned> cosmeticGeneration{0};
inline void clear() noexcept{
 worldBodyActive=false;validatedLocal=false;currentHandle=0xffffffffu;currentEpoch=0;localBase=0;localTick=0;flagsOffset=-1;originOffset=-1;velocityOffset=-1;probeShoulder=-1;
}
inline void onFrame(const dango::GameCameraFrame& camera,bool freeCameraRequested=false,bool forceWorldBody=false) noexcept {
 // Reset the first-/third-person evidence on BOTH F6 edges. The detached
 // camera's distance is not a valid hint after returning to normal view.
 if(cameraTransition.update(freeCameraRequested)){viewcompat::reset();++cosmeticGeneration;}
 struct RestoreOnFailure {bool succeeded=false;~RestoreOnFailure(){if(!succeeded)native::disable();}} gate;
 if(!std::atomic_load(&options)->enabled&&!forceWorldBody){
  clear();status=u8"关闭：原生可见性保持不变";return;
 }
 if(!camera.active||!source_camera::hooked){
  clear();status=u8"等待有效游戏相机";return;
 }
 auto frame=std::atomic_load(&source_entities::localSnapshot);
 if(!source_entities::bound||source_entities::faulted||frame->epoch!=camera.epoch||
    camera.tick<frame->tick||camera.tick-frame->tick>250){
  clear();status=u8"实体快照未验证或过期";return;
 }
 auto handle=visual::localplayer::handleAtIndex(*frame,int(source_entities::localRequestedIndex));
 if(handle==0xffffffffu){
  clear();status=u8"本机身份/引擎 ABI 未通过";return;
 }
 const source_entities::Record* actor=nullptr;
 for(const auto& r:frame->records)if(r.handle==handle){actor=&r;break;}
 if(!actor||actor->className!="CTerrorPlayer"||!actor->team||*actor->team!=2||
    !actor->health||*actor->health<=0||!actor->lifeState||*actor->lifeState!=0||
    !actor->modelIndex||*actor->modelIndex<=0||
    (actor->dormant&&*actor->dormant)){
  clear();status=u8"非正常存活本机求生者：不修改原生绘制";return;
 }
 auto index=handle&4095u;
 auto object=std::uint32_t(std::uintptr_t(source_entities::list));
 auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry entry{};
 if(!source_entities::wire::load(source_entities::reader,entries+index*sizeof(entry),entry)||
    !entry.entity||entry.serial!=(handle>>12)){
  clear();status=u8"实体 handle/serial 未通过";return;
 }
 // FreeCamera moves only the camera. The verified local player remains a
 // real networked CTerrorPlayer at its original position. Do not classify
 // detached camera distance as third-person view.
 const auto cfg=std::atomic_load(&options);
 flagsOffset=-1;originOffset=-1;velocityOffset=-1;
 for(const auto& pair:source_entities::classes)if(pair.second.name=="CTerrorPlayer"&&pair.second.schema){
  const auto& fields=pair.second.fields;
  auto flags=fields.find("m_fFlags"),origin=fields.find("m_vecOrigin");
  auto velocity=fields.find("m_vecVelocity[0]");
  if(velocity==fields.end()||velocity->second.type!=2)velocity=fields.find("m_vecVelocity");
  if(velocity!=fields.end()&&velocity->second.type==2&&velocity->second.offset<=65524u)velocityOffset=int(velocity->second.offset);
  if(flags!=fields.end()&&flags->second.type==0&&flags->second.offset<=65532u)flagsOffset=int(flags->second.offset);
  if(origin!=fields.end()&&origin->second.type==2&&origin->second.offset<=65524u)originOffset=int(origin->second.offset);
  break;
 }
 const auto actualShoulder=viewcompat::readShoulder();probeShoulder=actualShoulder?(*actualShoulder?1:0):-1;
 const auto shoulder=(freeCameraRequested&&cfg->thirdPersonCompatibility)?
    viewcompat::readShoulder():std::optional<bool>{};
 const auto bodyMode=freecamerabody::choose(freeCameraRequested,
                                            cfg->thirdPersonCompatibility,shoulder);
 if(bodyMode!=freecamerabody::Mode::NormalView){
  viewcompat::shoulderValue=shoulder?(*shoulder?1:0):-1;
  viewcompat::horizontalUnits=-1;viewcompat::eyeHeightUnits=-1;
  if(bodyMode==freecamerabody::Mode::NativeThirdPerson){
   // An actual native third-person camera already controls local player
   // visibility. Do not force an extra first-person DrawModel branch.
   viewcompat::status="FreeCamera: native third-person drawing preserved";
   clear();status=u8"FreeCamera：原生第三人称绘制";return;
  }
  viewcompat::status="FreeCamera: world-anchored local player, no camera distance test";
 }else if(cfg->thirdPersonCompatibility){
  // Normal game view retains all existing compatibility behavior.
  auto origin=viewcompat::freshLocalOrigin(entry.entity);
  if(!origin && frame->tick+75>=camera.tick)origin=actor->networkOrigin;
  const auto viewed=viewcompat::detectAndCommit(camera,origin);
  viewcompat::status=viewed==viewcompat::Mode::First?"第一人称：身体显示已启用":
      viewed==viewcompat::Mode::Third?"第三人称：恢复原生绘制":
      "视角状态不明确：等待确认";
 }else if(viewcompat::mode.load(std::memory_order_acquire)!=viewcompat::Mode::Unknown){
  viewcompat::reset();viewcompat::status="视角兼容已关闭";
 }
  currentHandle=handle;currentEpoch=camera.epoch;localBase=entry.entity;localTick=camera.tick;
 validatedLocal=true;
 // The one-byte patch previously stayed active through third-person views,
 // even when the Studio cosmetic gate refused to alter the bones. That
 // exposed local model draws which should remain owned by native rendering.
 // Do this only on the post-render game callback, never from a Studio draw.
 if(!visibility::allowNativeGate(bodyMode,cfg->thirdPersonCompatibility,
       viewcompat::mode.load(std::memory_order_acquire),probeShoulder.load())){
  worldBodyActive=false;
  if(native::disable()){
   gate.succeeded=true;
   status=cfg->thirdPersonCompatibility?
     u8"视角未确认为第一人称：原生 DrawModel 门控已恢复":
     u8"明确第三人称：原生 DrawModel 门控已恢复";
  }else status=native::state.load();
  return;
 }
 if(!native::enable(entry.entity)){worldBodyActive=false;status=native::state.load();return;}
 worldBodyActive=bodyMode==freecamerabody::Mode::WorldAnchored;
 gate.succeeded=true;
 status=worldBodyActive.load()?u8"FreeCamera：本机模型保留世界原位（原生骨骼）":
                              u8"本机原生 DrawModel 分支已放行";
}
inline void stop() noexcept {
 const bool restored=native::disable();clear();viewcompat::reset();cameraTransition.reset();
  status=restored&&native::restoredVerified.load()?
     u8"插件退出：原生 DrawModel 字节已恢复":
     u8"插件退出：DrawModel 恢复未确认，请完全退出游戏";
}
inline bool isDrawing() noexcept {return false;}
}
