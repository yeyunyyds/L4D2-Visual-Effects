#pragma once
#include "first_person_mean_calibration.hpp"
#include "first_person_body.hpp"
#include <cstdio>
#include <limits>
namespace visual::firstperson::distanceprobe {
struct Sample {
 bool valid=false,basis=false,eyeKnown=false,first=false;
 meanbody::Stance stance=meanbody::Stance::Unknown;
 int flags=-1,shoulder=-1;
 dango::Vec3 anchor{},chestDelta{},eyeDelta{},localCorrection{},front{},right{};
 viewcompat::Observation view{};
};
inline Sample measure(const StudioPose& studio,const dynamic::Rig& rig,const Matrix3x4* bones,
 const dango::CameraPose& pose,std::uint32_t entity){
 Sample s{};
 if(!entity||!bones||rig.anchor<0||unsigned(rig.anchor)>=studio.count)return s;
 s.anchor=dynamic::location(bones[unsigned(rig.anchor)]);
 s.chestDelta=pose.eye-s.anchor;
 if(!dynamic::valid(s.chestDelta))return s;
 s.valid=true;
 auto& reader=source_entities::reader;
 const int fo=flagsOffset.load(),oo=originOffset.load();
 std::uint32_t raw=0;
 if(fo>=0&&std::uint64_t(entity)+unsigned(fo)+4<=0x100000000ull&&
    source_entities::wire::load(reader,entity+unsigned(fo),raw)&&raw<=0x7fffffffu)s.flags=int(raw);
 s.stance=meanbody::stance(s.flags);
 std::optional<dango::Vec3> origin;
 dango::Vec3 v{};
 if(oo>=0&&std::uint64_t(entity)+unsigned(oo)+sizeof(v)<=0x100000000ull&&
    source_entities::wire::load(reader,entity+unsigned(oo),v)&&dynamic::valid(v))origin=v;
 s.shoulder=probeShoulder.load();
 std::optional<bool> shoulder;
 if(s.shoulder>=0)shoulder=s.shoulder!=0;
 s.view=viewcompat::classify(pose,origin,shoulder);
 // Conservative training label, not ground-truth view identity.
 s.first=s.view.evidence==viewcompat::Mode::First&&s.shoulder==0&&s.view.horizontal<=12.f;
 s.basis=dynamic::modelBasis(rig,bones,studio.count,s.front,s.right);
 if(s.basis&&rig.knownEye){
  auto worldOffset=dynamic::bodySpace(s.front,s.right,rig.eyeFromAnchor.x,-rig.eyeFromAnchor.y,rig.eyeFromAnchor.z);
  s.eyeDelta=pose.eye-(s.anchor+worldOffset);
  s.eyeKnown=dynamic::valid(s.eyeDelta);
  if(s.eyeKnown)s.localCorrection=meanbody::local(s.eyeDelta,s.front,s.right);
 }
 return s;
}
struct Recorder {
 std::uint64_t lastSample=0,lastFrame=0,flushAt=0;
 std::string batch;
 unsigned count=0;
 void flush(){if(!batch.empty())source_camera::log(batch);batch.clear();count=0;}
 void record(const Sample& s,const StudioPose& studio,const meanbody::Calibration& c,
  std::uint64_t tick,std::uint32_t epoch,std::uint32_t handle,bool dynamic2,bool learned){
  const auto frame=source_camera::renderBegins.load();
  if(!s.valid||frame==lastFrame||(lastSample&&tick>=lastSample&&tick-lastSample<50))return;
  lastFrame=frame;lastSample=tick;
  if(count>=20|| (flushAt&&tick>=flushAt&&tick-flushAt>=1000)){flush();flushAt=tick;}
  if(!flushAt)flushAt=tick;
  char line[1000]{};
  const auto nan=std::numeric_limits<float>::quiet_NaN();
  auto eye=s.eyeKnown?s.eyeDelta:dango::Vec3{nan,nan,nan};
  auto correction=s.eyeKnown?s.localCorrection:dango::Vec3{nan,nan,nan};
  std::snprintf(line,sizeof(line),"FPB_DISTANCE frame=%llu tick=%llu epoch=%u handle=%u model=%s checksum=%u mode=%s observed=%s firstCandidate=%d flags=%d stance=%s shoulder=%d originXY=%.3f originZ=%.3f chest=%.3f chestXY=%.3f chestZ=%.3f eye=%.3f eyeXY=%.3f eyeZ=%.3f correction=%.3f/%.3f/%.3f source=%s dynamic2=%d learned=%d samples=%u/%u/%u",
   static_cast<unsigned long long>(frame),static_cast<unsigned long long>(tick),unsigned(epoch),unsigned(handle),
   studio.name.c_str(),unsigned(studio.checksum),viewcompat::label(viewcompat::mode.load()),viewcompat::label(s.view.evidence),
   int(s.first),s.flags,meanbody::label(s.stance),s.shoulder,double(s.view.horizontal),double(s.view.eyeHeight),
   double(dango::length(s.chestDelta)),double(std::hypot(s.chestDelta.x,s.chestDelta.y)),double(s.chestDelta.z),
   double(dango::length(eye)),double(std::hypot(eye.x,eye.y)),double(eye.z),double(correction.x),double(correction.y),double(correction.z),
   s.eyeKnown?"bind-estimate":"unavailable",int(dynamic2),int(learned),c.states[0].count,c.states[1].count,c.states[2].count);
  if(!batch.empty())batch+='\n';batch+=line;++count;
 }
};
}
