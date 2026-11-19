#pragma once
// Strictly observational hook: preserve native ShouldDraw return on every path.
#include <atomic>
#include <memory>
#include <cstdint>
#include <cstring>
#include "first_person_body_config.hpp"
#include "first_person_native_gate.hpp"
#include "local_player.hpp"
#include "survivors.hpp"
namespace visual::firstperson {
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::atomic<const char*> status{u8"关闭：原生可见性保持不变"};
inline std::atomic<std::uint32_t> currentHandle{0xffffffffu},currentEpoch{0},localBase{0};
inline std::atomic<std::uint64_t> localTick{0};
inline std::atomic<bool> validatedLocal{false},probeBound{false},nativeVisibilityHook{false};
inline std::atomic<unsigned long> localCalls{0},nativeTrue{0},nativeFalse{0};
using ShouldDraw=bool(__thiscall*)(void*);
inline std::atomic<ShouldDraw> upstream{nullptr};
inline void** slotTable=nullptr;
constexpr std::uint32_t RenderableTableRva=0x5b63dc,RenderableColRva=0x69fc18;
constexpr std::uint32_t PrimaryTableRva=0x5b6494,NativeShouldDrawRva=0x253140;
constexpr unsigned ShouldDrawSlot=3;
inline bool __fastcall probe(void* self,void*) {
 auto fn=upstream.load(std::memory_order_acquire);
 // Always pass through exactly once, return the unmodified native result.
 bool result=fn?fn(self):false;
 if(!probeBound.load(std::memory_order_relaxed)||
    !validatedLocal.load(std::memory_order_relaxed)||
    !std::atomic_load(&options)->enabled)return result;
 auto base=localBase.load(std::memory_order_relaxed);
 if(!base||std::uint32_t(std::uintptr_t(self))!=base+4)return result;
 auto camera=std::atomic_load(&source_camera::snapshot);
 auto tick=localTick.load(std::memory_order_relaxed);
 if(!camera->active||camera->epoch!=currentEpoch.load(std::memory_order_relaxed)||
    camera->tick<tick||camera->tick-tick>250)return result;
 auto handle=currentHandle.load(std::memory_order_relaxed);
 auto index=handle&4095u;
 if(index>=source_entities::profile::MaxNetworked||!source_entities::list)return result;
 auto object=std::uint32_t(std::uintptr_t(source_entities::list));
 auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry entry{};
 if(!source_entities::wire::load(source_entities::reader,entries+index*sizeof(entry),entry)||
    entry.entity!=base||entry.serial!=(handle>>12))return result;
 localCalls.fetch_add(1,std::memory_order_relaxed);
 (result?nativeTrue:nativeFalse).fetch_add(1,std::memory_order_relaxed);
 return result;
}
inline void clear() noexcept {
 validatedLocal=false;currentHandle=0xffffffffu;currentEpoch=0;localBase=0;localTick=0;
}
inline bool swap(void** table,void* expected,void* replacement) {
 if(!table||GetModuleHandleW(L"client.dll")!=source_camera::clientModule)return false;
 DWORD old=0;
 if(!VirtualProtect(table+ShouldDrawSlot,4,PAGE_READWRITE,&old))return false;
 auto found=InterlockedCompareExchangePointer(table+ShouldDrawSlot,replacement,expected);
 DWORD ignored=0;bool protectedAgain=VirtualProtect(table+ShouldDrawSlot,4,old,&ignored)!=0;
 return found==expected&&protectedAgain;
}
inline void detach() noexcept {
 if(probeBound.exchange(false,std::memory_order_acq_rel)) {
  auto original=upstream.load(std::memory_order_acquire);
  if(!swap(slotTable,reinterpret_cast<void*>(&probe),reinterpret_cast<void*>(original)))
    source_camera::log("FirstPersonBody: downstream hook owns slot; pinned thunk retained");
 }
}
inline bool install(std::uint32_t base) {
 if(probeBound.load(std::memory_order_acquire))return true;
 auto module=source_camera::clientModule;
 if(!module||!source_camera::matchesImage(module,source_camera::ClientTimestamp,
    source_camera::ClientImageSize,"first person client")||
    !visual::verifiedModuleFile(module,
    "c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")) {
   status=u8"未验证客户端 PE/哈希：拒绝 Hook";return false;
 }
 auto image=std::uint32_t(std::uintptr_t(module));
 auto vt=std::uint32_t(image+RenderableTableRva);
 std::uint32_t col=0,primary=0,secondary=0;
 if(!source_entities::wire::load(source_entities::reader,vt-4,col)||
    col!=image+RenderableColRva||
    !source_entities::wire::load(source_entities::reader,base,primary)||
    primary!=image+PrimaryTableRva||
    !source_entities::wire::load(source_entities::reader,base+4,secondary)||
    secondary!=vt){
   status=u8"本机实体 RTTI/对象虚表不匹配：拒绝 Hook";return false;
 }
 constexpr unsigned char prefix[]={0x56,0x8b,0xf1,0x8b,0x86,0xdc,0,0,0,0xc1,0xe8,0x05,0xa8};
 auto native=reinterpret_cast<const void*>(std::uintptr_t(image+NativeShouldDrawRva));
 if(!source_camera::readable(native,sizeof(prefix))||
    std::memcmp(native,prefix,sizeof(prefix))||!source_camera::executable(native)){
   status=u8"ShouldDraw 代码指纹不匹配：拒绝 Hook";return false;
 }
 auto table=reinterpret_cast<void**>(std::uintptr_t(vt));
 void* before=nullptr;
 if(!source_camera::readable(table,(ShouldDrawSlot+1)*4)||
    !source_entities::reader.read(vt+ShouldDrawSlot*4,&before,4)||
    !source_camera::executable(before)||before==reinterpret_cast<void*>(&probe)){
   status=u8"ShouldDraw 虚表槽不可安全调用：拒绝 Hook";return false;
 }
 // Preserve upstream vtable hooks; pin to survive a subsequent downstream chain.
 HMODULE pinned=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
    reinterpret_cast<LPCWSTR>(&probe),&pinned)){
   status=u8"无法固定诊断 Hook 模块";return false;
 }
 upstream.store(reinterpret_cast<ShouldDraw>(before),std::memory_order_release);
 slotTable=table;
 if(!swap(table,before,reinterpret_cast<void*>(&probe))){
   status=u8"虚表安装冲突：保持原状";return false;
 }
 probeBound=true;
 source_camera::log("FirstPersonBody: verified ShouldDraw[3] observation hook installed; original result preserved");
 return true;
}
inline void onFrame(const dango::GameCameraFrame& camera) noexcept {
 struct RestoreOnFailure {bool succeeded=false;~RestoreOnFailure(){if(!succeeded)native::disable();}} gate;
 if(!std::atomic_load(&options)->enabled){
  clear();detach();status=u8"关闭：原生可见性保持不变";return;
 }
 if(!camera.active||!source_camera::hooked){
  clear();detach();status=u8"等待有效游戏相机";return;
 }
 auto frame=std::atomic_load(&source_entities::snapshot);
 if(!source_entities::bound||source_entities::faulted||frame->epoch!=camera.epoch||
    camera.tick<frame->tick||camera.tick-frame->tick>250){
  clear();detach();status=u8"实体快照未验证或过期";return;
 }
 auto handle=visual::localplayer::handle(*frame);
 if(handle==0xffffffffu){
  clear();detach();status=u8"本机身份/引擎 ABI 未通过";return;
 }
 const source_entities::Record* actor=nullptr;
 for(const auto& r:frame->records)if(r.handle==handle){actor=&r;break;}
 if(!actor||actor->className!="CTerrorPlayer"||!actor->team||*actor->team!=2||
    !actor->health||*actor->health<=0||!actor->lifeState||*actor->lifeState!=0||
    !actor->modelIndex||*actor->modelIndex<=0||
    (actor->dormant&&*actor->dormant)){
  clear();detach();status=u8"非正常存活本机求生者：不安装探针";return;
 }
 auto index=handle&4095u;
 auto object=std::uint32_t(std::uintptr_t(source_entities::list));
 auto entries=std::uint32_t(std::int64_t(object)+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry entry{};
 if(!source_entities::wire::load(source_entities::reader,entries+index*sizeof(entry),entry)||
    !entry.entity||entry.serial!=(handle>>12)){
  clear();detach();status=u8"实体 handle/serial 未通过";return;
 }
 if(!install(entry.entity)){clear();return;}
 currentHandle=handle;currentEpoch=camera.epoch;localBase=entry.entity;localTick=camera.tick;
 validatedLocal=true;
 if(!native::enable(entry.entity)){status=native::state.load();return;}
 gate.succeeded=true;
 status=u8"本机原生 DrawModel 分支已放行（游戏内未验证）";
}
inline void stop() noexcept {
 native::disable();clear();detach();status=u8"插件退出；原生可见性未修改";
}
inline bool isDrawing() noexcept {return false;}
}
