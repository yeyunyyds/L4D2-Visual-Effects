#pragma once
#include "special_infected.hpp"
#include "tank_cooldown.hpp"
#include "dango/entity_inventory.hpp"
#include <map>
#include <optional>
namespace visual::tank {
using dango::Vec3;using dango::inventory::Record;using dango::inventory::Frame;
struct Settings{bool enabled=false,draw=true;unsigned horizonMs=200;bool warnBar=false,warnRange=false,warnFill=false,warnOutline=false;};
template<class Json> Settings readSettings(const Json& j){Settings s;auto p=j.find("tankPrediction");if(p==j.end()||!p->is_object())return s;
 for(auto value:{std::pair<const char*,bool*>{"enabled",&s.enabled},{"draw",&s.draw},{"warnBar",&s.warnBar},{"warnRange",&s.warnRange},{"warnFill",&s.warnFill},{"warnOutline",&s.warnOutline}})if(p->contains(value.first)&&(*p)[value.first].is_boolean())*value.second=(*p)[value.first].template get<bool>();
 if(p->contains("horizonMs")&&(*p)["horizonMs"].is_number_unsigned())s.horizonMs=std::clamp((*p)["horizonMs"].template get<unsigned>(),0u,500u);
 return s;
}
template<class Json> void writeSettings(Json& j,const Settings& s){j["tankPrediction"]={{"enabled",s.enabled},{"draw",s.draw},{"horizonMs",s.horizonMs},{"warnBar",s.warnBar},{"warnRange",s.warnRange},{"warnFill",s.warnFill},{"warnOutline",s.warnOutline}};}
struct Parameters{std::optional<float> range,arc,windup,duration,interval,missInterval;std::uint64_t tick=0;std::string source="unavailable";};
inline bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline float horizontal(Vec3 v){return std::hypot(v.x,v.y);}
inline float yaw(Vec3 v){return std::atan2(v.y,v.x)*180.f/3.14159265358979323846f;}
inline float difference(float a,float b){return std::remainder(a-b,360.f);}
inline Vec3 forward(float angle){float r=angle*3.14159265358979323846f/180;return {std::cos(r),std::sin(r),0};}
inline bool survivor(const Record& r){return survivorPlayerClass(r.className)&&r.team&&*r.team==2&&r.networkOrigin&&finite(*r.networkOrigin)&&(!r.lifeState||*r.lifeState==0)&&(!r.health||*r.health>0)&&(!r.dormant||!*r.dormant);}
inline bool tank(const Record& r){return specialCharacter(r)==6&&r.networkOrigin&&finite(*r.networkOrigin)&&(!r.lifeState||*r.lifeState==0)&&(!r.health||*r.health>0)&&(!r.dormant||!*r.dormant);}
inline std::string role(const Record& r){return r.character&&*r.character>=0&&*r.character<int(SurvivorCount)?SurvivorNames[*r.character]:"Survivor #"+std::to_string(r.index);}
struct Aim{float yaw=0;bool eye=false;};
inline std::optional<Aim> aim(const Record& r){if(r.eyeAngles&&finite(*r.eyeAngles))return Aim{r.eyeAngles->y,true};if(r.networkAngles&&finite(*r.networkAngles))return Aim{r.networkAngles->y,false};return {};}
struct Motion{Vec3 position{},velocity{};std::uint64_t tick=0;};
struct Target{std::uint32_t handle=0;std::string name,source;Vec3 origin{};float score=0;bool authoritativeField=false;};
inline const Record* resolve(const Frame& f,std::uint32_t handle){for(const auto& r:f.records)if(r.handle==handle)return &r;return nullptr;}
inline std::optional<Target> resolveTarget(const Record& r,const Frame& frame){
 for(auto pair:{std::pair<const std::optional<std::uint32_t>*,const char*>{&r.enemyHandle,"network-enemy"},{&r.lookAtHandle,"network-look-at"}}){if(!*pair.first)continue;auto s=resolve(frame,**pair.first);if(s&&survivor(*s))return Target{s->handle,role(*s),pair.second,*s->networkOrigin,1,pair.first==&r.enemyHandle};}return {};
}
inline std::optional<Target> inferTarget(const Record& tank,const Frame& frame,float heading,std::uint32_t previous){
 std::optional<Target> first;float second=0;
 for(const auto& s:frame.records){if(!survivor(s))continue;auto offset=*s.networkOrigin-*tank.networkOrigin;float distance=horizontal(offset);if(distance<1||distance>2000||std::abs(offset.z)>300)continue;
  float angle=std::abs(difference(yaw(offset),heading));if(angle>65)continue;float score=(1-angle/65.f)*.8f+(1-distance/2000.f)*.2f;if(s.handle==previous)score+=.04f;
  if(!first||score>first->score){if(first)second=first->score;first=Target{s.handle,role(s),"estimated-facing",*s.networkOrigin,score,false};}else second=std::max(second,score);
 }
 if(!first||first->score<.6f||first->score-second<.12f)return {};
 return first;
}
struct RangeWarning{bool known=false,inside=false;float distance=0;};
struct Observation{RangeWarning warning;std::uint32_t handle=0;Vec3 origin{},velocity{},predicted{};float heading=0,pitch=0,radius=0,halfArc=0;bool eye=false,geometry=false,headingStalled=false,nearVerticalAim=false;std::optional<Target> target,gaze;std::optional<std::uint32_t> lookAt;std::optional<int> sequence;std::optional<float> cycle;float headingRate=0;std::optional<float> cooldown;CooldownReading timing;std::string action="unknown",reason;};
inline bool warningRequested(const Settings& s){return s.warnBar||s.warnRange||s.warnFill||s.warnOutline;}
// Match the displayed current sector, not the future dashed arc or a cooldown window.
inline RangeWarning rangeWarning(const Observation& o,const Record* local,const Record* actor){
 RangeWarning w;if(!o.geometry||!local||!actor||!survivor(*local)||!local->health||!tank(*actor)||!finite(o.origin)||!std::isfinite(o.radius)||o.radius<=0||!std::isfinite(o.heading)||!std::isfinite(o.halfArc)||o.halfArc<=0||o.halfArc>180)return w;
 auto delta=*local->networkOrigin-o.origin;w.known=true;w.distance=horizontal(delta);
 bool height=std::abs(delta.z)<=o.radius;
 auto bounds=[](const Record& r){return r.collisionMins&&r.collisionMaxs&&finite(*r.collisionMins)&&finite(*r.collisionMaxs)&&r.collisionMins->z<=r.collisionMaxs->z&&std::abs(r.collisionMins->z)<=256&&std::abs(r.collisionMaxs->z)<=256;};
 if(bounds(*local)&&bounds(*actor))height=local->networkOrigin->z+local->collisionMaxs->z>=o.origin.z+actor->collisionMins->z&&local->networkOrigin->z+local->collisionMins->z<=o.origin.z+actor->collisionMaxs->z;
 w.inside=height&&w.distance<=o.radius&&(w.distance<.001f||std::abs(difference(yaw(delta),o.heading))<=o.halfArc);
 return w;
}
struct Result{std::uint32_t epoch=0;std::uint64_t tick=0;Parameters parameters;ClockReading clock;std::vector<Observation> tanks;};
struct Track{Motion motion;float heading=0;std::uint64_t steadySince=0;std::uint32_t target=0;};
class Predictor {
 std::uint32_t epoch=0;std::map<std::uint32_t,Track> tracks;CombatClock clock;
public:
 void reset(){tracks.clear();clock.reset();epoch=0;}
 Result update(const Frame& frame,const Parameters& p,const Settings& s,std::uint32_t localHandle=0xffffffffu){
  if(epoch!=frame.epoch){tracks.clear();epoch=frame.epoch;}Result out;out.epoch=frame.epoch;out.tick=frame.tick;out.parameters=p;out.clock=clock.update(frame);std::map<std::uint32_t,Track> next;
  for(const auto& r:frame.records){if(!tank(r)||out.tanks.size()>=8)continue;auto direction=aim(r);Observation o;o.handle=r.handle;o.origin=*r.networkOrigin;o.sequence=r.sequence;o.cycle=r.animationCycle;o.lookAt=r.lookAtHandle;
   auto previous=tracks.find(r.handle);Track track;if(previous!=tracks.end())track=previous->second;
   if(direction){o.heading=direction->yaw;o.eye=direction->eye;if(o.eye){o.pitch=std::remainder(r.eyeAngles->x,360.f);o.nearVerticalAim=std::abs(std::cos(o.pitch*3.14159265358979323846f/180))<.25f;}}
   bool timely=track.motion.tick&&frame.tick>track.motion.tick&&frame.tick-track.motion.tick<=250;
   if(timely){float seconds=float(frame.tick-track.motion.tick)*.001f;auto displacement=o.origin-track.motion.position;
    if(horizontal(displacement)<200&&std::abs(displacement.z)<200)o.velocity=displacement*(1/seconds);
    if(horizontal(o.velocity)>1000)o.velocity={};
    if(direction)o.headingRate=difference(o.heading,track.heading)/seconds;
   }
   if(r.velocity&&finite(*r.velocity)&&dango::length(*r.velocity)<1000)o.velocity=*r.velocity;
   o.predicted=o.origin+o.velocity*(float(std::min(s.horizonMs,500u))*.001f);
   o.timing=readCooldown(r,frame,out.clock);o.cooldown=o.timing.remaining;
   if(r.lookAtHandle){auto looked=resolve(frame,*r.lookAtHandle);if(looked&&survivor(*looked))o.gaze=Target{looked->handle,role(*looked),"network-look-at",*looked->networkOrigin,1,false};}
   o.target=resolveTarget(r,frame);if(!o.target&&direction)o.target=inferTarget(r,frame,o.heading,track.target);
   if(!direction||!timely||std::abs(o.headingRate)>4)track.steadySince=frame.tick;
   if(direction&&timely&&!o.nearVerticalAim&&o.target&&frame.tick>=track.steadySince&&frame.tick-track.steadySince>=150){float angle=std::abs(difference(yaw(o.target->origin-o.origin),o.heading));o.headingStalled=angle>25;}
   // Never narrow the arc or shorten reach solely because a heading stayed still.
   // Geometry needs fresh runtime values, not a hard-coded stock distance.
   if(direction&&!o.nearVerticalAim&&p.range&&p.arc&&p.tick<=frame.tick&&frame.tick-p.tick<=2500&&*p.range>0&&*p.range<=512&&*p.arc>0&&*p.arc<=360){
    float hull=0;if(r.collisionMins&&r.collisionMaxs&&horizontal(*r.collisionMins)<128&&horizontal(*r.collisionMaxs)<128)hull=std::max({std::abs(r.collisionMins->x),std::abs(r.collisionMins->y),std::abs(r.collisionMaxs->x),std::abs(r.collisionMaxs->y)});
    o.radius=*p.range+hull;o.halfArc=*p.arc*.5f;o.geometry=true;
   }
   if(warningRequested(s))o.warning=rangeWarning(o,resolve(frame,localHandle),&r);
   o.reason=!direction?"aim unavailable":o.nearVerticalAim?"near-vertical aim; ground footprint withheld":!o.geometry?"runtime range/arc unavailable":o.eye?"network aim + runtime parameters; estimated footprint":"body angle fallback; low confidence";
   track.motion={o.origin,o.velocity,frame.tick};track.heading=o.heading;track.target=o.target?o.target->handle:0;next[r.handle]=track;out.tanks.push_back(std::move(o));
  }tracks=std::move(next);return out;
 }
};
}
