#include "tank_cooldown.hpp"
#include "tank_model.hpp"
#include "dango/source_entity_contract.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cstring>
void check(bool b){if(!b)throw std::runtime_error("Tank cooldown regression");}
struct Memory{unsigned char bytes[8192]{};bool read(unsigned a,void* p,std::size_t n){if(!a||std::uint64_t(a)+n>sizeof(bytes))return false;std::memcpy(p,bytes+a,n);return true;}template<class T>void put(unsigned a,T v){std::memcpy(bytes+a,&v,sizeof(v));}void text(unsigned a,const char* s){std::memcpy(bytes+a,s,std::strlen(s)+1);}};
int main(){using namespace visual::tank;Frame f;f.epoch=1;f.tick=1000;Record tank;tank.className="Tank";tank.handle=0x1001;tank.activeWeapon=0x2002;tank.simulationTime=100.f;tank.nextAttack=101.5f;Record claw;claw.className="CTankClaw";claw.handle=0x2002;claw.weaponOwner=tank.handle;claw.nextPrimaryAttack=101.f;f.records={tank,claw};
 CombatClock clock;check(!clock.update(f).time);f.tick=1050;f.records[0].simulationTime=100.05f;auto time=clock.update(f);check(time.time&&*time.time>100);auto c=readCooldown(tank,f,time);check(c.remaining&&!c.partial&&std::abs(*c.remaining-1.45f)<.001f);
 f.records[0].simulationTime=100.8f;f.tick=1100;time=clock.update(f);check(std::abs(*readCooldown(tank,f,time).remaining-.7f)<.001f);
 // Never subtract Windows uptime from Source deadlines or extrapolate a paused clock.
 f.tick=1400;check(!clock.update(f).time);f.records[0].simulationTime=101;f.tick=1450;check(clock.update(f).time.has_value());
 time={102.f,tank.handle,"test"};check(readCooldown(tank,f,time).remaining==0.f);
 tank.nextAttack=0;time.time=100.5f;c=readCooldown(tank,f,time);check(c.partial&&c.remaining&&*c.remaining==.5f);time.time=102;check(!readCooldown(tank,f,time).remaining);
 f.records[1].nextPrimaryAttack=0;check(!readCooldown(tank,f,time).remaining);tank.nextAttack.reset();check(!readCooldown(tank,f,time).remaining);
 f.records[1].nextPrimaryAttack=1000;check(!readCooldown(tank,f,time).remaining);f.records[1].nextPrimaryAttack=std::numeric_limits<float>::quiet_NaN();check(!readCooldown(tank,f,time).remaining);
 tank.nextAttack=104;f.records[1].weaponOwner=0x3003;check(!readCooldown(tank,f,time).remaining);f.records[1].weaponOwner=tank.handle;f.records[1].handle=0x4002;check(!readCooldown(tank,f,time).remaining);
 f.epoch=2;f.records[0].simulationTime=5;f.tick=1500;check(!clock.update(f).time);f.records[0].simulationTime=5.05f;f.tick=1550;check(clock.update(f).time.has_value());f.tick=100;check(!clock.update(f).time);
 Memory m;using namespace dango::entitywire;Table root;root.props=1000;root.count=2;root.name=2000;m.text(2000,"DT_Claw");m.put(256,root);Table child;child.props=1200;child.count=1;child.name=2050;m.text(2050,"DT_LocalActiveWeaponData");m.put(320,child);Prop local;local.name=2100;local.type=6;local.table=320;local.offset=0;m.text(2100,"LocalActiveWeaponData");m.put(1000,local);Prop next;next.name=2150;next.type=1;next.offset=2400;m.text(2150,"m_flNextPrimaryAttack");m.put(1200,next);Prop sim;sim.name=2200;sim.type=0;sim.offset=336;m.text(2200,"m_flSimulationTime");m.put(1060,sim);
 Fields fields;check(fields.empty());check(dango::entitywire::fields(m,256,fields,true));check(fields.at("m_flNextPrimaryAttack").offset==2400&&fields.at("m_flSimulationTime").type==0);m.put(2400,103.5f);float deadline=0;check(load(m,fields.at("m_flNextPrimaryAttack").offset,deadline)&&deadline==103.5f);
 local.offset=4;m.put(1000,local);check(dango::entitywire::fields(m,256,fields,true)&&!fields.count("m_flNextPrimaryAttack"));local.offset=0;m.put(1000,local);next.type=0;m.put(1200,next);check(dango::entitywire::fields(m,256,fields,true)&&!fields.count("m_flNextPrimaryAttack"));next.type=1;m.put(1200,next);m.text(2050,"DT_UnverifiedProxy");check(dango::entitywire::fields(m,256,fields,true)&&!fields.count("m_flNextPrimaryAttack"));check(dango::entitywire::fields(m,256,fields,false)&&!fields.count("m_flNextPrimaryAttack"));
 // Both additional zero-offset tables use their own exact name/type contract.
 for(auto spec:{std::array<const char*,3>{"bcc_localdata","DT_BCCLocalPlayerExclusive","m_flNextAttack"},{"serveranimdata","DT_ServerAnimationData","m_flCycle"}}){
  m.text(2100,spec[0]);m.text(2050,spec[1]);m.text(2150,spec[2]);check(dango::entitywire::fields(m,256,fields,true)&&fields.at(spec[2]).offset==2400);
 }
 std::cout<<"PASS: nested timer schema, owner/handle gates, progressing Source clock, pause/map resets, partial/zero/missing fields and deadline countdown\n";
}
