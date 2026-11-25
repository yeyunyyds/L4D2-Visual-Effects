#pragma once
// Diagnostic branch only: no new hooks, no game calls through the unknown getter,
// no writes to the observed field, no change to FPB behavior.
#include "first_person_body_effects.hpp"
#include "first_person_native_probe_policy.hpp"
#include <array>
#include <cstdio>
#include <string>
namespace visual::firstperson::nativeprobe {
constexpr std::uint32_t GetterRva=0xf670u;
inline std::string hex(const std::uint8_t* p,std::size_t n) {
 static constexpr char digits[]="0123456789abcdef";
 std::string s;s.reserve(n*2);
 for(std::size_t i=0;i<n;++i){s+=digits[p[i]>>4];s+=digits[p[i]&15];}
 return s;
}
struct Probe {
 HMODULE module=nullptr;
 bool verified=false,announced=false,active=false;
 std::uint64_t nextCheck=0,lastFlush=0;
 std::uint32_t epoch=0;
 std::array<unsigned long,7> previous{};
 std::string previousKey,batch;
 unsigned events=0,dropped=0;
 void flush(){
  if(!batch.empty())source_camera::log(batch);
  if(dropped)source_camera::log("FPB_NATIVE_PROBE overflow="+std::to_string(dropped));
  batch.clear();events=dropped=0;
 }
 void dump(std::uint32_t base,std::uint32_t rva,std::size_t size,const char* name){
  std::array<std::uint8_t,256> bytes{};
  char prefix[160]{};
  std::snprintf(prefix,sizeof(prefix),"FPB_NATIVE_PROBE %s base=0x%08x rva=0x%08x bytes=",name,unsigned(base),unsigned(rva));
  if(size<=bytes.size()&&std::uint64_t(rva)+size<=source_camera::ClientImageSize&&
     source_entities::reader.read(base+rva,bytes.data(),size))
   source_camera::log(std::string(prefix)+hex(bytes.data(),size));
  else source_camera::log(std::string(prefix)+"unreadable");
 }
 void frame(const dango::GameCameraFrame& f,bool freeRequested,bool nativeBefore) {
  if(!f.active){
   if(active){flush();source_camera::log("FPB_NATIVE_PROBE inactive");}
   active=false;previousKey.clear();return;
  }
  auto m=source_camera::clientModule;
  if(m!=module){flush();*this=Probe{};module=m;}
  if(!m||GetModuleHandleW(L"client.dll")!=m)return;
  if(!verified){
   if(f.tick<nextCheck)return;
   nextCheck=f.tick+5000;
   verified=source_camera::matchesImage(m,source_camera::ClientTimestamp,source_camera::ClientImageSize,"fpb native probe")&&
    visual::verifiedModuleFile(m,"4d01ed9fb856d57ce549b8587c386f361083ee581b001295fac0eff4c835d52d");
   if(!verified){source_camera::log("FPB_NATIVE_PROBE unsupported client; no field read");return;}
  }
  const auto base=std::uint32_t(std::uintptr_t(m));
  if(!announced){
   announced=true;
   source_camera::log("FPB_NATIVE_PROBE diagnostic build; passive reads; native byte semantics unproven; post-render samples");
   dump(base,branch::LocalGateRva-96,96+sizeof(branch::GateBytes),"gate-context");
   dump(base,GetterRva,160,"getter");
  }
  int raw=-1,displacement=-1;
  const char* reason="gate-signature-rejected";
  Getter getter{};
  std::array<std::uint8_t,sizeof(branch::GateBytes)> gate{};
  if(source_entities::reader.read(base+branch::LocalGateRva,gate.data(),gate.size())&&
      branch::matches(gate.data(),gate.size(),true)){
   displacement=gate[branch::DisplacementOffset];
   std::uint8_t bytes[7]{};
   reason="getter-unreadable";
   if(source_camera::executable(reinterpret_cast<void*>(std::uintptr_t(base+GetterRva)))&&
      source_entities::reader.read(base+GetterRva,bytes,sizeof(bytes))){
    getter=decode(bytes,sizeof(bytes));reason="getter-unresolved";
    if(getter.kind!=GetterKind::Unknown){
     auto object=getter.operand;bool resolved=true;
     if(getter.kind==GetterKind::AbsoluteSlot){
      resolved=getter.operand>=base&&std::uint64_t(getter.operand)+4<=std::uint64_t(base)+source_camera::ClientImageSize&&
       source_entities::reader.read(getter.operand,&object,sizeof(object));
     }
     std::uint32_t address=0;std::uint8_t value=0;reason="object-unreadable";
     if(resolved&&fieldAddress(object,address)&&source_entities::reader.read(address,&value,sizeof(value))){
      raw=int(value);reason="sampled-unproven";
     }
    }
   }
  }
  const auto cfg=std::atomic_load(&options);
  const auto shoulder=viewcompat::readShoulder();
  const int sh=shoulder?(*shoulder?1:0):-1;
  const auto mode=viewcompat::mode.load();
  const bool nativeOn=native::active.load(),local=validatedLocal.load();
  std::array<unsigned long,7> counters={effects::matched.load(),effects::dynamicApplied.load(),
   effects::dynamicRejected.load(),effects::headHidden.load(),effects::headRejected.load(),
   effects::armsHidden.load(),effects::armsRejected.load()};
  std::array<unsigned long,7> delta{};
  const bool newEpoch=!active||epoch!=f.epoch;
  if(!newEpoch)for(std::size_t i=0;i<delta.size();++i)delta[i]=counters[i]-previous[i];
  previous=counters;epoch=f.epoch;active=true;
  char key[384]{};
  std::snprintf(key,sizeof(key),"%u:%d:%d:%s:%s:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d",
    unsigned(epoch),raw,displacement,reason,viewcompat::label(mode),sh,int(cfg->enabled),
    int(cfg->thirdPersonCompatibility),int(cfg->dynamicAnchor),int(nativeOn),int(nativeBefore),int(local),
    int(freeRequested),int(worldBodyActive.load()),int(delta[0]!=0),int(delta[1]!=0),int(delta[2]!=0),
    int(delta[3]!=0),int(delta[4]!=0),int(delta[5]!=0),int(delta[6]!=0));
  const bool heartbeat=newEpoch||f.tick<lastFlush||f.tick-lastFlush>=1000;
  if(heartbeat){flush();lastFlush=f.tick;}
  if(heartbeat||previousKey!=key){
   char line[1024]{};
   std::snprintf(line,sizeof(line),"FPB_NATIVE_PROBE sample frame=%llu epoch=%u sampleTick=%llu raw=%d originalWouldDraw=%d getter=%s reason=%s jz=%d fpb=%d compat=%d dynamic=%d head=%d arms=%d free=%d world=%d nativeBefore=%d nativeAfter=%d local=%d view=%s shoulder=%d xy=%d eyeZ=%d deltaMatch=%lu deltaAligned=%lu deltaRejected=%lu deltaHead=%lu/%lu deltaArms=%lu/%lu totals=%lu/%lu/%lu/%lu/%lu/%lu/%lu",
    static_cast<unsigned long long>(source_camera::renderBegins.load()),unsigned(epoch),static_cast<unsigned long long>(f.tick),
    raw,raw<0?-1:int(raw!=0),label(getter.kind),reason,displacement,int(cfg->enabled),
    int(cfg->thirdPersonCompatibility),int(cfg->dynamicAnchor),int(cfg->hideHead),int(cfg->hideArms),
    int(freeRequested),int(worldBodyActive.load()),int(nativeBefore),int(nativeOn),int(local),viewcompat::label(mode),sh,
    viewcompat::horizontalUnits.load(),viewcompat::eyeHeightUnits.load(),
    delta[0],delta[1],delta[2],delta[3],delta[4],delta[5],delta[6],
    counters[0],counters[1],counters[2],counters[3],counters[4],counters[5],counters[6]);
   if(events<64){if(!batch.empty())batch+='\n';batch+=line;++events;}else ++dropped;
   previousKey=key;
  }
 }
};
inline void sample(const dango::GameCameraFrame& frame,bool freeRequested,bool nativeBefore) noexcept {
 try{static Probe probe;probe.frame(frame,freeRequested,nativeBefore);}catch(...){}
}
}
