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
struct Mean {
 static constexpr unsigned Required=30;
 unsigned count=0; Vec3 value{};
 bool ready()const noexcept{return count>=Required;}
 bool add(Vec3 sample) noexcept {
  if(ready()||!dynamic::valid(sample)||dango::length(sample)>96.f)return false;
  ++count;value=value+(sample-value)*(1.f/float(count));return true;
 }
};
struct Calibration {
 std::array<Mean,3> states{};
 std::uint32_t epoch=0,handle=0xffffffffu,checksum=0;
 std::string model;float fallback=0;bool selected=false;
 std::uint64_t lastSample=0;
 Stance stable=Stance::Unknown;std::uint64_t stableSince=0;
 void reset(){states={};lastSample=0;stable=Stance::Unknown;stableSince=0;}
 void identify(std::uint32_t e,std::uint32_t h,const std::string& m,std::uint32_t c,float height,bool enabled){
  if(epoch!=e||handle!=h||model!=m||checksum!=c||fallback!=height||selected!=enabled){
   reset();epoch=e;handle=h;model=m;checksum=c;fallback=height;selected=enabled;
  }
 }
 bool observe(Stance s,bool first,Vec3 sample,std::uint64_t tick){
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
