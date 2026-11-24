#pragma once
// Independent model-aware first-person body placement for native Studio draws.
// Independently developed: validated camera/model data and per-draw bone copies.
#include "first_person_body_config.hpp"
#include "first_person_body_parts.hpp"
#include "dango/world_camera.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
namespace visual::firstperson::dynamic {
using dango::Vec3;
inline bool valid(Vec3 a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 location(const Matrix3x4& m){return {m.m[0][3],m.m[1][3],m.m[2][3]};}
inline bool unitFrame(const dango::CameraPose& pose,Vec3& front,Vec3& right) {
 front=dango::normalize(Vec3{pose.forward.x,pose.forward.y,0.f});
 right=dango::normalize(Vec3{pose.right.x,pose.right.y,0.f});
 if(dango::length(front)<.15f && dango::length(right)>.95f)
  front=dango::normalize(cross(Vec3{0,0,1},right));
 if(dango::length(right)<.15f && dango::length(front)>.95f)
  right=dango::normalize(cross(front,Vec3{0,0,1}));
 return dango::length(front)>.9f&&dango::length(right)>.9f&&std::abs(dot(front,right))<.1f;
}
inline Vec3 bodySpace(Vec3 forward,Vec3 right,float f,float r,float up){
 return forward*f+right*r+Vec3{0,0,up};
}
inline float planar(Vec3 a,Vec3 b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));}
struct Rig {
 int anchor=-1,head=-1,neck=-1,left=-1,right=-1;
 bool knownEye=false,bindEye=false;
 Vec3 eyeFromAnchor{};const char* source="unavailable";
};
inline bool bind(const StudioPose& studio,int index,Vec3& out){
 if(index<0||unsigned(index)>=studio.count||studio.bindValid.size()!=studio.count||
    studio.bindLocal.size()!=studio.count||!studio.bindValid[unsigned(index)])return false;
 const auto& p=studio.bindLocal[unsigned(index)];
 out={p[0],p[1],p[2]};return valid(out);
}
inline Rig discover(const StudioPose& studio,float fallbackHeight,const parts::Selection& selected){
 Rig rig{};
 rig.neck=selected.neck;rig.head=selected.head;
 if(studio.count==0||studio.parents.size()!=studio.count||studio.boneNames.size()!=studio.count)return rig;
 // Prefer a stable upper-spine/chest ancestor; the head and neck animate independently.
 if(rig.neck>=0){
  for(int n=studio.parents[unsigned(rig.neck)],depth=0;n>=0&&depth<int(studio.count);
      n=studio.parents[unsigned(n)],++depth){
   if(unsigned(n)>=studio.count)break;
   const auto& name=studio.boneNames[unsigned(n)];
   if(name.find("spine")!=std::string::npos||name.find("chest")!=std::string::npos){rig.anchor=n;break;}
  }
  if(rig.anchor<0)rig.anchor=rig.neck;
 }
 // Some mod rigs have a spine but lack a conventional head bone.
 if(rig.anchor<0)for(unsigned i=0;i<studio.count;++i) {
  const auto& name=studio.boneNames[i];
  if(name.find("upperchest")!=std::string::npos||name.find("spine3")!=std::string::npos||
     name.find("spine4")!=std::string::npos){rig.anchor=int(i);break;}
 }
 if(rig.anchor<0)return rig;
 for(unsigned i=0;i<studio.count;++i){
  const auto& n=studio.boneNames[i];
  if((n.find("clavicle")!=std::string::npos||n.find("shoulder")!=std::string::npos)&&parts::side(n)==1)rig.left=int(i);
  if((n.find("clavicle")!=std::string::npos||n.find("shoulder")!=std::string::npos)&&parts::side(n)==2)rig.right=int(i);
 }
 Vec3 base{},head{};
 if(!bind(studio,rig.anchor,base))return rig;
 if(studio.eyeValid){
  Vec3 eye{studio.eyeLocal[0],studio.eyeLocal[1],studio.eyeLocal[2]};
  if(bind(studio,rig.head,head) &&
     dango::length(eye-head)<28.f && eye.z-base.z>2.f&&eye.z-base.z<65.f){
   rig.eyeFromAnchor=eye-base;rig.knownEye=true;rig.bindEye=true;rig.source="mdl-eyeposition";return rig;
  }
 }
 if(bind(studio,rig.head,head)){
  rig.eyeFromAnchor=head-base+Vec3{0,0,fallbackHeight};
  rig.knownEye=true;rig.source="inverse-bind-head-fallback";return rig;
 }
 Vec3 neck{};
 if(bind(studio,rig.neck,neck)){
  rig.eyeFromAnchor=neck-base+Vec3{0,0,fallbackHeight};
  rig.knownEye=true;rig.source="inverse-bind-neck-fallback";return rig;
 }
 rig.source="live-pose-bootstrap";
 return rig;
}
inline Rig discover(const StudioPose& studio,float fallbackHeight){
 return discover(studio,fallbackHeight,parts::discover(studio));
}
// Lives inside the validated StudioCache entry, never keyed by a raw pose pointer.
struct CachedModel {
 std::string name,key;
 parts::Selection selection;
 Rig rig{};
 bool named=false,partsReady=false,rigReady=false;
 float fallback=0;
 void identify(const StudioPose& studio){
  if(named)return;
  name=canonicalModel(studio.name);key=name+"#"+std::to_string(studio.checksum);named=true;
 }
 const parts::Selection& partPlan(const StudioPose& studio){
  if(!partsReady){selection=parts::discover(studio);partsReady=true;}
  return selection;
 }
 const Rig& alignment(const StudioPose& studio,float height){
  if(!rigReady||fallback!=height){rig=discover(studio,height,partPlan(studio));fallback=height;rigReady=true;}
  return rig;
 }
};
inline bool modelBasis(const Rig& rig,const Matrix3x4* bones,unsigned count,Vec3& front,Vec3& right){
 if(!bones||rig.left<0||rig.right<0||unsigned(rig.left)>=count||unsigned(rig.right)>=count)return false;
 auto left=location(bones[unsigned(rig.left)]),r=location(bones[unsigned(rig.right)]);
 auto v=r-left;
 v.z=0;float width=dango::length(v);
 if(!std::isfinite(width)||width<4.f||width>70.f)return false;
 right=v*(1.f/width);
 front=dango::normalize(cross(Vec3{0,0,1},right));
 return dango::length(front)>.9f;
}
inline float angle(Vec3 from,Vec3 to){
 return std::atan2(from.x*to.y-from.y*to.x,dot(from,to));
}
struct Track {
 std::uint32_t epoch=0,handle=0xffffffffu;
 std::string model;
 std::uint64_t tick=0;
 bool active=false,bootstrapped=false;
 Vec3 lastEye{},filtered{},bootstrap{};
 void reset(){active=false;bootstrapped=false;tick=0;model.clear();filtered={};}
};
struct Result {bool ok=false,usedBootstrap=false;Vec3 translation{};float yawRadians=0;const char* source="unavailable";};
inline Result compute(const StudioPose& studio,const Rig& rig,const Matrix3x4* bones,
  const dango::CameraPose& camera,const Settings& config,const ModelProfile* model,
  std::uint32_t epoch,std::uint32_t handle,std::uint64_t tick,Track& track,
  const std::string* cachedKey=nullptr){
 Result out{};out.source=rig.source;
 if(!config.dynamicAnchor||!bones||rig.anchor<0||unsigned(rig.anchor)>=studio.count||!valid(camera.eye))return out;
 Vec3 cameraFront{},cameraRight{};
 if(!unitFrame(camera,cameraFront,cameraRight))return out;
 const auto anchor=location(bones[unsigned(rig.anchor)]);
 if(!valid(anchor))return out;
 Vec3 bodyFront=cameraFront,bodyRight=cameraRight;
 bool haveBody=modelBasis(rig,bones,studio.count,bodyFront,bodyRight);
 // Treat model swap/respawn/map/large camera discontinuity as a new calibration.
 std::string uncachedKey;
 if(!cachedKey)uncachedKey=canonicalModel(studio.name)+"#"+std::to_string(studio.checksum);
 const auto& key=cachedKey?*cachedKey:uncachedKey;
 const bool reset=!track.active||track.model!=key||track.epoch!=epoch||track.handle!=handle||
     tick<track.tick||tick-track.tick>500||dango::length(camera.eye-track.lastEye)>128.f;
 if(reset){
  track.reset();track.model=key;track.epoch=epoch;track.handle=handle;
  track.active=true;track.lastEye=camera.eye;track.tick=tick;
 }
 float radians=0;
 if(config.followCameraYaw&&haveBody){
  radians=std::clamp(angle(bodyFront,cameraFront),-.78f,.78f);
 }
 const float yawDeg=config.yawTrim+(model?model->yaw:0.f);
 radians=std::clamp(radians+yawDeg*0.01745329251994329577f,-.90f,.90f);
 // Body rotation is about this chest/neck anchor; translational anchor stays put.
 out.yawRadians=radians;
 Vec3 forward=haveBody?bodyFront:cameraFront;
 Vec3 right=haveBody?bodyRight:cameraRight;
 // If yaw correction is applied, rotate the body basis accordingly.
 if(haveBody&&radians!=0){
  float cs=std::cos(radians),sn=std::sin(radians);
  forward={forward.x*cs-forward.y*sn,forward.x*sn+forward.y*cs,0};
  right={right.x*cs-right.y*sn,right.x*sn+right.y*cs,0};
 }
 Vec3 relative=rig.eyeFromAnchor;
 if(!rig.knownEye){
  if(!track.bootstrapped){
   auto baseline=camera.eye-anchor;
   track.bootstrap={dot(baseline,forward),-dot(baseline,right),baseline.z};
   track.bootstrapped=true;
  }
  relative=track.bootstrap;out.usedBootstrap=true;
 }
 // Model bind coordinates: +Y is left; our user offsets have +right.
 const Vec3 worldEyeOffset=bodySpace(forward,right,relative.x,-relative.y,relative.z);
 const float userF=config.forward+(model?model->forward:0.f);
 const float userR=config.right+(model?model->right:0.f);
 const float userZ=config.up+(model?model->up:0.f);
 const auto targetAnchor=camera.eye-worldEyeOffset+bodySpace(cameraFront,cameraRight,userF,userR,userZ);
 const auto raw=targetAnchor-anchor;
 if(!valid(raw)||dango::length(raw)>96.f)return out;
 const std::uint64_t dt=tick>=track.tick?tick-track.tick:0;
 if(reset)track.filtered=raw;
 else if(dt>0){
  // Horizontal-only damping avoids jitter; jump/landing Z must never lag.
  float alpha=1.f-std::exp(-float(std::min<std::uint64_t>(dt,100))/18.f);
  track.filtered.x+=(raw.x-track.filtered.x)*alpha;
  track.filtered.y+=(raw.y-track.filtered.y)*alpha;
 }
 track.filtered.z=raw.z;
 track.tick=tick;track.lastEye=camera.eye;
 out.ok=valid(track.filtered);
 out.translation=track.filtered;
 return out;
}
// The original matrices remain untouched. Only render-local copies are altered.
inline bool transform(Matrix3x4* bones,unsigned count,Vec3 pivot,Vec3 translate,float yawRadians){
 if(!bones||!valid(pivot)||!valid(translate)||!std::isfinite(yawRadians)||std::abs(yawRadians)>.91f)return false;
 for(unsigned i=0;i<count;++i)if(!parts::matrixValid(bones[i]))return false;
 const float cs=std::cos(yawRadians),sn=std::sin(yawRadians);
 for(unsigned i=0;i<count;++i){
  auto& m=bones[i];
  for(unsigned col=0;col<3;++col){
   float x=m.m[0][col],y=m.m[1][col];
   m.m[0][col]=cs*x-sn*y;m.m[1][col]=sn*x+cs*y;
  }
  const float x=m.m[0][3]-pivot.x,y=m.m[1][3]-pivot.y;
  m.m[0][3]=pivot.x+cs*x-sn*y+translate.x;
  m.m[1][3]=pivot.y+sn*x+cs*y+translate.y;
  m.m[2][3]+=translate.z;
 }
 return true;
}
}
