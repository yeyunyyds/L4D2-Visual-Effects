#include "tank_observed_timer.hpp"
#include <stdexcept>
#include <iostream>
void verify(bool ok,int line){if(!ok)throw std::runtime_error("observed swing timer regression at line "+std::to_string(line));}
#define check(x) verify((x),__LINE__)
bool near(float a,float b){return std::abs(a-b)<.001f;}
int main(){using namespace visual::tank;std::optional<SequenceName> punch=SequenceName{"Punch_Left","ACT_MELEE_ATTACK1"},idle=SequenceName{"Idle_Rage",""},rock=SequenceName{"Throw_02","ACT_TANK_OVERHEAD_THROW"};
 SwingStopwatch watch;auto step=[&](float time,int seq,float cycle,const std::optional<SequenceName>& n,bool manual=false){return watch.update(time,1000,seq,cycle,n,1.5f,1.f,manual);};
 auto r=step(10,9,.5f,idle);check(!r.remaining);r=step(10.1f,4,.05f,punch);check(r.triggered&&r.remaining&&near(*r.remaining,.9f)&&r.eventId==1&&near(*r.startedAt,10));
 r=step(10.2f,4,.2f,punch);check(!r.triggered&&near(*r.remaining,.8f)&&r.eventId==1);
 r=step(10.3f,4,.3f,{});check(!r.triggered);r=step(10.4f,4,.4f,punch);check(!r.triggered&&near(*r.remaining,.6f));
 r=step(11.1f,4,.9f,punch);check(r.expired&&r.remaining==0);r=step(11.2f,4,.02f,punch);check(r.triggered&&r.eventId==2);
 r=step(11.3f,49,.1f,rock);check(!r.remaining&&r.throwing);r=step(12.3f,49,.5f,rock);check(!r.remaining&&r.throwing);r=step(12.5f,9,.1f,idle);check(!r.remaining&&r.sinceThrow==0.f);r=step(13,9,.6f,idle);check(r.sinceThrow&&near(*r.sinceThrow,.5f));
 r=step(13.1f,80,.2f,{},true);check(r.triggered&&r.remaining==1&&std::string(r.source)=="manual-observed-punch");r=step(13.5f,80,.5f,{});check(near(*r.remaining,.6f));
 // No new wall-clock timer is fabricated when Source time is unavailable.
 r=watch.update({},999999,80,{},punch,1.5f,1.f,true);check(!r.remaining&&!r.triggered);
 r=step(14.2f,80,.8f,{});check(r.expired);r=step(2,9,.1f,idle);check(!r.remaining&&!r.triggered);
 SwingStopwatch late;r=late.update(20,1,4,.7f,punch,1.5f,1.f);check(!r.remaining);r=late.update(20.1f,2,4,.8f,punch,1.5f,1.f);check(!r.triggered);r=late.update(20.2f,3,4,.01f,punch,1.5f,1.f);check(r.triggered);
 SwingStopwatch noInterval;r=noInterval.update(1,1,80,{}, {},{},1.f,true);check(!r.remaining&&!r.triggered);
 SwingStopwatch zero;r=zero.update(1,1,80,{}, {},1.5f,0.f,true);check(r.remaining==0.f&&r.expired&&r.span==0.f);
 SwingStopwatch overrides;r=overrides.update(1,1,49,.1f,rock,1.5f,1.f);check(!r.remaining);r=overrides.update(1.1f,2,49,.2f,rock,1.5f,1.f,true);check(r.triggered&&r.remaining&&near(*r.remaining,1));r=overrides.update(1.2f,3,49,.3f,rock,1.5f,1.f);check(r.remaining&&near(*r.remaining,.9f));
 std::cout<<"PASS: observed onset/cycle deduplication, earliest interval, unknown frames, manual marks, pause/reset, zero/missing parameters and separate throw state\n";
}
