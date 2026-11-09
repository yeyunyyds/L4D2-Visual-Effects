#pragma once
#include "rock_prediction.hpp"
#include "dango/source_trace_contract.hpp"
namespace visual::rocks {
enum class LandingState {Unavailable,NoHit,Impact,StartSolid,BudgetExhausted,Invalid};
struct Landing {LandingState state=LandingState::Unavailable;Vec3 position{},centerPosition{},normal{};float assumedHalfSize=0;float seconds=0;uint64_t sampleMs=0,computedMs=0;bool valid()const{return state==LandingState::Impact;}};
// Cached impact time belongs to its original motion sample. Rebase it before
// clipping a trajectory from a newer sample, never bend a line back to old time.
inline std::optional<float> landingSecondsFor(const Landing& hit,const Sample& current,uint64_t now){
 if(!hit.valid()||now<hit.computedMs||now-hit.computedMs>SampleLifetimeMs||now<current.wallMs||now-current.wallMs>SampleLifetimeMs||current.wallMs<hit.sampleMs||now<hit.sampleMs||now-hit.sampleMs>SampleLifetimeMs||!std::isfinite(hit.seconds))return {};
 float remaining=hit.seconds-float(current.wallMs-hit.sampleMs)*.001f;
 return remaining>0?std::optional<float>(remaining):std::nullopt;
}
inline Vec3 ballistic(Vec3 p,Vec3 v,float g,float t){return p+v*t+Vec3{0,0,-g}*(.5f*t*t);}
template<class Cast> Landing predictLanding(const Sample& sample,float gravity,uint64_t now,unsigned& budget,Cast cast,float assumedHalfSize=0){
 Landing out;out.assumedHalfSize=assumedHalfSize;out.sampleMs=sample.wallMs;out.computedMs=now;
 if(!std::isfinite(assumedHalfSize)||assumedHalfSize<0||assumedHalfSize>64||(assumedHalfSize>0&&assumedHalfSize<4)){out.state=LandingState::Invalid;return out;}
 if(!sample.velocityValid||!valid(sample.position)||!valid(sample.velocity,20000)||!std::isfinite(gravity)||gravity<0||gravity>4000){out.state=LandingState::Invalid;return out;}
 Vec3 start=sample.position;
 for(unsigned i=1;i<=60;++i){if(!budget){out.state=LandingState::BudgetExhausted;return out;}--budget;float t=i*.05f;Vec3 end=ballistic(sample.position,sample.velocity,gravity,t);if(!valid(end)){out.state=LandingState::Invalid;return out;}dango::trace::Result hit;
  if(!cast(start,end,hit)){out.state=LandingState::Unavailable;return out;}
  if(!dango::trace::validSweep(hit,start,end,{assumedHalfSize,assumedHalfSize,assumedHalfSize})){out.state=LandingState::Invalid;return out;}
  if(hit.startSolid||hit.allSolid){out.state=LandingState::StartSolid;return out;}
  if(hit.fraction<1){if(!dango::trace::wallHit(hit)){out.state=LandingState::Invalid;return out;}out.state=LandingState::Impact;out.centerPosition=hit.end;out.position=hit.end-dango::trace::boxSupport(hit.plane.normal,{assumedHalfSize,assumedHalfSize,assumedHalfSize});out.normal=hit.plane.normal;out.seconds=(i-1+hit.fraction)*.05f;return out;}
  start=end;
 }
 out.state=LandingState::NoHit;return out;
}
inline std::array<Vec3,32> landingDisk(Vec3 center,Vec3 normal,float radius=32){
 normal=dango::normalize(normal);Vec3 reference=std::abs(normal.z)>.9f?Vec3{1,0,0}:Vec3{0,0,1};
 auto cross=[](Vec3 a,Vec3 b){return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
 auto x=dango::normalize(cross(normal,reference)),y=cross(normal,x);std::array<Vec3,32> ring{};
 for(unsigned i=0;i<ring.size();++i){float angle=6.28318530718f*i/ring.size();ring[i]=center+normal*1.f+x*(radius*std::cos(angle))+y*(radius*std::sin(angle));}return ring;
}
}
