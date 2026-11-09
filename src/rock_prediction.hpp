#pragma once
#include "rock_identity.hpp"
#include <optional>
namespace visual::rocks {
using dango::Vec3;
inline constexpr float Gravity=320,HalfSize=16;
inline constexpr uint64_t SampleLifetimeMs=250;
inline bool valid(Vec3 p,float limit=32768){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})<=limit;}
enum class State {Discovered,Held,Flight,Stopped,Stale,Destroyed};
struct Sample {uint64_t wallMs=0;Vec3 position{},velocity{};bool velocityValid=false;};
struct Point {float seconds=0;Vec3 position{},velocity{};};
struct Track {dango::inventory::Record record;Identity identity=Identity::None;State state=State::Discovered;uint64_t lastSeenMs=0;std::optional<Sample> motion;std::vector<Point> trajectory;};
class Tracker {
 uint32_t epoch_=0;std::map<uint32_t,Track> tracks_;
 void clearMotion(Track& t,State state){t.motion.reset();t.trajectory.clear();t.state=state;}
public:
 void reset(uint32_t epoch){tracks_.clear();epoch_=epoch;}
 const std::map<uint32_t,Track>& tracks()const{return tracks_;}
 void discover(const dango::inventory::Frame& frame){
  if(frame.epoch!=epoch_)reset(frame.epoch);
  for(const auto& r:frame.records){auto identification=identifyRock(r,frame);if(identification.kind==Identity::None)continue;
   for(auto& item:tracks_)if(item.first!=r.handle&&item.second.record.index==r.index)clearMotion(item.second,State::Destroyed);
   if(tracks_.count(r.handle))continue;
   if(tracks_.size()>=64){auto old=std::find_if(tracks_.begin(),tracks_.end(),[](const auto& item){return item.second.state==State::Destroyed||item.second.state==State::Stale;});if(old==tracks_.end())continue;tracks_.erase(old);}
   Track t;t.record=r;t.identity=identification.kind;t.lastSeenMs=frame.tick;tracks_.emplace(r.handle,std::move(t));
  }
 }
 void unavailable(uint32_t handle,bool destroyed){auto it=tracks_.find(handle);if(it!=tracks_.end())clearMotion(it->second,destroyed?State::Destroyed:State::Stale);}
 void observe(const dango::inventory::Record& r,uint64_t now,unsigned horizonMs){
  auto it=tracks_.find(r.handle);if(it==tracks_.end()||it->second.state==State::Destroyed)return;auto& t=it->second;if(r.className!=t.record.className||!validIdentity(r)){clearMotion(t,State::Destroyed);return;}t.record=r;t.lastSeenMs=now;
  if(!r.parent||*r.parent!=0xffffffffu){clearMotion(t,State::Held);return;}
  if(!r.networkOrigin||!valid(*r.networkOrigin)||(r.dormant&&*r.dormant)){clearMotion(t,State::Stale);return;}
  Sample s;s.wallMs=now;s.position=*r.networkOrigin;
  if(t.motion){const auto& old=*t.motion;if(now<=old.wallMs)return;float dt=float(now-old.wallMs)/1000;float distance=dango::length(s.position-old.position);
   if(distance<.001f){if(now-old.wallMs>SampleLifetimeMs){t.trajectory.clear();t.state=State::Stale;}return;}
   if(dt>1||distance>20000*dt+64){clearMotion(t,State::Stale);return;}
   if(r.velocity&&valid(*r.velocity,20000)){s.velocity=*r.velocity;s.velocityValid=true;}else{s.velocity=(s.position-old.position)*(1/dt);s.velocityValid=valid(s.velocity,20000);}
  }else if(r.velocity&&valid(*r.velocity,20000)){s.velocity=*r.velocity;s.velocityValid=true;}
  t.motion=s;t.trajectory.clear();if(!s.velocityValid||dango::length(s.velocity)<=20){t.state=State::Stopped;return;}t.state=State::Flight;
  const float horizon=std::clamp(horizonMs,100u,1000u)/1000.f;
  for(unsigned i=0;i<=unsigned(std::ceil(horizon/.025f));++i){float dt=std::min(i*.025f,horizon);Vec3 g{0,0,-Gravity};auto p=s.position+s.velocity*dt+g*(.5f*dt*dt);if(!valid(p))break;t.trajectory.push_back({dt,p,s.velocity+g*dt});}
 }
 void expire(uint64_t now){for(auto it=tracks_.begin();it!=tracks_.end();){auto& t=it->second;if(now>=t.lastSeenMs&&now-t.lastSeenMs>2000){it=tracks_.erase(it);continue;}if(t.state!=State::Destroyed&&(now<t.lastSeenMs||now-t.lastSeenMs>500))clearMotion(t,State::Stale);if(t.motion&&(now<t.motion->wallMs||now-t.motion->wallMs>SampleLifetimeMs))t.trajectory.clear();++it;}}
};
}
