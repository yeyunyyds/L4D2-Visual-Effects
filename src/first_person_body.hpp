#pragma once
#include <atomic>
#include <memory>
#include <cstdint>
#include "first_person_body_config.hpp"
#include "local_player.hpp"
#include "survivors.hpp"

namespace visual::firstperson {
// This experiment contains NO new visibility hook. Existing DrawModelExecute
// and StudioRender callbacks occur too late to restore a rejected local player.
// A real hook requires an independently verified L4D2 client.dll call site,
// signature, x86 ABI, and a chain-safe installation/removal protocol.
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::atomic<const char*> status{u8"关闭：原生可见性保持不变"};
inline std::atomic<std::uint32_t> currentHandle{0xffffffffu},currentEpoch{0};
inline std::atomic<bool> validatedLocal{false},nativeVisibilityHook{false};
inline void clear() noexcept {
 currentHandle=0xffffffffu;currentEpoch=0;validatedLocal=false;
 nativeVisibilityHook=false;
}
inline void onFrame(const dango::GameCameraFrame& camera) noexcept {
 clear();
 if(!std::atomic_load(&options)->enabled) {
  status=u8"关闭：原生可见性保持不变";return;
 }
 if(!camera.active||!source_camera::hooked) {
  status=u8"等待游戏内有效相机；未安装可见性 Hook";return;
 }
 const auto frame=std::atomic_load(&source_entities::snapshot);
 if(!source_entities::bound||source_entities::faulted||
    frame->epoch!=camera.epoch||camera.tick<frame->tick||
    camera.tick-frame->tick>250) {
  status=u8"实体快照无效或过期；未安装可见性 Hook";return;
 }
 const auto handle=visual::localplayer::handle(*frame);
 if(handle==0xffffffffu) {
  status=u8"本机身份或引擎 ABI 验证失败；未安装可见性 Hook";return;
 }
 for(const auto& r:frame->records) {
  if(r.handle!=handle)continue;
  if(!visual::survivorPlayerClass(r.className)||!r.team||*r.team!=2||
     !r.health||*r.health<=0||!r.lifeState||*r.lifeState!=0||
     !r.modelIndex||*r.modelIndex<=0||(r.dormant&&*r.dormant)) {
   status=u8"本机角色不是可验证的存活求生者；未安装可见性 Hook";return;
  }
  // localplayer::handle already checked entry, serial and full handle.
  currentHandle=handle;currentEpoch=camera.epoch;validatedLocal=true;
  status=u8"已识别本机求生者；缺少经验证的 ShouldDraw ABI，安全拒绝 Hook";
  return;
 }
 status=u8"本机实体不在已验证目录；未安装可见性 Hook";
}
inline void stop() noexcept {
 clear();
 status=u8"插件退出；原生可见性未修改";
}
inline bool isDrawing() noexcept {return false;}
} // namespace visual::firstperson
