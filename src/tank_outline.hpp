#pragma once
#include "tank_runtime.hpp"
#include "tank_warning.hpp"
#include "special_glow_settings.hpp"
#include "entity_visibility_runtime.hpp"
#include "module_verification.hpp"
namespace visual::tank::outline {
using SetType=void(__thiscall*)(void*,int);
using ColorProxy=void(__cdecl*)(const void*,void*,void*);
inline SetType setType=nullptr;inline ColorProxy colorProxy=nullptr;
inline std::atomic<const char*> status{u8"描边提醒关闭"};
inline std::atomic<unsigned> activeCount{0};
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
inline void cleanup(){for(const auto& pair:slots)restore(pair.second);slots.clear();activeCount=0;clean=true;}
inline GlowLayout layout(unsigned index){
 static std::uint32_t epoch=0;static std::map<std::uint32_t,GlowLayout> cache;auto camera=std::atomic_load(&source_camera::snapshot);if(epoch!=camera->epoch){cache.clear();epoch=camera->epoch;}auto object=std::uint32_t(reinterpret_cast<uintptr_t>(source_entities::list));std::uint32_t net=0;
 if(!source_entities::wire::load(source_entities::reader,object+source_entities::profile::CacheOffset+index*8,net)||!net)return {};
 auto cls=source_entities::classAddress(net);source_entities::wire::ClientClass cc;
 if(!cls||!source_entities::wire::load(source_entities::reader,*cls,cc))return {};
 auto found=cache.find(cc.table);if(found!=cache.end())return found->second;return cache.emplace(cc.table,glowLayout(source_entities::reader,cc.table)).first->second;
}
// One manager owns a native Glow lease per entity. Tank attack warning wins
// over the per-class setting, and both modes return to the same saved original.
struct Desired{int type=3,color=0;bool operator==(const Desired& r)const{return type==r.type&&color==r.color;}};
inline bool apply(Slot& s,const Desired& desired){
 std::array<int,4> next{desired.type,8192,0,desired.color};
 bool okay=true;for(unsigned i=1;i<3;++i)okay=writeInt(s.base+s.layout.type+i*4,next[i])&&okay;
 if(!okay){
  std::array<int,4> now{};if(read(s,now))for(unsigned i=1;i<3;++i)if(now[i]==next[i])writeInt(s.base+s.layout.type+i*4,s.lease.before[i]);
  return false;
 }
 // The native proxy also refreshes the engine RGB cache.
 color(s,next[3]);setType(reinterpret_cast<void*>(uintptr_t(s.base+s.layout.property)),next[0]);
 s.lease.written=next;return true;
}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 owner=GetCurrentThreadId();
 auto cfg=std::atomic_load(&options);
 auto special=std::atomic_load(&specialglow::options);
 auto prediction=std::atomic_load(&snapshot);
 auto inventory=std::atomic_load(&source_entities::snapshot);
 auto now=GetTickCount64();
 bool warn=cfg->enabled&&cfg->draw&&cfg->warnOutline&&!faulted&&
    display::foreground&&!display::paused&&camera.active&&
    prediction->epoch==camera.epoch&&camera.tick>=prediction->tick&&camera.tick-prediction->tick<=250;
 bool native=warn||special->any();
 if(stopped||!camera.active||!native){if(!clean)cleanup();status=u8"描边已关闭";return;}
 if(!inventory||inventory->epoch!=camera.epoch||camera.tick<inventory->tick||camera.tick-inventory->tick>350){
  if(!clean)cleanup();status=u8"实体快照不可用";return;
 }
 static std::uint64_t last=0;static std::shared_ptr<const Settings> oldCfg;
 static std::shared_ptr<const specialglow::Settings> oldSpecial;static std::shared_ptr<const Result> oldPrediction;
 if(now>=last&&now-last<40&&cfg==oldCfg&&special==oldSpecial&&prediction==oldPrediction)return;
 last=now;oldCfg=cfg;oldSpecial=special;oldPrediction=prediction;
 std::map<std::uint32_t,Desired> wanted;
 for(const auto& r:inventory->records){
  if(!specialglow::eligible(r))continue;
  int role=specialCharacter(r);if(role<0)continue;
  const auto& rule=special->types[unsigned(role)];
  if(!rule.enabled)continue;
  if(rule.occlude&&!visibility::visible(camera.epoch,r.handle,now))continue;
  // LOS-mode glow is used in occluded mode. Constant mode draws through walls.
  wanted[r.handle]={rule.occlude?2:3,int(rule.color)};
 }
 if(warn)for(const auto& o:prediction->tanks){
  if(!o.warning.known||!o.warning.inside)continue;
  auto it=std::find_if(inventory->records.begin(),inventory->records.end(),
     [&](const auto& r){return r.handle==o.handle&&specialglow::eligible(r)&&specialCharacter(r)==6;});
  if(it!=inventory->records.end())wanted[o.handle]={2,255}; // original red warning priority
 }
 if(wanted.empty()){if(!clean)cleanup();status=u8"等待有效特感或 Tank 警告";return;}
 if(!contract()){if(!clean)cleanup();status=u8"描边接口未验证，不调用";return;}
 for(auto it=slots.begin();it!=slots.end();){
  if(!wanted.count(it->first)||it->second.epoch!=camera.epoch){
   restore(it->second);it=slots.erase(it);
  }else ++it;
 }
 bool busy=false,rejected=false;
 for(const auto& request:wanted){
  auto handle=request.first;const auto& desired=request.second;
  auto old=slots.find(handle);
  if(old!=slots.end()){
   std::array<int,4> nowFields{};
   if(!read(old->second,nowFields)||nowFields!=old->second.lease.written){
    // Do not fight other mods or the server if they changed our values.
    restore(old->second);slots.erase(old);busy=true;continue;
   }
   if(nowFields[0]==desired.type&&nowFields[3]==desired.color)continue;
   if(!apply(old->second,desired)){restore(old->second);slots.erase(old);rejected=true;}
   continue;
  }
  if(!source_entities::list)continue;
  Slot slot;slot.epoch=camera.epoch;slot.handle=handle;
  auto entries=std::uint32_t(std::int64_t(reinterpret_cast<uintptr_t>(source_entities::list))+source_entities::profile::EntryBaseDelta);
  source_entities::wire::Entry entry{};
  if(!source_entities::wire::load(source_entities::reader,entries+(handle&4095)*sizeof(entry),entry)||
      !entry.entity||entry.serial!=(handle>>12)){rejected=true;continue;}
  slot.base=entry.entity;slot.layout=layout(handle&4095);
  std::array<int,4> currentFields{};std::uint32_t vt=0,firstMethod=0;
  if(!slot.layout||!read(slot,currentFields)||
     !source_entities::wire::load(source_entities::reader,slot.base+slot.layout.property,vt)||
     !source_camera::owns(reinterpret_cast<void*>(uintptr_t(vt)),source_camera::clientModule)||
     !source_entities::wire::load(source_entities::reader,vt,firstMethod)||
     !source_camera::owns(reinterpret_cast<void*>(uintptr_t(firstMethod)),source_camera::clientModule)||
     !source_camera::executable(reinterpret_cast<void*>(uintptr_t(firstMethod)))){
   rejected=true;continue;
  }
  if(!slot.lease.acquire(currentFields)){busy=true;continue;}
  if(!pinModule()){rejected=true;continue;}
  auto inserted=slots.emplace(handle,slot).first;clean=false;
  if(!apply(inserted->second,desired)){restore(inserted->second);slots.erase(inserted);rejected=true;}
 }
 activeCount=unsigned(slots.size());clean=slots.empty();
 status=!slots.empty()?u8"特感原生描边运行中":busy?u8"已有描边或被其他插件接管":
   rejected?u8"描边布局或实体校验失败":u8"等待可用实体";
}catch(...){cleanup();status=u8"描边异常，已恢复已接管实体";}
inline void stop(){stopped=true;if(owner==GetCurrentThreadId()){cleanup();return;}for(unsigned i=0;i<30&&!clean;++i)Sleep(5);if(!clean)source_camera::log("Tank outline restore deferred: no game-thread frame during stop");}
}
