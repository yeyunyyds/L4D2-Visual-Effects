#include "rock_prediction.hpp"
#include "rock_landing.hpp"
#include "rock_settings.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace visual::rocks;
using dango::inventory::Record;
Record rock(unsigned index=7,unsigned serial=2){Record r;r.index=index;r.serial=serial;r.handle=(serial<<12)|index;r.className="CTankRock";r.parent=0xffffffffu;r.networkOrigin=Vec3{0,0,100};r.velocity=Vec3{100,0,0};return r;}
void discover(Tracker& t,Record r,uint64_t tick=100,uint32_t epoch=1){dango::inventory::Frame f;f.epoch=epoch;f.tick=tick;f.records={r};t.discover(f);}
int main(){
 Tracker tracker;auto r=rock();discover(tracker,r);r.parent=123;tracker.observe(r,100,500);assert(tracker.tracks().at(r.handle).state==State::Held);assert(tracker.tracks().at(r.handle).trajectory.empty());
 r.parent=0xffffffffu;tracker.observe(r,110,500);auto t=tracker.tracks().at(r.handle);assert(t.state==State::Flight);assert(t.trajectory.size()==21);assert(std::abs(t.trajectory.back().position.z-60)<.001f);
 tracker.observe(r,120,500);assert(tracker.tracks().at(r.handle).motion->wallMs==110);tracker.observe(r,400,500);assert(tracker.tracks().at(r.handle).trajectory.empty());
 r.networkOrigin=Vec3{2,0,100};r.velocity.reset();tracker.observe(r,420,500);assert(tracker.tracks().at(r.handle).motion->velocityValid);
 auto reused=rock(7,3);discover(tracker,reused,430);assert(tracker.tracks().at(r.handle).state==State::Destroyed);tracker.expire(1000);assert(tracker.tracks().at(r.handle).state==State::Destroyed);
 auto second=rock(8);discover(tracker,second,440);tracker.observe(second,440,1000);assert(tracker.tracks().at(second.handle).trajectory.size()==41);
 tracker.unavailable(second.handle,false);assert(tracker.tracks().at(second.handle).trajectory.empty());tracker.observe(second,450,500);assert(!tracker.tracks().at(second.handle).trajectory.empty());
 second.networkOrigin=Vec3{std::numeric_limits<float>::quiet_NaN(),0,0};tracker.observe(second,460,500);assert(!tracker.tracks().at(second.handle).motion);
 tracker.reset(2);assert(tracker.tracks().empty());for(unsigned i=0;i<100;++i)discover(tracker,rock(i),500,2);assert(tracker.tracks().size()==64);tracker.expire(3000);assert(tracker.tracks().empty());
 auto grenade=rock();grenade.className="CBaseCSGrenadeProjectile";auto tank=rock(4);tank.className="Tank";dango::inventory::Frame f;f.records={tank,grenade};assert(identifyRock(grenade,f).kind==Identity::None);grenade.owner=tank.handle;assert(identifyRock(grenade,f).kind==Identity::TankOwnedSharedProjectile);
 auto survivor=rock(5);survivor.className="CTerrorPlayer";survivor.team=2;survivor.zombieClass=8;f.records.push_back(survivor);grenade.thrower=survivor.handle;assert(identifyRock(grenade,f).kind==Identity::None);survivor.team=3;f.records.back()=survivor;assert(identifyRock(grenade,f).kind==Identity::TankOwnedSharedProjectile);
 Sample sample;sample.position={0,0,100};sample.velocity={100,0,0};sample.velocityValid=true;sample.wallMs=100;unsigned budget=64;
 auto floor=[](Vec3 a,Vec3 b,dango::trace::Result& hit){hit.start=a;hit.end=b;if(b.z<=HalfSize){hit.fraction=(a.z-HalfSize)/(a.z-b.z);hit.end=a+(b-a)*hit.fraction;hit.plane.normal={0,0,1};}return true;};
 auto landing=predictLanding(sample,Gravity,100,budget,floor,HalfSize);assert(landing.valid());assert(std::abs(landing.centerPosition.z-HalfSize)<.001f);assert(std::abs(landing.position.z)<.001f);assert(landing.seconds<.79f);
 budget=0;assert(!predictLanding(sample,Gravity,100,budget,floor,HalfSize).valid());budget=64;assert(!predictLanding(sample,Gravity,100,budget,[](Vec3,Vec3,dango::trace::Result&){return false;},HalfSize).valid());
 for(int tick:{30,60,100}){Tracker sim;auto base=rock();discover(sim,base);for(int i=0;i<tick;++i){float dt=float(i)/tick;base.networkOrigin=ballistic({0,0,100},{100,0,100},Gravity,dt);base.velocity=Vec3{100,0,100-Gravity*dt};sim.observe(base,100+uint64_t(dt*1000),500);}assert(!sim.tracks().at(base.handle).trajectory.empty());}
 std::cout<<"PASS: rock identity, held/release, reuse, freshness, missing velocity, bounded state and fixed-model landing\n";
}
