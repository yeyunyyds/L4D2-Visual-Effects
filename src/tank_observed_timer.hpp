#pragma once
#include "tank_sequence.hpp"
#include <optional>
#include <cmath>
namespace visual::tank {
struct ObservedTimer {
 std::optional<float> remaining,span,sinceThrow,startedAt;unsigned eventId=0;bool triggered=false,expired=false,throwing=false;
 const char* source="unknown";const char* reason="no observed punch onset";
};
class SwingStopwatch {
 bool initialized=false,attack=false,throwing=false,hasTimer=false;int sequence=-1;std::optional<float> cycle,lastTime,deadline,span,throwEnd;
 const char* source="unknown";unsigned eventId=0;std::optional<float> startedAt;
public:
 void reset(){initialized=attack=throwing=hasTimer=false;sequence=-1;cycle.reset();lastTime.reset();deadline.reset();span.reset();throwEnd.reset();startedAt.reset();eventId=0;source="unknown";}
 ObservedTimer update(std::optional<float> now,std::uint64_t tick,int seq,std::optional<float> currentCycle,const std::optional<SequenceName>& name,std::optional<float> interval,std::optional<float> miss,bool manual=false){
  (void)tick;ObservedTimer out;
  if(!now||!std::isfinite(*now)){out.reason="Source clock unavailable/stalled";return out;}
  if(lastTime&&*now<*lastTime-.001f){reset();out.reason="Source clock reset";return out;}
  auto valid=[](std::optional<float> f){return f&&std::isfinite(*f)&&*f>=0&&*f<=10;};
  bool known=name.has_value();auto kind=known?classify(*name):std::string();bool currentAttack=kind=="attack-animation",currentThrow=kind=="throw-animation";
  bool cycleValid=currentCycle&&std::isfinite(*currentCycle)&&*currentCycle>=0&&*currentCycle<=1;
  bool wrapped=cycleValid&&cycle&&*cycle>.65f&&*currentCycle<.25f;
  bool onset=known&&currentAttack&&((initialized&&(!attack||seq!=sequence||wrapped))||(!initialized&&cycleValid&&*currentCycle<.2f));
  if(known&&currentThrow&&!throwing){hasTimer=false;deadline.reset();span.reset();throwEnd.reset();}
  if(known&&throwing&&!currentThrow)throwEnd=now;
  if(manual||onset){
   hasTimer=false;deadline.reset();span.reset();
   if(valid(interval)){
    float duration=*interval;if(valid(miss))duration=std::min(duration,*miss);
    // An automatically observed onset happened between this and the preceding
    // sample. Use the earlier bound so a delayed observation does not grant a
    // falsely long "safe" window. Manual marks start at the marked instant.
    float start=*now;if(!manual&&lastTime&&*now>=*lastTime&&*now-*lastTime<=.25f)start=*lastTime;
    deadline=start+duration;span=duration;startedAt=start;++eventId;hasTimer=true;source=manual?"manual-observed-punch":"animation-observed-punch";out.triggered=true;
    out.reason=valid(miss)?"parameter-based estimate; earliest hit/miss interval":"miss interval unavailable; normal-interval estimate";
   }else out.reason="punch marked but live swing interval unavailable";
  }
  if(known){initialized=true;attack=currentAttack;throwing=currentThrow;sequence=seq;if(cycleValid)cycle=currentCycle;else cycle.reset();}
  // An unresolved frame does not count as leaving/re-entering the same attack.
  lastTime=now;out.throwing=throwing;out.eventId=eventId;out.startedAt=startedAt;
  if(throwEnd&&*now>=*throwEnd)out.sinceThrow=*now-*throwEnd;
  if(throwing&&!hasTimer){out.reason="throw animation; recovery duration unknown";return out;}
  if(hasTimer&&deadline&&span){out.remaining=std::max(0.f,*deadline-*now);out.span=span;out.expired=*out.remaining<=0;out.source=source;if(!out.triggered)out.reason=out.expired?"estimated window ended; attack may be available":"observed-punch stopwatch estimate";}
  return out;
 }
};
}
