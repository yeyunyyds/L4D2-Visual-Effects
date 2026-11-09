#pragma once
#include "dango/entity_follow.hpp"
#include "target_identity.hpp"
#include "special_infected.hpp"
#include "entity_categories.hpp"
#include <limits>
#include <set>
namespace visual {
struct AimSelection {
 int classId=-1;std::uint32_t handle{};std::string name;float score=0;
 dango::inventory::Record record{};std::uint32_t epoch=0;bool approximate=false,attachment=false;
};
inline dango::inventory::Record aimRecord(const AimSelection& a){
 if(a.record.classId>=0)return a.record;
 dango::inventory::Record r;r.classId=a.classId;r.handle=a.handle;r.className=a.name;return r;
}
inline bool actorIdentityKnown(const dango::inventory::Record& r){
 if(!survivorPlayerClass(r.className))return true;
 return specialCharacter(r)>=0||survivorCharacter(r)>=0;
}
inline std::string aimLabel(const AimSelection& a){
 auto r=aimRecord(a);int s=specialCharacter(r),v=survivorCharacter(r);
 return s>=0?SpecialNames[s]:v>=0?SurvivorNames[v]:a.name;
}
struct AimQuery {
 std::uint32_t localHandle=0xffffffffu;bool handsOnly=false,blocked=false;
 std::optional<std::uint32_t> hitHandle;float limit=1200;
};
struct AimResult {std::uint64_t id=0,tick=0;std::uint32_t epoch=0;std::vector<AimSelection> candidates;std::string status;};
struct AimSession {
 AimSelection aimed;std::vector<AimSelection> candidates;std::uint32_t aimEpoch=0;
 std::uint64_t request=0,requestedAt=0;std::size_t candidate=0;bool pending=false,handsOnly=false;std::string aimStatus;
 void clear(){aimed={};candidates.clear();request=0;candidate=0;pending=false;aimStatus.clear();}
 void validateEpoch(std::uint32_t epoch){if(aimEpoch!=epoch){clear();aimEpoch=epoch;}}
 void begin(std::uint64_t id,std::uint32_t epoch,std::uint64_t tick=0){clear();aimEpoch=epoch;request=id;requestedAt=tick;pending=true;aimStatus=u8"等待游戏帧识别";}
 bool accept(const AimResult& r){if(!pending||r.id!=request||r.epoch!=aimEpoch)return false;pending=false;candidates=r.candidates;aimStatus=r.status;candidate=0;aimed=candidates.empty()?AimSelection{}:candidates.front();return true;}
 void next(){if(candidates.empty())return;candidate=(candidate+1)%candidates.size();aimed=candidates[candidate];}
};
inline bool aimVisible(const dango::inventory::Record& r,bool nativeHit=false){
 return eligibleRecord(r)&&r.classId>=0&&r.classId<=4096&&((r.dormant&&!*r.dormant)||(!r.dormant&&nativeHit))&&(!r.effects||!(*r.effects&32));
}
inline bool aimActor(const dango::inventory::Record& r){return specialCharacter(r)>=0||survivorPlayerClass(r.className)||r.className=="Infected";}
using AimOwnerLinks=std::map<std::uint32_t,std::uint32_t>;
inline constexpr std::uint32_t AmbiguousAimOwner=0xfffffffeu;
inline AimOwnerLinks aimOwnerLinks(const dango::inventory::Frame& f){
 AimOwnerLinks links;for(const auto& actor:f.records)if(aimActor(actor))for(auto child:{actor.activeWeapon,actor.viewModel}){
  if(!child||*child==0xffffffffu)continue;
  auto added=links.emplace(*child,actor.handle);
  if(!added.second&&added.first->second!=actor.handle)added.first->second=AmbiguousAimOwner;
 }return links;
}
// Follow only current full handles. A released carryable/projectile's historic
// thrower is not its current owner. Unknown weapon classes use activeWeapon too.
inline std::uint32_t aimActorOwner(const dango::inventory::Frame& f,const dango::inventory::Record& r,const AimOwnerLinks* links=nullptr){
 if(aimActor(r))return 0xffffffffu;
 AimOwnerLinks generated;if(!links){generated=aimOwnerLinks(f);links=&generated;}
 const auto* current=&r;std::set<std::uint32_t> seen;
 for(unsigned depth=0;current&&depth<16;++depth){
  if(!seen.insert(current->handle).second)return 0xffffffffu;
  if(current!=&r&&aimActor(*current))return current->handle;
  auto referenced=links->find(current->handle);if(referenced!=links->end())return referenced->second;
  const dango::inventory::Record* next=nullptr;
  if(current->parent&&*current->parent!=0xffffffffu)next=dango::follow::find(f,*current->parent);
  if(!next&&current->weaponParent&&*current->weaponParent!=0xffffffffu)next=dango::follow::find(f,*current->weaponParent);
  if(!next)for(auto owner:{current->weaponOwner,current->owner}){
   if(!owner||*owner==0xffffffffu)continue;
   auto p=dango::follow::find(f,*owner);if(!p)continue;
   bool active=p->activeWeapon&&*p->activeWeapon==current->handle;
   bool view=p->viewModel&&*p->viewModel==current->handle;
   bool weapon=current->weaponOwner&&*current->weaponOwner==*owner;
   if(active||view||handModelClass(current->className)||(weapon&&current->className.find("Projectile")==std::string::npos&&(!dango::follow::carryable(*current)||dango::follow::held(f,*current)))){next=p;break;}
  }
  current=next;
 }return 0xffffffffu;
}
// Do not offer a stale snapshot record after its index has been freed/reused.
template<class Entries> dango::inventory::Frame currentAimFrame(const dango::inventory::Frame& frame,const Entries& entries){
 dango::inventory::Frame current;current.epoch=frame.epoch;current.tick=frame.tick;current.records.reserve(frame.records.size());
 for(const auto& r:frame.records)if(r.index<entries.size()&&entries[r.index].entity&&entries[r.index].serial==r.serial&&r.serial<=0xfffffu&&r.handle==((r.serial<<12)|r.index))current.records.push_back(r);
 return current;
}
inline bool aimBounds(const dango::inventory::Record& r,dango::Vec3& lo,dango::Vec3& hi){
 if(r.collisionMins&&r.collisionMaxs&&dango::scene::finite(*r.collisionMins)&&dango::scene::finite(*r.collisionMaxs)){
  lo=*r.collisionMins;hi=*r.collisionMaxs;
  if(lo.x<=hi.x&&lo.y<=hi.y&&lo.z<=hi.z&&dango::length(hi-lo)>.01f&&dango::length(lo)<8192&&dango::length(hi)<8192)return true;
 }
 if(aimActor(r)){float half=specialCharacter(r)==6?32.f:18.f;lo={-half,-half,0};hi={half,half,specialCharacter(r)==6?110.f:76.f};}
 else {lo={-12,-12,-12};hi={12,12,12};}
 return false;
}
inline std::optional<float> aimBoxRay(const dango::CameraPose& eye,const dango::scene::Transform& pose,dango::Vec3 lo,dango::Vec3 hi,float limit){
 if(!dango::scene::valid(pose)||!std::isfinite(limit)||limit<=0)return {};
 auto delta=eye.eye-dango::Vec3{pose[3],pose[7],pose[11]};float entry=0,exit=limit;
 const float lower[]={lo.x,lo.y,lo.z},upper[]={hi.x,hi.y,hi.z};
 for(unsigned axis=0;axis<3;++axis){
  dango::Vec3 basis{pose[axis],pose[4+axis],pose[8+axis]};
  float origin=delta.x*basis.x+delta.y*basis.y+delta.z*basis.z;
  float direction=eye.forward.x*basis.x+eye.forward.y*basis.y+eye.forward.z*basis.z;
  if(std::abs(direction)<1e-6f){if(origin<lower[axis]||origin>upper[axis])return {};continue;}
  float a=(lower[axis]-origin)/direction,b=(upper[axis]-origin)/direction;if(a>b)std::swap(a,b);
  entry=std::max(entry,a);exit=std::min(exit,b);if(entry>exit)return {};
 }
 return entry;
}
inline std::vector<AimSelection> identifyAimCandidates(const dango::inventory::Frame& frame,const dango::GameCameraFrame& camera,std::uint64_t tick,AimQuery query={}){
 using namespace dango;std::vector<AimSelection> hits;CameraPose eye;
 if(!usableCamera(camera,tick)||!cameraPose(camera,eye)||frame.epoch!=camera.epoch||!frame.tick||tick<frame.tick||tick-frame.tick>250||query.blocked||!std::isfinite(query.limit)||query.limit<=0)return hits;
 auto make=[&](const inventory::Record& r,float distance,bool approximate,bool attachment){AimSelection a;a.classId=r.classId;a.handle=r.handle;a.name=r.className;a.score=distance;a.record=r;a.epoch=frame.epoch;a.approximate=approximate;a.attachment=attachment;return a;};
 auto links=aimOwnerLinks(frame);auto owner=[&](const inventory::Record& r){return aimActorOwner(frame,r,&links);};
 if(query.handsOnly){
  auto local=follow::find(frame,query.localHandle);if(!local)return hits;
  for(const auto& r:frame.records){bool active=local->activeWeapon&&*local->activeWeapon==r.handle;bool view=local->viewModel&&*local->viewModel==r.handle;
   if(r.handle==local->handle||!eligibleRecord(r)||r.classId<0||r.classId>4096||(r.dormant&&*r.dormant))continue;
   if(active||view||owner(r)==local->handle)hits.push_back(make(r,view?0.f:active?1.f:2.f,true,true));
  }
 }else{
  auto native=query.hitHandle?follow::find(frame,*query.hitHandle):nullptr;
  auto nativeOwner=native?owner(*native):0xffffffffu;
  if(nativeOwner==AmbiguousAimOwner)return hits;
  auto nativeHandle=nativeOwner!=0xffffffffu?nativeOwner:query.hitHandle.value_or(0xffffffffu);
  for(const auto& r:frame.records){
   bool nativeHit=r.handle==nativeHandle;
   if(!r.index||r.handle==query.localHandle||!aimVisible(r,nativeHit)||!actorIdentityKnown(r))continue;
   if(aimActor(r)&&((r.health&&*r.health<=0)||(r.lifeState&&*r.lifeState!=0)))continue;
   if(owner(r)!=0xffffffffu)continue;
   auto pose=follow::pose(frame,r.handle);
   if(aimActor(r)&&r.networkOrigin&&scene::finite(*r.networkOrigin)&&(!r.parent||*r.parent==0xffffffffu)){pose.ready=true;pose.matrix=follow::transform(*r.networkOrigin,{});}
   if(!pose.ready){if(nativeHit)hits.push_back(make(r,-1.f,true,false));continue;}
   // Character collision extents are axis aligned; eye/body pitch must not tilt them.
   if(aimActor(r))pose.matrix=follow::transform({pose.matrix[3],pose.matrix[7],pose.matrix[11]},{});
   Vec3 lo,hi;bool precise=aimBounds(r,lo,hi);auto distance=aimBoxRay(eye,pose.matrix,lo,hi,query.limit);
   if(!distance&&!nativeHit)continue;
   hits.push_back(make(r,nativeHit?-1.f:*distance,!precise,false));
  }
 }
 std::sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.score!=b.score?a.score<b.score:a.handle<b.handle;});
 if(hits.size()>16)hits.resize(16);
 if(!query.handsOnly){
  // Attachments are explicit secondary choices, never competitors for the body.
  auto bodies=hits;for(const auto& body:bodies)if(aimActor(body.record))for(const auto& r:frame.records){
   if(hits.size()>=32)break;
   if(aimVisible(r)&&r.handle!=query.localHandle&&owner(r)==body.handle)hits.push_back(make(r,body.score,true,true));
  }
 }
 return hits;
}
inline AimSelection identifyAim(const dango::inventory::Frame& f,const dango::GameCameraFrame& c,std::uint64_t tick){auto candidates=identifyAimCandidates(f,c,tick);return candidates.empty()?AimSelection{}:candidates.front();}
} // namespace visual
