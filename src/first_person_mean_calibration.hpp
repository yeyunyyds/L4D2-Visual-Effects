#pragma once
#include "first_person_dynamic_alignment.hpp"
#include <array>
namespace visual::firstperson::meanbody {
using dango::Vec3;
enum class Stance:int {Unknown=-1,Standing=0,Crouching=1,Airborne=2};
// Source FL_ONGROUND and FL_DUCKING; unknown if recvprop absent.
// Airborne has priority over ducking: log raw flags for crouch-jumps.
inline Stance stance(int flags) noexcept {
 if(flags<0)return Stance::Unknown;
 if(!(flags&1))return Stance::Airborne;
 return flags&2?Stance::Crouching:Stance::Standing;
}
inline const char* label(Stance s) noexcept {
 switch(s){case Stance::Standing:return "standing";case Stance::Crouching:return "crouching";case Stance::Airborne:return "airborne";default:return "unknown";}
}
inline Vec3 local(Vec3 delta,Vec3 forward,Vec3 right) noexcept {
 return {dynamic::dot(delta,forward),dynamic::dot(delta,right),delta.z};
}
// Draw-time cosmetics policy. Does not write the native gate.
struct DistanceGate {
 bool allowed=false,pending=false;
 std::uint64_t since=0,last=0;
 void reset(){allowed=pending=false;since=last=0;}
 bool update(float distance,bool known,int shoulder,bool fallbackFirst,std::uint64_t tick){
  if(tick<last||(last&&tick-last>500))reset();
  last=tick;
  if(shoulder==1||(known&&(!std::isfinite(distance)||distance>=28.f))){
   allowed=pending=false;return false;
  }
  // A rig without an eye estimate uses the existing conservative view evidence.
  if(!known){allowed=pending=false;return fallbackFirst;}
  if(allowed)return true;
  if(distance>24.f){pending=false;return false;}
  if(!pending){pending=true;since=tick;return false;}
  if(tick-since>=80){allowed=true;pending=false;}
  return allowed;
 }
};
struct TransformSample {Vec3 translation{};float yaw=0;};
inline TransformSample capture(Vec3 translation,float yaw,Vec3 forward,Vec3 right){
 return {local(translation,forward,right),yaw};
}
inline Vec3 replay(TransformSample sample,Vec3 forward,Vec3 right){
 return dynamic::bodySpace(forward,right,sample.translation.x,sample.translation.y,sample.translation.z);
}
struct Mean {
 static constexpr unsigned Required=30;
 unsigned count=0; Vec3 value{};float yaw=0;
 bool ready()const noexcept{return count>=Required;}
 bool add(TransformSample sample) noexcept {
  if(ready()||!dynamic::valid(sample.translation)||dango::length(sample.translation)>96.f||
     !std::isfinite(sample.yaw)||std::abs(sample.yaw)>.90f)return false;
  ++count;const float a=1.f/float(count);
  value=value+(sample.translation-value)*a;yaw+=(sample.yaw-yaw)*a;return true;
 }
 TransformSample result()const noexcept{return {value,yaw};}
};
struct Calibration {
 std::array<Mean,3> states{};
 std::uint32_t epoch=0,handle=0xffffffffu,checksum=0;
 std::string model;std::array<float,6> settings{};
 std::uint64_t lastSample=0;
 Stance stable=Stance::Unknown;std::uint64_t stableSince=0;
 void reset(){states={};lastSample=0;stable=Stance::Unknown;stableSince=0;}
 void identify(std::uint32_t e,std::uint32_t h,const std::string& m,std::uint32_t c,const std::array<float,6>& params){
  if(epoch!=e||handle!=h||model!=m||checksum!=c||settings!=params){
   reset();epoch=e;handle=h;model=m;checksum=c;settings=params;
  }
 }
 bool observe(Stance s,bool first,TransformSample sample,std::uint64_t tick){
  if(!first||s==Stance::Unknown){stable=Stance::Unknown;stableSince=0;return false;}
  if(s!=stable||tick<stableSince){stable=s;stableSince=tick;}
  // Exclude crouch/stand transitions; air samples span several jumps.
  if(s!=Stance::Airborne&&tick-stableSince<150)return false;
  if(lastSample&&tick>=lastSample&&tick-lastSample<50)return false;
  lastSample=tick;
  return states[unsigned(s)].add(sample);
 }
 const Mean* frozen(Stance s)const noexcept {
  if(s==Stance::Unknown)return nullptr;
  const auto& m=states[unsigned(s)];return m.ready()?&m:nullptr;
 }
};
}
