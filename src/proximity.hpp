#pragma once
#include "entity_selection.hpp"
#include "aim_selection.hpp"
#include <set>
#include <memory>
namespace visual {
struct ProximitySettings {
 bool enabled=false,includeCommon=false;
 int triggerClassId=-1;
 std::string triggerName="Tank";
 unsigned radius=500;
};
inline unsigned normalizedRadius(unsigned radius){return std::clamp(radius,100u,5000u);}
template<class Json> ProximitySettings readProximity(const Json& j){
 ProximitySettings result;auto p=j.find("proximityStretch");if(p==j.end()||!p->is_object())return result;
 auto enabled=p->find("enabled");if(enabled!=p->end()&&enabled->is_boolean())result.enabled=enabled->template get<bool>();
 auto common=p->find("includeCommon");if(common!=p->end()&&common->is_boolean())result.includeCommon=common->template get<bool>();
 auto id=p->find("triggerClassId");if(id!=p->end()&&id->is_number_integer()&&*id>=-1&&*id<=4096)result.triggerClassId=id->template get<int>();
 auto name=p->find("triggerName");if(name!=p->end()&&name->is_string()){
  auto text=name->template get<std::string>();if(!text.empty()&&text.size()<=127)result.triggerName=std::move(text);
 }
 auto radius=p->find("radius");if(radius!=p->end()&&radius->is_number_integer()){
  if(*radius<100)result.radius=100;else if(*radius>5000)result.radius=5000;else result.radius=radius->template get<unsigned>();
 }
 return result;
}
template<class Json> void writeProximity(Json& j,const ProximitySettings& settings){
 j["proximityStretch"]={{"enabled",settings.enabled},{"includeCommon",settings.includeCommon},
  {"triggerClassId",settings.triggerClassId},{"triggerName",settings.triggerName},{"radius",normalizedRadius(settings.radius)}};
}
// Immutable per-snapshot mask; membership is by full handle, never by viewer distance.
struct ProximityResult {
 bool active=false;
 std::uint32_t epoch=0;std::uint64_t tick=0;
 unsigned sources=0;
 std::set<std::uint32_t> inRange;
 std::shared_ptr<const ProximitySettings> options;
};
using HeldHandles=std::set<std::uint32_t>;
inline HeldHandles experimentExcludedHandles(const dango::inventory::Frame& frame,bool includeCommon){
 HeldHandles held;
 for(const auto& r:frame.records){
  if(r.activeWeapon&&*r.activeWeapon!=0xffffffffu)held.insert(*r.activeWeapon);
  if(r.viewModel&&*r.viewModel!=0xffffffffu)held.insert(*r.viewModel);
  if(handModelClass(r.className)||(pickupClass(r.className)&&dango::follow::held(frame,r))||(!includeCommon&&r.className=="Infected"))held.insert(r.handle);
 }
 // Exclude attachments parented to held models too; cycles are bounded.
 for(unsigned depth=0;depth<16;++depth){bool changed=false;
  for(const auto& r:frame.records)if(r.parent&&*r.parent!=0xffffffffu&&held.count(*r.parent))changed|=held.insert(r.handle).second;
  if(!changed)break;
 }
 return held;
}
inline ProximityResult evaluateProximity(const dango::inventory::Frame& frame,const dango::GameCameraFrame& camera,
                                        const ProximitySettings& settings,std::uint64_t tick){
 ProximityResult result;result.epoch=frame.epoch;result.tick=frame.tick;
 if(!settings.enabled||!camera.active||frame.epoch!=camera.epoch||!frame.tick||tick<frame.tick||tick-frame.tick>250)return result;
 std::vector<dango::Vec3> centers;
 for(const auto& r:frame.records){
  int role=specialIndex(settings.triggerName),survivor=survivorIndex(settings.triggerName);
  bool matches=settings.triggerClassId>=0?r.classId==settings.triggerClassId:role>=0?specialCharacter(r)==role:survivor>=0?survivorCharacter(r)==survivor:r.className==settings.triggerName;
  if(!matches)continue;
  auto group=groupForName(r.className);bool actor=group==EntityGroup::Special||group==EntityGroup::Survivors||r.className=="Infected";
  if(actor&&((r.health&&*r.health<=0)||(r.lifeState&&*r.lifeState!=0)))continue;
  auto pose=dango::follow::pose(frame,r.handle);if(pose.ready)centers.push_back(dango::scene::point(pose.matrix,{}));
 }
 result.sources=unsigned(centers.size());if(centers.empty())return result;
 const auto excluded=experimentExcludedHandles(frame,settings.includeCommon);
 const double radius=normalizedRadius(settings.radius),radiusSquared=radius*radius;
 for(const auto& r:frame.records){
  if(!eligibleRecord(r)||r.classId<0||r.classId>4096||excluded.count(r.handle))continue;
  auto pose=dango::follow::pose(frame,r.handle);if(!pose.ready)continue;
  auto point=dango::scene::point(pose.matrix,{});
  for(auto center:centers){auto delta=point-center;
   double squared=double(delta.x)*delta.x+double(delta.y)*delta.y+double(delta.z)*delta.z;
   if(std::isfinite(squared)&&squared<=radiusSquared){result.inRange.insert(r.handle);break;}
  }
 }
 result.active=!result.inRange.empty();return result;
}
inline bool selectedForEffect(const dango::inventory::Record& r,const EntitySelection& selection,
                              const ProximitySettings& proximity,const ProximityResult& targets){
 if(!proximity.enabled)return selection.selectedRecord(r);
 return targets.inRange.count(r.handle)!=0;
}
} // namespace visual
