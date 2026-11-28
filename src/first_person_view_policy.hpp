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
 float originMotion=-1,relativeGrowth=0,originMotionXY=-1;
 const char* reason="unverified";
};
struct Gate {
 // Experimental values, in Source units and milliseconds. Relative to
 // live entity origin, independent of the animated model skeleton.
 static constexpr float EyeExit=28.f,EyeEnter=24.f,HeightExit=66.f,HeightEnter=64.5f;
 static constexpr float StillTolerance=.75f,HorizontalExit=4.f,HorizontalEnter=2.5f,Growth=1.5f;
 static constexpr float HeadEnter=10.f,HeadExit=12.f;
 static constexpr float MovingHeightLimit=70.f;
 static constexpr std::uint64_t MotionWindow=80,ResumeDelay=80;
 bool allowed=false,pending=false,detached=false,above=false,early=false;
 bool haveWindow=false,haveHead=false,approaching=false;
 std::uint64_t since=0,last=0,windowTick=0,headTick=0;
 Vec3 windowOrigin{},windowRelative{};
 float motion=-1,growth=0,priorHead=0,horizontalMotion=-1;
 bool stationary=false;
 void reset(){*this=Gate{};}
 Decision update(Input in){
  if(in.tick<last||(last&&in.tick-last>500))reset();
  const bool recent=last&&in.tick-last<=MotionWindow;
  last=in.tick;
  const bool origin=in.originKnown&&finite(in.origin)&&finite(in.camera);
  const bool eye=in.eyeKnown&&std::isfinite(in.eyeDistance)&&in.eyeDistance>=0;
  const bool head=in.headKnown&&std::isfinite(in.headDistance)&&in.headDistance>=0;
  Vec3 rel=origin?in.camera-in.origin:Vec3{};
  const float xy=std::hypot(rel.x,rel.y);
  bool retainHeight=false;
  float horizontalEvidence=-1;
  if(origin){
   // Keep height retention conservative when recent observations are missing.
   if(!recent)horizontalMotion=-1;
   if(!haveWindow){haveWindow=true;windowTick=in.tick;windowOrigin=in.origin;windowRelative=rel;}
   // Observe accumulated displacement, not consecutive equal samples.
   float partialXY=std::hypot(in.origin.x-windowOrigin.x,in.origin.y-windowOrigin.y);
   if(in.tick-windowTick>=MotionWindow){
    horizontalMotion=recent&&in.tick-windowTick<=2*MotionWindow?partialXY:-1.f;
    motion=dango::length(in.origin-windowOrigin);
    growth=xy-std::hypot(windowRelative.x,windowRelative.y);
    stationary=motion<=StillTolerance;
    if(stationary&&xy>=HorizontalExit&&growth>=Growth)detached=true;
    windowTick=in.tick;windowOrigin=in.origin;windowRelative=rel;partialXY=0.f;
   }
   if(xy<=HorizontalEnter)detached=false;
   // A fixed-position interaction cannot by itself produce horizontal
   // entity motion. Only exempt the height exit for an established first
   // person view, with positive motion and near-origin camera geometry.
   horizontalEvidence=recent&&in.tick-windowTick<MotionWindow?
       std::fmax(horizontalMotion,partialXY):horizontalMotion;
   if(!recent)horizontalEvidence=-1;
   retainHeight=allowed&&!above&&in.shoulder==0&&
       horizontalEvidence>StillTolerance&&xy<=HorizontalEnter&&
       rel.z>HeightExit&&rel.z<=MovingHeightLimit;
   if(rel.z>HeightExit&&!retainHeight)above=true;
   if(rel.z<=HeightEnter)above=false;
  }else{
   haveWindow=false;stationary=false;motion=-1;growth=0;horizontalMotion=-1;
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
  out.originMotionXY=horizontalEvidence;
  const char* block=nullptr;
  if(in.shoulder==1)block="shoulder";
  else if(above)block="origin-height";
  else if(detached)block="origin-static-xy";
  else if(eye&&in.eyeDistance>=EyeExit)block="eye-distance";
  else if(!eye&&!in.fallbackFirst)block="unverified";
  if(block){allowed=pending=false;out.reason=block;}
  else if(allowed){out.full=true;out.reason=retainHeight?"horizontal-retain":"first";}
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
// Keep a previously verified first-person presentation across a brief failed
// same-draw camera sample. Explicit third-person evidence, identity changes,
// stale frame snapshots and gaps beyond 48ms all invalidate this exemption.
struct CameraGapHold {
 static constexpr std::uint64_t MaxAgeMs=48;
 bool valid=false;
 std::uint64_t acceptedAt=0;
 std::uint32_t epoch=0,handle=0xffffffffu,checksum=0;
 void reset(){*this=CameraGapHold{};}
 void observe(bool safeFirst,std::uint32_t currentEpoch,std::uint32_t currentHandle,
              std::uint32_t currentChecksum,std::uint64_t now){
  if(!safeFirst){reset();return;}
  valid=true;epoch=currentEpoch;handle=currentHandle;checksum=currentChecksum;acceptedAt=now;
 }
 bool accept(bool ready,bool committedFirst,int shoulder,
             std::uint32_t currentEpoch,std::uint32_t currentHandle,std::uint32_t currentChecksum,
             std::uint64_t now,std::uint64_t cameraFrameTick) const {
  return valid&&ready&&committedFirst&&shoulder==0&&
    epoch==currentEpoch&&handle==currentHandle&&checksum==currentChecksum&&
    now>=acceptedAt&&now-acceptedAt<=MaxAgeMs&&
    now>=cameraFrameTick&&now-cameraFrameTick<=MaxAgeMs;
 }
};
// Experimental pass isolation for FPB: a secondary camera can be sampled by
// Studio while the main view is being rendered. A recent verified centered
// first-person sample anchors the main view; a clearly off-axis sample from
// the same entity/model is excluded from FPB cosmetics only. The draw itself
// continues with its original bones (shadows and other passes stay intact).
struct MainViewPassFilter {
 static constexpr std::uint64_t MaxReferenceAgeMs=100;
 bool valid=false;
 std::uint32_t epoch=0,handle=0xffffffffu,checksum=0;
 std::uint64_t observedAt=0;
 Vec3 lastEye{};
 float lastRelativeZ=0.f;
 void reset(){*this=MainViewPassFilter{};}
 bool sameIdentity(std::uint32_t e,std::uint32_t h,std::uint32_t c) const{
  return valid&&epoch==e&&handle==h&&checksum==c;
 }
 void observe(bool committedFirst,int shoulder,bool originKnown,
              float originXY,float originZ,Vec3 camera,
              std::uint32_t e,std::uint32_t h,std::uint32_t c,
              std::uint64_t now){
  if(!committedFirst||shoulder!=0||!originKnown||!finite(camera)||
     !std::isfinite(originXY)||!std::isfinite(originZ)||originXY>2.5f||
     originZ<30.f||originZ>88.f)return;
  valid=true;epoch=e;handle=h;checksum=c;observedAt=now;lastEye=camera;lastRelativeZ=originZ;
 }
 bool isSecondary(bool committedFirst,int shoulder,bool originKnown,
                  float originXY,float originZ,Vec3 camera,
                  std::uint32_t e,std::uint32_t h,std::uint32_t c,
                  std::uint64_t now,int playerFlags=-1) const{
  if(!committedFirst||shoulder!=0||!originKnown||!finite(camera)||
     !std::isfinite(originXY)||!std::isfinite(originZ)||
     !sameIdentity(e,h,c)||now<observedAt||
     now-observedAt>MaxReferenceAgeMs)return false;
  // Preserve the existing large-horizontal-offset exclusion.
  if(originXY>=14.f&&originZ>=30.f&&originZ<=88.f&&
     dango::length(camera-lastEye)>=12.f)return true;
  // Logs: a centered main eye at Z~62 and a separate lower eye
  // at XY~3/Z~44 both classified as first-person. Only a grounded,
  // non-ducking near-offset eye can be filtered by the vertical rule.
  // Genuine jumps/crouches must not be rejected by height alone.
  const bool groundedStanding=playerFlags>=0&&(playerFlags&1)!=0&&
                              (playerFlags&2)==0;
  return groundedStanding&&originXY>=2.5f&&originXY<14.f&&
         originZ>=30.f&&originZ<=88.f&&
         lastRelativeZ-originZ>=12.f&&lastEye.z-camera.z>=12.f;
 }
};
}
