#pragma once
// Approach A: modify only the rel8 displacement in the verified native
// C_TerrorPlayer local-player DrawModel early-out. No extra render calls.
#include <windows.h>
#include <atomic>
#include <mutex>
#include <cstring>
#include "first_person_body_patch.hpp"
#include "module_verification.hpp"
namespace visual::firstperson::native {
inline std::atomic<bool> active{false},faulted{false},restoredVerified{true};
inline std::atomic<unsigned long> enables{0},restores{0},failures{0};
inline std::atomic<const char*> state{u8"等待本机存活求生者"};
inline std::mutex mutex;
inline HMODULE owner=nullptr;
inline void fail(const char* message) noexcept {
 state=message;faulted=true;++failures;source_camera::log(std::string("FPB: ")+message);
}
inline bool patch(bool enable) noexcept {
 std::lock_guard<std::mutex> lock(mutex);
 if(active.load()==enable)return true;
 HMODULE module=enable?source_camera::clientModule:owner;
 if(!module||GetModuleHandleW(L"client.dll")!=module){
  fail(u8"client.dll 实例不匹配；拒绝写入");return false;
 }
 std::uintptr_t base=std::uintptr_t(module);
 unsigned char sample[sizeof(branch::GateBytes)]{};
 if(!source_entities::reader.read(std::uint32_t(base+branch::LocalGateRva),sample,sizeof(sample))||
    !branch::matches(sample,sizeof(sample),true)||
    sample[branch::DisplacementOffset]!=(enable?branch::Original:branch::PassThrough)){
  fail(u8"本机 DrawModel 字节或第三方补丁冲突；拒绝写入");return false;
 }
 auto target=reinterpret_cast<unsigned char*>(base+branch::DisplacementRva);
 if(!source_camera::executable(target)){fail(u8"补丁目标不可执行");return false;}
 DWORD old=0,ignored=0;
 if(!VirtualProtect(target,1,PAGE_EXECUTE_READWRITE,&old)){fail(u8"无法修改代码页保护");return false;}
 // Single x86 byte store: JZ +0x13 -> JZ +0. It falls through to the
 // SAME original C_BasePlayer DrawModel call in the engine's normal flow.
 *reinterpret_cast<volatile unsigned char*>(target)=enable?branch::PassThrough:branch::Original;
 FlushInstructionCache(GetCurrentProcess(),target,1);
 bool restoredProtection=VirtualProtect(target,1,old,&ignored)!=FALSE;
 unsigned char confirmed=0;
 const bool readback=source_entities::reader.read(
  std::uint32_t(base+branch::DisplacementRva),&confirmed,1)&&
  confirmed==(enable?branch::PassThrough:branch::Original);
 if(!readback){
  // Keep the gate in a pending-restoration state rather than report clean.
  active=true;owner=module;restoredVerified=false;
  fail(u8"DrawModel 补丁字节未验证；恢复状态未知");return false;
 }
 active=enable;restoredVerified=!enable;
 if(enable){owner=module;++enables;}else{owner=nullptr;++restores;}
 if(!restoredProtection){fail(u8"代码保护恢复失败；请退出游戏");return false;}
 state=enable?u8"原生 DrawModel 分支已放行":u8"原生 DrawModel 字节已验证恢复";
 source_camera::log(enable?"FPB: native local draw gate opened (+0)":"FPB: native local draw gate restored (+0x13)");
 return true;
}
inline void disable() noexcept {if(active.load())patch(false);}
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
 if(faulted.load())return false;
 if(active.load())return true;
 if(!verify(entity))return false;
 if(!patch(true))return false;
 state=u8"原生本机 DrawModel 分支已放行（未实机验证）";return true;
}
}
