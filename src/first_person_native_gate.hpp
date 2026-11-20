#pragma once
// Only the verified local-player DrawModel JZ displacement is modified.
// A failed readback or page-protection restore is UNKNOWN, not "patched".
#include <windows.h>
#include <atomic>
#include <mutex>
#include <cstring>
#include "first_person_body_patch.hpp"
#include "first_person_native_gate_policy.hpp"
#include "module_verification.hpp"
namespace visual::firstperson::native {
inline std::atomic<bool> active{false},faulted{false},restoredVerified{true};
inline std::atomic<gatepolicy::Phase> phase{gatepolicy::Phase::Clean};
inline std::atomic<unsigned long> enables{0},restores{0},failures{0};
inline std::atomic<const char*> state{u8"等待本机存活求生者"};
inline std::mutex mutex;
inline HMODULE owner=nullptr;
// Preserve the prior page protection when VirtualProtect restoration fails.
inline DWORD pendingProtection=0;
inline void fail(const char* message) noexcept {
 state=message;++failures;
 // Avoid repeatedly logging the same unresolved failure every game frame.
 if(!faulted.exchange(true))source_camera::log(std::string("FPB: ")+message);
}
inline void uncertain(HMODULE module,const char* reason) noexcept {
 owner=module;
 phase=gatepolicy::Phase::Unknown;
 active=false;
 restoredVerified=false;
 fail(reason);
}
inline bool patch(bool enable) noexcept {
 using gatepolicy::Phase;using gatepolicy::Decision;
 std::lock_guard<std::mutex> lock(mutex);
 const auto prior=phase.load(std::memory_order_acquire);
 if(!enable&&prior==Phase::Clean&&restoredVerified.load())return true;
 if(enable&&prior==Phase::Unknown){
  fail(u8"DrawModel 状态未知；拒绝重新启用");return false;
 }
 HMODULE module=enable?source_camera::clientModule:owner;
 if(!module||GetModuleHandleW(L"client.dll")!=module){
  if(!enable){phase=Phase::Unknown;active=false;restoredVerified=false;}
  fail(u8"client.dll 实例不匹配；拒绝写入");return false;
 }
 const auto base=std::uintptr_t(module);
 unsigned char sample[sizeof(branch::GateBytes)]{};
 if(!source_entities::reader.read(std::uint32_t(base+branch::LocalGateRva),
                                  sample,sizeof(sample))||
    !branch::matches(sample,sizeof(sample),true)){
  if(!enable)uncertain(module,u8"DrawModel 机器码不匹配；拒绝覆盖第三方修改");
  else fail(u8"DrawModel 机器码不匹配；拒绝覆盖第三方修改");
  return false;
 }
 const auto observed=gatepolicy::classify(sample[branch::DisplacementOffset],
                                          branch::Original,branch::PassThrough);
 const auto decision=gatepolicy::decide(enable,prior,observed);
 if(decision==Decision::Conflict){
  if(!enable)uncertain(module,u8"DrawModel 原生字节所有权冲突；拒绝写入");
  else fail(u8"DrawModel 原生字节所有权冲突；拒绝写入");
  return false;
 }
 auto target=reinterpret_cast<unsigned char*>(base+branch::DisplacementRva);
 if(!source_camera::executable(target)){
  if(!enable)uncertain(module,u8"DrawModel 目标不可执行；拒绝写入");
  else fail(u8"DrawModel 目标不可执行；拒绝写入");
  return false;
 }
 // An earlier protection-restoration failure may have left the page writable.
 // Reconcile that state before declaring the native code clean.
 if(pendingProtection){
  DWORD unused=0;
  if(!VirtualProtect(target,1,pendingProtection,&unused)){
   uncertain(module,u8"DrawModel 页保护恢复仍失败");return false;
  }
  pendingProtection=0;
 }
 if(decision==Decision::AlreadyClean){
  if(prior!=Phase::Clean)++restores;
  phase=Phase::Clean;active=false;owner=nullptr;restoredVerified=true;
  state=u8"原生 DrawModel 字节已确认恢复";return true;
 }
 if(decision==Decision::AlreadyPatched){
  phase=Phase::Patched;active=true;owner=module;restoredVerified=false;
  return true;
 }
 DWORD old=0,ignored=0;
 if(!VirtualProtect(target,1,PAGE_EXECUTE_READWRITE,&old)){
  if(!enable)uncertain(module,u8"无法修改 DrawModel 代码页保护");
  else fail(u8"无法修改 DrawModel 代码页保护");
  return false;
 }
 // Claim the write before performing it. Failure must preserve the owner
 // for the next disable()/plugin shutdown recovery attempt.
 owner=module;phase=Phase::Unknown;active=false;restoredVerified=false;
 const auto desired=enable?branch::PassThrough:branch::Original;
 *reinterpret_cast<volatile unsigned char*>(target)=desired;
 const bool cacheFlushed=FlushInstructionCache(GetCurrentProcess(),target,1)!=FALSE;
 const bool protectedAgain=VirtualProtect(target,1,old,&ignored)!=FALSE;
 if(!protectedAgain)pendingProtection=old;
 unsigned char confirmed=0;
 const bool readback=source_entities::reader.read(
  std::uint32_t(base+branch::DisplacementRva),&confirmed,1)&&confirmed==desired;
 if(!readback){
  uncertain(module,u8"DrawModel 写入后字节未确认；等待安全恢复");return false;
 }
 if(!protectedAgain){
  uncertain(module,u8"DrawModel 页保护未恢复；状态待确认");return false;
 }
 if(!cacheFlushed){
  uncertain(module,u8"DrawModel 指令缓存未确认刷新；状态待恢复");return false;
 }
 phase=enable?Phase::Patched:Phase::Clean;
 active=enable;restoredVerified=!enable;
 if(enable){++enables;}else{owner=nullptr;++restores;}
 state=enable?u8"原生 DrawModel 分支已放行":u8"原生 DrawModel 字节已验证恢复";
 source_camera::log(enable?"FPB: native local draw gate opened (+0)":
                           "FPB: native local draw gate restored (+0x13)");
 return true;
}
inline bool disable() noexcept {
 if(phase.load(std::memory_order_acquire)==gatepolicy::Phase::Clean&&
    restoredVerified.load(std::memory_order_acquire))return true;
 return patch(false);
}
inline bool verify(std::uint32_t entity) noexcept {
 using namespace source_camera;
 auto cm=clientModule,em=engineModule;
 if(!cm||!em||
    !matchesImage(cm,ClientTimestamp,ClientImageSize,"fpb client")||
    !matchesImage(em,EngineTimestamp,EngineImageSize,"fpb engine")||
    !visual::verifiedModuleFile(cm,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f")||
    !visual::verifiedModuleFile(em,"8a6d468059394bf4cfce32f11dcba12ef45de88da1373084d5a2916d0bf7ba86")){
  fail(u8"客户端/引擎 PE 或 SHA-256 不匹配");return false;
 }
 auto image=std::uint32_t(std::uintptr_t(cm));
 auto vt=image+branch::RenderableTableRva;
 std::uint32_t col=0,primary=0,secondary=0,draw=0;
 if(!source_entities::wire::load(source_entities::reader,vt-4,col)||
    col!=image+branch::RenderableColRva||
    !source_entities::wire::load(source_entities::reader,entity,primary)||
    primary!=image+branch::PrimaryTableRva||
    !source_entities::wire::load(source_entities::reader,entity+4,secondary)||
    secondary!=vt||
    !source_entities::wire::load(source_entities::reader,vt+branch::DrawSlot*4,draw)||
    draw!=image+branch::EntryRva||
    !executable(reinterpret_cast<void*>(std::uintptr_t(draw)))){
  fail(u8"C_TerrorPlayer RTTI/DrawModel[9] 不匹配");return false;
 }
 constexpr unsigned char callsite[]={0x8b,0x45,0x0c,0x50,0x56,0x8b,0xcf,0xe8,0x6e,0xac,0xfc,0xff,0x8b,0xf8};
 constexpr unsigned char prologue[]={0x55,0x8b,0xec,0x56,0x57,0x6a,0xff,0x8b,0xf1,0xe8,0x72,0x01,0xe4,0xff};
 auto base=std::uintptr_t(cm);
 if(!readable(reinterpret_cast<void*>(base+0x2586e6),sizeof(callsite))||
    std::memcmp(reinterpret_cast<void*>(base+0x2586e6),callsite,sizeof(callsite))||
    !readable(reinterpret_cast<void*>(base+branch::DrawRva),sizeof(prologue))||
    std::memcmp(reinterpret_cast<void*>(base+branch::DrawRva),prologue,sizeof(prologue))){
  fail(u8"原版 DrawModel 调用点/函数指纹不匹配");return false;
 }
 return true;
}
inline bool enable(std::uint32_t entity) noexcept {
 if(faulted.load()||phase.load()==gatepolicy::Phase::Unknown)return false;
 if(active.load())return true;
 if(!verify(entity))return false;
 if(!patch(true))return false;
 state=u8"原生本机 DrawModel 分支已放行";return true;
}
}
