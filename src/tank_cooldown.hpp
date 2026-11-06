#pragma once
#include "dango/entity_inventory.hpp"
#include "survivors.hpp"
#include <optional>
namespace visual::tank {
using dango::inventory::Frame;using dango::inventory::Record;
struct ClockReading{std::optional<float> time;std::uint32_t source=0;const char* reason="game clock unavailable";};
class CombatClock {
 std::uint32_t epoch=0;float last=0;std::uint64_t lastAdvance=0,lastTick=0;bool initialized=false,advanced=false;
public:
 void reset(){*this=CombatClock{};}
 ClockReading update(const Frame& f){
  if(epoch!=f.epoch||f.tick<lastTick){reset();epoch=f.epoch;}lastTick=f.tick;
  std::optional<float> latest;std::uint32_t source=0;
  for(const auto& r:f.records){if(!survivorPlayerClass(r.className)&&r.className!="Tank")continue;if((r.health&&*r.health<=0)||(r.dormant&&*r.dormant))continue;
   if(r.simulationTime&&std::isnormal(*r.simulationTime)&&*r.simulationTime>0&&*r.simulationTime<10000000&&(!latest||*r.simulationTime>*latest)){latest=r.simulationTime;source=r.handle;}
  }
  if(!latest)return {};
  if(!initialized||*latest<last-.001f||*latest>last+10){initialized=true;advanced=false;last=*latest;lastAdvance=f.tick;return {{},source,"waiting for clock progression"};}
  if(*latest>last+.00001f){last=*latest;lastAdvance=f.tick;advanced=true;}
  if(!advanced||f.tick<lastAdvance||f.tick-lastAdvance>250)return {{},source,"game clock stalled or unverified"};
  return {*latest,source,"observed actor simulation time; no wall-clock extrapolation"};
 }
};
struct CooldownReading {
 std::optional<float> remaining,weaponRemaining,actorRemaining,weaponDeadline,actorDeadline;
 const char* reason="attack timing unavailable";bool partial=false;
};
inline CooldownReading readCooldown(const Record& tank,const Frame& frame,const ClockReading& clock){
 CooldownReading out;
 if(!clock.time){out.reason=clock.reason;return out;}
 if(!tank.activeWeapon){out.reason="active weapon unavailable";return out;}
 const Record* weapon=nullptr;for(const auto& r:frame.records)if(r.handle==*tank.activeWeapon){weapon=&r;break;}
 if(!weapon||weapon->className!="CTankClaw"){out.reason="active claw missing or handle changed";return out;}
 for(const auto& owner:{weapon->weaponOwner,weapon->owner})if(owner&&*owner!=0xffffffffu&&*owner!=tank.handle){out.reason="claw owner mismatch";return out;}
 auto valid=[&](std::optional<float> deadline)->std::optional<float>{if(!deadline||!std::isfinite(*deadline)||*deadline<=0||*deadline>=10000000||*deadline-*clock.time>30)return {};return deadline;};
 out.weaponDeadline=valid(weapon->nextPrimaryAttack);out.actorDeadline=valid(tank.nextAttack);
 if(out.weaponDeadline)out.weaponRemaining=std::max(0.f,*out.weaponDeadline-*clock.time);
 if(out.actorDeadline)out.actorRemaining=std::max(0.f,*out.actorDeadline-*clock.time);
 if(out.weaponRemaining&&out.actorRemaining){out.remaining=std::max(*out.weaponRemaining,*out.actorRemaining);out.reason="client claw/actor timer constraints; not AI attack eligibility";}
 else if(out.weaponRemaining||out.actorRemaining){out.partial=true;out.remaining=out.weaponRemaining?out.weaponRemaining:out.actorRemaining;out.reason="partial timer lower bound; missing or zero field is unconfirmed";if(out.remaining&&*out.remaining==0){out.remaining.reset();out.reason="observed gate elapsed; other timing gate unconfirmed";}}
 else out.reason="zero or missing timing fields; replication unconfirmed";
 return out;
}
}
