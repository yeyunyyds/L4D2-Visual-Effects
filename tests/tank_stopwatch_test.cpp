#include "tank_observed_timer.hpp"
#include <stdexcept>
#include <iostream>
void verify(bool ok,int line){if(!ok)throw std::runtime_error("observed swing timer regression at line "+std::to_string(line));}
#define check(x) verify((x),__LINE__)
bool near(float a,float b){return std::abs(a-b)<.001f;}
int main(){using namespace visual::tank;std::optional<SequenceName> punch=SequenceName{"Punch_Left","ACT_MELEE_ATTACK1"},idle=SequenceName{"Idle_Rage",""},rock=SequenceName{"Throw_02","ACT_TANK_OVERHEAD_THROW"};
 SwingStopwatch watch;auto step=[&](float time,int seq,float cycle,const std::optional<SequenceName>& n,bool manual=false){return watch.update(time,1000,seq,cycle,n,1.5f,1.f,manual,false,.5f);};
 auto r=step(10,9,.5f,idle);check(!r.remaining);r=step(10.1f,4,.05f,punch);check(r.triggered&&r.remaining&&near(*r.remaining,1.9f)&&r.span==2.f&&r.eventId==1&&near(*r.startedAt,10));
 r=step(10.2f,4,.2f,punch);check(!r.triggered&&near(*r.remaining,1.8f)&&r.eventId==1);
 r=step(10.3f,4,.3f,{});check(!r.triggered);r=step(10.4f,4,.4f,punch);check(!r.triggered&&near(*r.remaining,1.6f));
 r=step(11.1f,4,.9f,punch);check(!r.expired&&near(*r.remaining,.9f));r=step(12.f,4,.95f,punch);check(r.expired&&r.remaining==0);r=step(12.1f,4,.02f,punch);check(r.triggered&&r.eventId==2&&r.span==2.f);
 r=step(12.2f,49,.1f,rock);check(!r.remaining&&r.throwing);r=step(13.2f,49,.5f,rock);check(!r.remaining&&r.throwing);r=step(13.4f,9,.1f,idle);check(!r.remaining&&r.sinceThrow==0.f);r=step(14,9,.6f,idle);check(r.sinceThrow&&near(*r.sinceThrow,.6f));
 r=step(14.1f,80,.2f,{},true);check(r.triggered&&r.remaining==2&&std::string(r.source)=="manual-observed-punch");r=step(14.5f,80,.5f,{});check(near(*r.remaining,1.6f));
 r=watch.update({},999999,80,{},punch,1.5f,1.f,true,false,.5f);check(!r.remaining&&!r.triggered);
 r=step(16.2f,80,.8f,{});check(r.expired);r=step(2,9,.1f,idle);check(!r.remaining&&!r.triggered);
 SwingStopwatch late;r=late.update(20,1,4,.7f,punch,1.5f,1.f,false,false,.5f);check(!r.remaining);r=late.update(20.1f,2,4,.8f,punch,1.5f,1.f,false,false,.5f);check(!r.triggered);r=late.update(20.2f,3,4,.01f,punch,1.5f,1.f,false,false,.5f);check(r.triggered);
 // The live windup and normal interval are both required; miss time is not a substitute.
 SwingStopwatch missing;check(!missing.update(1,1,9,.1f,idle,1.5f,1.f,false,true).remaining);check(!missing.update(1.1f,2,9,.2f,idle,{},1.f,false,true,.5f).remaining);
 SwingStopwatch zero;r=zero.update(1,1,9,.1f,idle,0.f,1.f,false,true,0.f);check(r.remaining==0.f&&r.expired&&r.span==0.f);
 // Miss-only changes must not halve a normal-cycle estimate. No fixed 2s constant.
 SwingStopwatch dynamic;r=dynamic.update(1,1,9,.1f,idle,2.3f,.1f,false,true,.7f);check(r.span&&near(*r.span,3.f));r=dynamic.update(1.1f,2,9,.2f,idle,2.3f,0.f,false,false,.7f);check(!r.triggered&&near(*r.remaining,2.9f));
 r=dynamic.update(4,3,9,.01f,idle,.8f,10.f,false,true,.2f);check(r.span==1.f);
 std::cout<<"PASS: complete normal attack cycle, live windup/interval updates, miss separation, deduplication, missing clocks/parameters, pause/reset and independent throw state\n";
}
