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
// Independent from FPB's validatedLocal/currentHandle: the production
// path clears those on third-person entry and when the user disables FPB.
struct ActorSample {
 std::uint32_t entity=0,handle=0xffffffffu;
 std::optional<dango::Vec3> origin;
 const char* reason="local-unverified";
};
inline ActorSample verifiedActor(const dango::GameCameraFrame& f,std::uint32_t base) {
 ActorSample out{};
 auto snapshot=std::atomic_load(&source_entities::snapshot);
 if(!source_entities::bound||source_entities::faulted||!source_entities::list||
    snapshot->epoch!=f.epoch||f.tick<snapshot->tick||f.tick-snapshot->tick>250){
  out.reason="snapshot-unverified";return out;
 }
 auto h=visual::localplayer::handle(*snapshot);
 if(h==0xffffffffu)return out;
 const source_entities::Record* actor=nullptr;
 for(const auto& r:snapshot->records)if(r.handle==h){actor=&r;break;}
 if(!actor||actor->className!="CTerrorPlayer"||!actor->team||*actor->team!=2||
    !actor->health||*actor->health<=0||!actor->lifeState||*actor->lifeState!=0||
    !actor->modelIndex||*actor->modelIndex<=0||(actor->dormant&&*actor->dormant)){
  out.reason="not-live-local-survivor";return out;
 }
 const auto entries=std::uint32_t(std::int64_t(std::uintptr_t(source_entities::list))+source_entities::profile::EntryBaseDelta);
 source_entities::wire::Entry entry{};
 auto& reader=source_entities::reader;
 if(!source_entities::wire::load(reader,entries+(h&4095u)*sizeof(entry),entry)||
    !entry.entity||entry.serial!=(h>>12)||entry.entity>0xffffffffu-4u){
  out.reason="serial-mismatch";return out;
 }
 std::uint32_t primary=0,secondary=0,col=0,slot=0,check=0;
 if(!source_entities::wire::load(reader,entry.entity,primary)||primary!=base+branch::PrimaryTableRva||
    !source_entities::wire::load(reader,entry.entity+4,secondary)||secondary!=base+branch::RenderableTableRva||
    !source_entities::wire::load(reader,secondary-4,col)||col!=base+branch::RenderableColRva||
    !source_entities::wire::load(reader,secondary+branch::DrawSlot*4,slot)||slot!=base+branch::EntryRva||
    !source_entities::wire::load(reader,primary+0x23cu,check)||
    !source_camera::owns(reinterpret_cast<void*>(std::uintptr_t(check)),source_camera::clientModule)||
    !source_camera::executable(reinterpret_cast<void*>(std::uintptr_t(check)))){
  out.reason="local-rtti-or-method-mismatch";return out;
 }
 out.entity=entry.entity;out.handle=h;out.reason="local-verified";
 out.origin=viewcompat::freshLocalOrigin(entry.entity);
 if(!out.origin&&f.tick-snapshot->tick<=75)out.origin=actor->networkOrigin;
 return out;
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
   source_camera::log("FPB_NATIVE_PROBE diagnostic build 2; checked-argument local field candidate; passive reads; post-render samples");
   dump(base,branch::LocalGateRva-96,96+sizeof(branch::GateBytes),"gate-context");
   dump(base,GetterRva,160,"getter");
  }
  const auto actor=verifiedActor(f,base);
  int raw=-1,displacement=-1;
  const char* reason="gate-signature-rejected";
  Getter getter{};
  std::array<std::uint8_t,sizeof(branch::GateBytes)> gate{};
  if(source_entities::reader.read(base+branch::LocalGateRva,gate.data(),gate.size())&&
      branch::matches(gate.data(),gate.size(),true)){
   displacement=gate[branch::DisplacementOffset];
   std::array<std::uint8_t,sizeof(CheckedGetter)> code{};
   std::array<std::uint8_t,sizeof(GateCallerPrefix)> caller{};
   reason="getter-unreadable";
   if(source_entities::reader.read(base+GetterRva,code.data(),code.size())){
    getter=decode(code.data(),code.size());reason="getter-signature-rejected";
    if(getter.kind==GetterKind::CheckedArgument){
     reason="caller-signature-rejected";
     constexpr std::uint8_t prologue[]={0x55,0x8b,0xec,0x56,0x57,0x6a,0xff,0x8b,0xf1,0xe8,0x92,0x00,0xe4,0xff};
     std::uint8_t actual[sizeof(prologue)]{};
     if(source_entities::reader.read(base+branch::LocalGateRva-96,caller.data(),caller.size())&&
        callerMatches(caller.data(),caller.size())&&
        source_entities::reader.read(base+branch::DrawRva,actual,sizeof(actual))&&
        std::memcmp(actual,prologue,sizeof(actual))==0){
      reason=actor.reason;
      std::uint32_t address=0;std::uint8_t value=0;
      if(actor.entity){
       reason="field-unreadable";
       if(fieldAddress(actor.entity,address)&&source_entities::reader.read(address,&value,sizeof(value))){
        raw=int(value);reason="sampled-local-unproven";
       }
      }
     }
    }
   }
  }
  const auto cfg=std::atomic_load(&options);
  const auto shoulder=viewcompat::readShoulder();
  const int sh=shoulder?(*shoulder?1:0):-1;
  const auto mode=viewcompat::mode.load();
  dango::CameraPose pose{};viewcompat::Observation geometry{};
  if(dango::cameraPose(f,pose))geometry=viewcompat::classify(pose,actor.origin,shoulder);
  const bool nativeOn=native::active.load(),local=validatedLocal.load();
  std::array<unsigned long,7> counters={effects::matched.load(),effects::dynamicApplied.load(),
   effects::dynamicRejected.load(),effects::headHidden.load(),effects::headRejected.load(),
   effects::armsHidden.load(),effects::armsRejected.load()};
  std::array<unsigned long,7> delta{};
  const bool newEpoch=!active||epoch!=f.epoch;
  if(!newEpoch)for(std::size_t i=0;i<delta.size();++i)delta[i]=counters[i]-previous[i];
  previous=counters;epoch=f.epoch;active=true;
  char key[384]{};
  std::snprintf(key,sizeof(key),"%u:%d:%d:%s:%s:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%u:%s",
    unsigned(epoch),raw,displacement,reason,viewcompat::label(mode),sh,int(cfg->enabled),
    int(cfg->thirdPersonCompatibility),int(cfg->dynamicAnchor),int(nativeOn),int(nativeBefore),int(local),
    int(freeRequested),int(worldBodyActive.load()),int(delta[0]!=0),int(delta[1]!=0),int(delta[2]!=0),
    int(delta[3]!=0),int(delta[4]!=0),int(delta[5]!=0),int(delta[6]!=0),unsigned(actor.handle),viewcompat::label(geometry.evidence));
  const bool heartbeat=newEpoch||f.tick<lastFlush||f.tick-lastFlush>=1000;
  if(heartbeat){flush();lastFlush=f.tick;}
  if(heartbeat||previousKey!=key){
   char line[1024]{};
   std::snprintf(line,sizeof(line),"FPB_NATIVE_PROBE sample frame=%llu epoch=%u sampleTick=%llu raw=%d originalWouldDraw=%d getter=%s reason=%s jz=%d fpb=%d compat=%d dynamic=%d head=%d arms=%d free=%d world=%d nativeBefore=%d nativeAfter=%d local=%d probeHandle=%u probeActor=%s view=%s shoulder=%d xy=%d eyeZ=%d sampleView=%s sampleXY=%.2f sampleEyeZ=%.2f deltaMatch=%lu deltaAligned=%lu deltaRejected=%lu deltaHead=%lu/%lu deltaArms=%lu/%lu totals=%lu/%lu/%lu/%lu/%lu/%lu/%lu",
    static_cast<unsigned long long>(source_camera::renderBegins.load()),unsigned(epoch),static_cast<unsigned long long>(f.tick),
    raw,raw<0?-1:int(raw!=0),label(getter.kind),reason,displacement,int(cfg->enabled),
    int(cfg->thirdPersonCompatibility),int(cfg->dynamicAnchor),int(cfg->hideHead),int(cfg->hideArms),
    int(freeRequested),int(worldBodyActive.load()),int(nativeBefore),int(nativeOn),int(local),unsigned(actor.handle),actor.reason,viewcompat::label(mode),sh,
    viewcompat::horizontalUnits.load(),viewcompat::eyeHeightUnits.load(),viewcompat::label(geometry.evidence),double(geometry.horizontal),double(geometry.eyeHeight),
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
