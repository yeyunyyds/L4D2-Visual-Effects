#pragma once
#include "first_person_view_policy.hpp"
#include "first_person_dynamic_alignment.hpp"
#include "first_person_body.hpp"
#include <cstdio>
#include <limits>
namespace visual::firstperson::distanceprobe {
struct Sample {
 bool valid=false,basis=false,eyeKnown=false,first=false,originKnown=false,headKnown=false,chestKnown=false;
 const char* stance="unknown";
 float headDistance=0;
 dango::Vec3 entityOrigin{},camera{};
 int flags=-1,shoulder=-1;
 float speed=-1.f,verticalSpeed=std::numeric_limits<float>::quiet_NaN();
 dango::Vec3 anchor{},chestDelta{},eyeDelta{},localCorrection{},front{},right{};
 viewcompat::Observation view{};
};
inline Sample measure(const StudioPose& studio,const dynamic::Rig& rig,const parts::Selection& plan,const Matrix3x4* bones,
 const dango::CameraPose& pose,std::uint32_t entity){
 Sample s{};
 if(!entity||!bones||!dynamic::valid(pose.eye))return s;
 s.camera=pose.eye;
 if(rig.anchor>=0&&unsigned(rig.anchor)<studio.count){
  s.anchor=dynamic::location(bones[unsigned(rig.anchor)]);
  s.chestDelta=pose.eye-s.anchor;s.chestKnown=dynamic::valid(s.chestDelta);
 }
 if(plan.head>=0&&unsigned(plan.head)<studio.count){
  auto delta=pose.eye-dynamic::location(bones[unsigned(plan.head)]);
  s.headKnown=dynamic::valid(delta);if(s.headKnown)s.headDistance=dango::length(delta);
 }
 s.valid=true;
 auto& reader=source_entities::reader;
 const int fo=flagsOffset.load(),oo=originOffset.load();
 std::uint32_t raw=0;
 if(fo>=0&&std::uint64_t(entity)+unsigned(fo)+4<=0x100000000ull&&
    source_entities::wire::load(reader,entity+unsigned(fo),raw)&&raw<=0x7fffffffu)s.flags=int(raw);
 s.stance=s.flags<0?"unknown":!(s.flags&1)?"airborne":s.flags&2?"crouching":"standing";
 const int vo=velocityOffset.load();dango::Vec3 velocity{};
 if(vo>=0&&std::uint64_t(entity)+unsigned(vo)+sizeof(velocity)<=0x100000000ull&&
    source_entities::wire::load(reader,entity+unsigned(vo),velocity)&&dynamic::valid(velocity)&&
    dango::length(velocity)<10000.f){
  s.speed=std::hypot(velocity.x,velocity.y);s.verticalSpeed=velocity.z;
 }
 std::optional<dango::Vec3> origin;
 dango::Vec3 v{};
 if(oo>=0&&std::uint64_t(entity)+unsigned(oo)+sizeof(v)<=0x100000000ull&&
    source_entities::wire::load(reader,entity+unsigned(oo),v)&&dynamic::valid(v)){origin=v;s.originKnown=true;s.entityOrigin=v;}
 s.shoulder=probeShoulder.load();
 std::optional<bool> shoulder;
 if(s.shoulder>=0)shoulder=s.shoulder!=0;
 s.view=viewcompat::classify(pose,origin,shoulder);
 // Diagnostic candidate label, not ground-truth view identity.
 s.first=s.view.evidence==viewcompat::Mode::First&&s.shoulder==0&&s.view.horizontal<=12.f;
 s.basis=dynamic::modelBasis(rig,bones,studio.count,s.front,s.right);
 if(s.chestKnown&&s.basis&&rig.knownEye){
  auto worldOffset=dynamic::bodySpace(s.front,s.right,rig.eyeFromAnchor.x,-rig.eyeFromAnchor.y,rig.eyeFromAnchor.z);
  s.eyeDelta=pose.eye-(s.anchor+worldOffset);
  s.eyeKnown=dynamic::valid(s.eyeDelta);
  if(s.eyeKnown)s.localCorrection=dango::Vec3{dynamic::dot(s.eyeDelta,s.front),dynamic::dot(s.eyeDelta,s.right),s.eyeDelta.z};
 }
 return s;
}
struct Recorder {
 std::uint64_t lastSample=0,lastFrame=0,flushAt=0;
 std::string batch;
 unsigned count=0;
 std::string lastPath,lastReason;
 std::uint32_t lastEpoch=0,lastHandle=0xffffffffu;
 void flush(){if(!batch.empty())source_camera::log(batch);batch.clear();count=0;}
 void record(const Sample& s,const StudioPose& studio,const viewpolicy::Decision& d,
  std::uint64_t tick,std::uint32_t epoch,std::uint32_t handle,bool compatibility,const char* path,bool haveApplied,dango::Vec3 applied,float yaw){
  const auto frame=source_camera::renderBegins.load();
  if(!s.valid)return;
  if(lastEpoch!=epoch||lastHandle!=handle||lastPath!=path||lastReason!=d.reason){
   char event[380]{};
   std::snprintf(event,sizeof(event),"FPB_VIEW tick=%llu compat=%d path=%s reason=%s stationary=%d originXY=%.3f originZ=%.3f eye=%.3f head=%.3f",
    static_cast<unsigned long long>(tick),int(compatibility),path,d.reason,int(d.stationary),
    double(s.view.horizontal),double(s.view.eyeHeight),double(s.eyeKnown?dango::length(s.eyeDelta):-1.f),double(s.headKnown?s.headDistance:-1.f));
   source_camera::log(event);lastEpoch=epoch;lastHandle=handle;lastPath=path;lastReason=d.reason;
  }
  if(frame==lastFrame||(lastSample&&tick>=lastSample&&tick-lastSample<50))return;
  lastFrame=frame;lastSample=tick;
  if(count>=20|| (flushAt&&tick>=flushAt&&tick-flushAt>=1000)){flush();flushAt=tick;}
  if(!flushAt)flushAt=tick;
  char line[1600]{};
  const auto nan=std::numeric_limits<float>::quiet_NaN();
  auto eye=s.eyeKnown?s.eyeDelta:dango::Vec3{nan,nan,nan};
  auto correction=s.eyeKnown?s.localCorrection:dango::Vec3{nan,nan,nan};
  auto actual=haveApplied?applied:dango::Vec3{nan,nan,nan};
  auto chest=s.chestKnown?s.chestDelta:dango::Vec3{nan,nan,nan};
  auto o=s.originKnown?s.entityOrigin:dango::Vec3{nan,nan,nan};
  std::snprintf(line,sizeof(line),"FPB_DISTANCE frame=%llu tick=%llu epoch=%u handle=%u model=%s checksum=%u mode=%s observed=%s firstCandidate=%d flags=%d stance=%s shoulder=%d speed=%.3f velocityZ=%.3f originXY=%.3f originZ=%.3f chest=%.3f chestXY=%.3f chestZ=%.3f eye=%.3f eyeXY=%.3f eyeZ=%.3f correction=%.3f/%.3f/%.3f source=%s compat=%d path=%s reason=%s stationary=%d detached=%d originMotion=%.3f relativeGrowth=%.3f origin=%.3f/%.3f/%.3f camera=%.3f/%.3f/%.3f head=%.3f actualWorld=%.3f/%.3f/%.3f actualYaw=%.5f",
   static_cast<unsigned long long>(frame),static_cast<unsigned long long>(tick),unsigned(epoch),unsigned(handle),
   studio.name.c_str(),unsigned(studio.checksum),viewcompat::label(viewcompat::mode.load()),viewcompat::label(s.view.evidence),
   int(s.first),s.flags,s.stance,s.shoulder,double(s.speed),double(s.verticalSpeed),double(s.view.horizontal),double(s.view.eyeHeight),
   double(dango::length(chest)),double(std::hypot(chest.x,chest.y)),double(chest.z),
   double(dango::length(eye)),double(std::hypot(eye.x,eye.y)),double(eye.z),double(correction.x),double(correction.y),double(correction.z),
   s.eyeKnown?"bind-estimate":"unavailable",int(compatibility),path,d.reason,int(d.stationary),int(d.detached),double(d.originMotion),double(d.relativeGrowth),double(o.x),double(o.y),double(o.z),double(s.camera.x),double(s.camera.y),double(s.camera.z),double(s.headKnown?s.headDistance:nan),double(actual.x),double(actual.y),double(actual.z),double(haveApplied?yaw:nan));
  if(!batch.empty())batch+='\n';batch+=line;++count;
 }
};
}
