#pragma once
#include "tank_runtime.hpp"
#include "tank_warning.hpp"
#include "module_verification.hpp"
namespace visual::tank::outline {
using SetType=void(__thiscall*)(void*,int);
using ColorProxy=void(__cdecl*)(const void*,void*,void*);
inline SetType setType=nullptr;inline ColorProxy colorProxy=nullptr;
inline std::atomic<const char*> status{u8"描边提醒关闭"};
inline std::atomic<DWORD> owner{0};inline std::atomic<bool> stopped{false},clean{true};
struct Slot{std::uint32_t epoch=0,handle=0,base=0;GlowLayout layout;GlowLease lease;};
inline std::map<std::uint32_t,Slot> slots;
inline bool contract(){
 auto m=source_camera::clientModule;if(!m||GetModuleHandleW(L"client.dll")!=m)return false;
 static HMODULE checked=nullptr;static bool accepted=false;if(checked!=m){checked=m;accepted=verifiedModuleFile(m,"c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f");}if(!accepted)return false;
 constexpr unsigned char typeBytes[]={0x55,0x8b,0xec,0x53,0x8b,0x5d,0x08,0x56,0x57,0x8b,0xf9,0x39,0x5f,0x04,0x8d,0x77,0x04,0x74,0x0d,0x8b,0x46,0xfc,0x8b,0x10,0x8d,0x4e,0xfc,0x56,0xff,0xd2};
 constexpr unsigned char colorBytes[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0x8b,0x48,0x04,0x8b,0x45,0x0c,0x8b,0x55,0x10,0xf3,0x0f,0x10,0x05};
 if(!source_entities::bytes(m,0x109a20,typeBytes)||!source_entities::bytes(m,0x1098a0,colorBytes))return false;
 auto base=reinterpret_cast<uintptr_t>(m);if(!source_camera::executable(reinterpret_cast<void*>(base+0x109a20))||!source_camera::executable(reinterpret_cast<void*>(base+0x1098a0)))return false;
 setType=reinterpret_cast<SetType>(base+0x109a20);colorProxy=reinterpret_cast<ColorProxy>(base+0x1098a0);return true;
}
inline bool pinModule(){static bool pinned=false;if(pinned)return true;HMODULE module=nullptr;pinned=GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN|GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<const wchar_t*>(&status),&module)!=FALSE;return pinned;}
inline bool current(const Slot& s){
 if(std::atomic_load(&source_camera::snapshot)->epoch!=s.epoch||!source_entities::list||GetModuleHandleW(L"client.dll")!=source_camera::clientModule)return false;
 auto entries=std::uint32_t(std::int64_t(reinterpret_cast<uintptr_t>(source_entities::list))+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry e;
 return source_entities::wire::load(source_entities::reader,entries+(s.handle&4095)*16,e)&&e.entity==s.base&&e.serial==(s.handle>>12);
}
inline bool read(const Slot& s,std::array<int,4>& value){return current(s)&&source_entities::reader.read(s.base+s.layout.type,value.data(),sizeof(value));}
inline bool writeInt(std::uint32_t address,int value){SIZE_T n=0;return WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(uintptr_t(address)),&value,4,&n)&&n==4;}
inline void color(const Slot& s,int value){
 // Exact x86 CRecvProxyData: prop*, DVariant[16], element, objectID.
 struct Data{std::uint32_t prop;int value;std::uint32_t variant[3];int element,object;};static_assert(sizeof(Data)==28,"RecvProxy ABI");
 Data data{};data.value=value;colorProxy(&data,reinterpret_cast<void*>(uintptr_t(s.base+s.layout.property)),reinterpret_cast<void*>(uintptr_t(s.base+s.layout.type+12)));
}
inline void restore(const Slot& s){std::array<int,4> now{};if(!read(s,now)||!contract())return;auto own=s.lease.owned(now);
 for(unsigned i=1;i<3;++i)if(own[i])writeInt(s.base+s.layout.type+i*4,s.lease.before[i]);
 if(own[3])color(s,s.lease.before[3]);if(own[0])setType(reinterpret_cast<void*>(uintptr_t(s.base+s.layout.property)),s.lease.before[0]);
}
inline void cleanup(){for(const auto& pair:slots)restore(pair.second);slots.clear();clean=true;}
inline GlowLayout layout(unsigned index){
 static std::uint32_t epoch=0;static std::map<std::uint32_t,GlowLayout> cache;auto camera=std::atomic_load(&source_camera::snapshot);if(epoch!=camera->epoch){cache.clear();epoch=camera->epoch;}auto object=std::uint32_t(reinterpret_cast<uintptr_t>(source_entities::list));std::uint32_t net=0;
 if(!source_entities::wire::load(source_entities::reader,object+source_entities::profile::CacheOffset+index*8,net)||!net)return {};
 auto cls=source_entities::classAddress(net);source_entities::wire::ClientClass cc;
 if(!cls||!source_entities::wire::load(source_entities::reader,*cls,cc))return {};
 auto found=cache.find(cc.table);if(found!=cache.end())return found->second;return cache.emplace(cc.table,glowLayout(source_entities::reader,cc.table)).first->second;
}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 owner=GetCurrentThreadId();auto cfg=std::atomic_load(&options);auto result=std::atomic_load(&snapshot);
 if(stopped||faulted||!cfg->enabled||!cfg->draw||!cfg->warnOutline||!camera.active||camera.epoch!=result->epoch||camera.tick<result->tick||camera.tick-result->tick>250||!display::foreground||display::paused){if(!clean)cleanup();status=u8"描边提醒关闭或画面暂停";return;}
 static std::shared_ptr<const Result> previous;static std::shared_ptr<const Settings> previousCfg;
 if(previous==result&&previousCfg==cfg)return;previous=result;previousCfg=cfg;
 std::set<std::uint32_t> wanted;for(const auto& o:result->tanks)if(o.warning.known&&o.warning.inside)wanted.insert(o.handle);
 if(wanted.empty()){if(!clean)cleanup();status=u8"等待玩家进入攻击范围";return;}
 if(!contract()){if(!clean)cleanup();status=u8"描边接口未验证，不调用";return;}
 for(auto it=slots.begin();it!=slots.end();)if(!wanted.count(it->first)||it->second.epoch!=camera.epoch){restore(it->second);it=slots.erase(it);}else ++it;
 bool busy=false,rejected=false;
 for(auto handle:wanted){auto old=slots.find(handle);if(old!=slots.end()){std::array<int,4> now{};if(read(old->second,now)&&now==old->second.lease.written)continue;restore(old->second);slots.erase(old);}
  if(!source_entities::list)continue;Slot s;s.epoch=camera.epoch;s.handle=handle;
  auto entries=std::uint32_t(std::int64_t(reinterpret_cast<uintptr_t>(source_entities::list))+source_entities::profile::EntryBaseDelta);source_entities::wire::Entry e;
  if(!source_entities::wire::load(source_entities::reader,entries+(handle&4095)*16,e)||e.serial!=(handle>>12)||!e.entity){rejected=true;continue;}s.base=e.entity;s.layout=layout(handle&4095);
  std::array<int,4> now{};std::uint32_t vt=0,first=0;
  if(!s.layout||!read(s,now)||!source_entities::wire::load(source_entities::reader,s.base+s.layout.property,vt)||!source_camera::owns(reinterpret_cast<void*>(uintptr_t(vt)),source_camera::clientModule)||!source_entities::wire::load(source_entities::reader,vt,first)||!source_camera::owns(reinterpret_cast<void*>(uintptr_t(first)),source_camera::clientModule)||!source_camera::executable(reinterpret_cast<void*>(uintptr_t(first)))){rejected=true;continue;}if(!s.lease.acquire(now)){busy=true;continue;}
  if(!pinModule()){rejected=true;continue;}auto added=slots.emplace(handle,s).first;clean=false;bool okay=true;for(unsigned i=1;i<3;++i)okay=writeInt(s.base+s.layout.type+i*4,s.lease.written[i])&&okay;
  if(!okay){restore(added->second);slots.erase(added);rejected=true;continue;}
  color(s,s.lease.written[3]);setType(reinterpret_cast<void*>(uintptr_t(s.base+s.layout.property)),s.lease.written[0]);
 }
 clean=slots.empty();status=!slots.empty()?u8"范围内 Tank 红色描边中":busy?u8"已有描边，保留原效果":rejected?u8"描边布局或实体校验未通过":u8"等待玩家进入攻击范围";
}catch(...){cleanup();status=u8"描边异常，已停止本次提醒";}
inline void stop(){stopped=true;if(owner==GetCurrentThreadId()){cleanup();return;}for(unsigned i=0;i<30&&!clean;++i)Sleep(5);if(!clean)source_camera::log("Tank outline restore deferred: no game-thread frame during stop");}
}
