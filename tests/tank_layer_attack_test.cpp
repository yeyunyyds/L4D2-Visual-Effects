#include "tank_layer_attack.hpp"
#include "tank_observed_timer.hpp"
#include <iostream>
#include <stdexcept>
void verify(bool ok,int line){if(!ok)throw std::runtime_error("layer onset regression line "+std::to_string(line));}
#define check(v) verify((v),__LINE__)
int main(){using namespace visual::tank;SequenceName left{"Hulk_RunAttack1_gesture",""},right{"Hulk_RunAttack2_gesture","ACT_TERROR_ATTACK_MOVING"},moving{"Attack_Moving","ACT_TERROR_ATTACK_MOVING"},idle{"ragdoll","ACT_DIERAGDOLL"};
 LayerAttackDetector d;auto sample=[&](float time,int seq,float cycle,float weight,SequenceName n){return d.update(time,true,{{0,seq,cycle,weight,n}});};
 check(!sample(1,0,0,0,idle).onset);auto a=sample(1.05f,43,.02f,1,left);check(a.onset&&a.active&&a.layer==0&&a.names.label==left.label);
 check(!sample(1.1f,43,.08f,1,left).onset);check(!sample(1.1f,43,.02f,1,left).onset);
 check(!d.update(1.15f,false,{}).onset);check(!sample(1.2f,43,.2f,1,left).onset);
 check(!sample(1.25f,43,.3f,.02f,left).onset);check(!sample(1.3f,43,.4f,.08f,left).onset);
 sample(1.35f,43,.8f,1,left);check(sample(1.4f,43,.01f,1,left).onset);
 check(sample(1.45f,45,.02f,1,right).onset);sample(1.5f,45,.9f,0,right);check(sample(1.55f,45,.02f,1,right).onset);
 // A weight dip does not turn the same progressing gesture into another punch.
 LayerAttackDetector dip;check(dip.update(1,true,{{0,43,.02f,1,left}}).onset);
 check(!dip.update(1.05f,true,{{0,43,.08f,0,left}}).onset);check(!dip.update(1.1f,true,{{0,43,.14f,1,left}}).onset);
 check(!dip.update(1.15f,true,{{0,43,.9f,0,left}}).onset);check(dip.update(1.2f,true,{{0,43,.02f,1,left}}).onset);
 // Layer numbers/sequence numbers do not identify an attack without its name.
 LayerAttackDetector unknown;check(!unknown.update(1,true,{{0,43,.02f,1,{}}}).onset);
 check(!unknown.update(1.1f,true,{{0,43,.1f,1,SequenceName{"Throw_04","ACT_TANK_OVERHEAD_THROW"}}}).onset);
 // Joining mid-swing and resuming after a gap must not create a full new window.
 LayerAttackDetector late;check(!late.update(1,true,{{0,43,.6f,1,left}}).onset);check(!late.update(2,true,{{0,43,.7f,1,left}}).onset);
 check(!late.update({},true,{{0,43,.01f,1,left}}).onset);check(!late.update(2.05f,true,{{0,43,.8f,1,left}}).onset);check(late.update(2.1f,true,{{0,43,.02f,1,left}}).onset);
 // Multiple attack layers beginning together count once; delayed blending does not restart.
 LayerAttackDetector blend;check(blend.update(1,true,{{0,43,.01f,1,left},{1,40,.01f,1,moving}}).onset);
 check(!blend.update(1.1f,true,{{0,43,.1f,1,left},{1,40,.1f,1,moving},{2,45,.01f,1,right}}).onset);
 check(!blend.update(1.2f,true,{{0,43,.2f,1,{}},{1,40,.2f,1,moving}}).onset);
 // Invalid weights and clock rollback cannot fabricate an onset.
 check(!blend.update(1.3f,true,{{0,43,.01f,2,left}}).onset);check(!blend.update(.1f,true,{{0,43,.6f,1,left}}).onset);
 // The runtime feeds an ordinary movement body plus a separate layer onset.
 SwingStopwatch watch;SequenceName run{"Run_4",""};watch.update(10,1,9,.5f,run,1.5f,1.f,false,false,.5f);
 auto r=watch.update(10.05f,2,9,.6f,run,1.5f,1.f,false,true,.5f);check(r.triggered&&r.eventId==1&&std::string(r.source)=="layer-observed-punch"&&r.remaining&&*r.remaining<2&&*r.remaining>1.9f);
 r=watch.update(10.1f,3,9,.7f,run,1.5f,1.f,false,false,.5f);check(!r.triggered&&r.eventId==1&&*r.remaining<1.91f&&*r.remaining>1.89f);
 r=watch.update(10.2f,4,49,.1f,SequenceName{"Throw_02","ACT_TANK_OVERHEAD_THROW"},1.5f,1.f);check(!r.remaining&&r.throwing);
 r=watch.update(11.2f,5,9,.2f,run,1.5f,1.f,false,true,.5f);check(r.triggered&&r.eventId==2&&r.remaining);
 SwingStopwatch missing;check(!missing.update(1,1,9,.1f,run,{}, {},false,true).remaining);
 std::cout<<"PASS: actual Tank gesture names, onset/cycle/sequence restarts, blend deduplication, stale/unknown/paused samples and automatic layer stopwatch integration\n";
}
