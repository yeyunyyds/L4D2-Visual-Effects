#pragma once
// Pure, draw-time FPB cosmetic policy. Never changes the native DrawModel gate.
#include "dango/world_camera.hpp"
#include <cmath>
#include <cstdint>
namespace visual::firstperson::viewpolicy {
using dango::Vec3;
inline bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct Input {
 bool originKnown=false,eyeKnown=false,headKnown=false,fallbackFirst=false;
 Vec3 origin{},camera{};
 float eyeDistance=0,headDistance=0;
 int shoulder=-1;
 std::uint64_t tick=0;
};
struct Decision {
 bool full=false,earlyHead=false,stationary=false,detached=false;
 float originMotion=-1,relativeGrowth=0;
 const char* reason="unverified";
};
struct Gate {
 // Experimental values, in Source units and milliseconds. Relative to
 // live entity origin, independent of the animated model skeleton.
 static constexpr float EyeExit=28.f,EyeEnter=24.f,HeightExit=66.f,HeightEnter=64.5f;
 static constexpr float StillTolerance=.75f,HorizontalExit=4.f,HorizontalEnter=2.5f,Growth=1.5f;
 static constexpr float HeadEnter=10.f,HeadExit=12.f;
 // Only retain an already-confirmed first-person view during a short
 // landing overshoot. Do not widen normal third-person entry thresholds.
 static constexpr float LandingHeight=70.f,LandingEye=12.f,LandingHead=12.f;
 static constexpr float LandingDrop=.5f,LandingRiseTolerance=.5f;
 static constexpr std::uint64_t MotionWindow=80,ResumeDelay=80,LandingHold=32;
 bool allowed=false,pending=false,detached=false,above=false,early=false;
 bool haveWindow=false,haveHead=false,approaching=false;
 std::uint64_t since=0,last=0,windowTick=0,headTick=0;
 Vec3 windowOrigin{},windowRelative{};
 float motion=-1,growth=0,priorHead=0;
 bool stationary=false;
 bool haveLive=false,landingEvidence=false;
 std::uint64_t liveTick=0,landingTick=0;
 Vec3 liveOrigin{},liveCamera{};
 float landingOriginZ=0,landingCameraZ=0;
 void reset(){*this=Gate{};}
 Decision update(Input in){
  if(in.tick<last||(last&&in.tick-last>500))reset();
  last=in.tick;
  const bool origin=in.originKnown&&finite(in.origin)&&finite(in.camera);
  const bool eye=in.eyeKnown&&std::isfinite(in.eyeDistance)&&in.eyeDistance>=0;
  const bool head=in.headKnown&&std::isfinite(in.headDistance)&&in.headDistance>=0;
  Vec3 rel=origin?in.camera-in.origin:Vec3{};
  const float xy=std::hypot(rel.x,rel.y);
  bool retainLanding=false;
  if(origin){
   // A falling entity can briefly outrun the interpolated camera on landing.
   // Use fresh vertical motion rather than the potentially stale 80 ms
   // movement window, and reject any independent upward camera motion.
   if(haveLive&&in.tick-liveTick<=MotionWindow){
    const float dz=in.origin.z-liveOrigin.z;
    const float cz=in.camera.z-liveCamera.z;
    if(dz<=-LandingDrop&&cz<=0.f&&cz-dz>0.f){
     landingEvidence=true;landingTick=in.tick;
     landingOriginZ=in.origin.z;landingCameraZ=in.camera.z;
    }else if(landingEvidence&&(in.tick-landingTick>LandingHold||
       in.origin.z>landingOriginZ+LandingRiseTolerance||
       in.camera.z>landingCameraZ+LandingRiseTolerance)){
     landingEvidence=false;
    }
   }else landingEvidence=false;
   haveLive=true;liveTick=in.tick;liveOrigin=in.origin;liveCamera=in.camera;
   if(!haveWindow){haveWindow=true;windowTick=in.tick;windowOrigin=in.origin;windowRelative=rel;}
   // Observe accumulated displacement, not consecutive equal samples.
   if(in.tick-windowTick>=MotionWindow){
    motion=dango::length(in.origin-windowOrigin);
    growth=xy-std::hypot(windowRelative.x,windowRelative.y);
    stationary=motion<=StillTolerance;
    if(stationary&&xy>=HorizontalExit&&growth>=Growth)detached=true;
    windowTick=in.tick;windowOrigin=in.origin;windowRelative=rel;
   }
   if(xy<=HorizontalEnter)detached=false;
   retainLanding=allowed&&!above&&in.shoulder==0&&landingEvidence&&
       !detached&&xy<=HorizontalEnter&&rel.z>HeightExit&&rel.z<=LandingHeight&&
       eye&&in.eyeDistance<=LandingEye&&head&&in.headDistance<=LandingHead;
   if(rel.z>HeightExit&&!retainLanding)above=true;
   if(rel.z<=HeightEnter)above=false;
  }else{
   haveWindow=false;stationary=false;motion=-1;growth=0;
   haveLive=landingEvidence=false;
   // Do not clear positive evidence just because a read failed.
  }
  // Head proximity is independent of the full-body resume delay. A returning
  // camera can hide the head even while the shoulder CVar is stale.
  if(head){
   if(!haveHead){haveHead=true;headTick=in.tick;priorHead=in.headDistance;}
   else if(in.tick-headTick>=16){
    if(priorHead-in.headDistance>=.15f)approaching=true;
    else if(in.headDistance-priorHead>=.15f)approaching=false;
    headTick=in.tick;priorHead=in.headDistance;
   }
   if(in.headDistance>HeadExit)early=false;
  }else{haveHead=false;approaching=false;early=false;}
  Decision out{};
  out.stationary=stationary;out.originMotion=motion;out.relativeGrowth=growth;out.detached=detached;
  const char* block=nullptr;
  if(in.shoulder==1)block="shoulder";
  else if(above)block="origin-height";
  else if(detached)block="origin-static-xy";
  else if(eye&&in.eyeDistance>=EyeExit)block="eye-distance";
  else if(!eye&&!in.fallbackFirst)block="unverified";
  if(block){allowed=pending=landingEvidence=false;out.reason=block;}
  else if(allowed){out.full=true;out.reason=retainLanding?"landing-retain":"first";}
  else if((eye&&in.eyeDistance>EyeEnter)||(origin&&rel.z>HeightEnter)){pending=false;out.reason="hysteresis";}
  else{
   if(!pending){pending=true;since=in.tick;}
   if(in.tick-since>=ResumeDelay){allowed=true;pending=false;out.full=true;out.reason="first";}
   else out.reason="settling";
  }
  // Require near-origin geometry when it is available. During outgoing
  // detachment, do not hide the head merely because it was nearby.
  if(!out.full&&head&&(!origin||(xy<=12.f&&rel.z<=HeightExit))){
   if(in.headDistance<=HeadEnter&&(approaching||pending))early=true;
   if(early&&in.headDistance<=HeadExit)out.earlyHead=true;
  }else early=false;
  if(out.full)approaching=false;
  return out;
 }
};
}
