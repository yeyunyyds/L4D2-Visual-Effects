#include "tank_model.hpp"
#include "tank_cvar_contract.hpp"
#include "tank_sequence.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok){if(!ok)throw std::runtime_error("Tank prediction regression");}
struct Memory{std::vector<unsigned char> bytes=std::vector<unsigned char>(16384);bool read(std::uint32_t address,void* out,std::size_t n){if(!address||std::uint64_t(address)+n>bytes.size())return false;std::memcpy(out,bytes.data()+address,n);return true;}template<class T>void put(unsigned address,T value){std::memcpy(bytes.data()+address,&value,sizeof(value));}void text(unsigned address,const char* s){std::memcpy(bytes.data()+address,s,std::strlen(s)+1);}};
int main(){using namespace visual::tank;using dango::inventory::Record;using dango::inventory::Frame;
 nlohmann::json j;check(!readSettings(j).enabled);Settings cfg;cfg.enabled=true;cfg.horizonMs=200;writeSettings(j,cfg);check(readSettings(j).enabled&&readSettings(j).horizonMs==200);
 Record t;t.className="Tank";t.handle=0x1001;t.index=1;t.networkOrigin=dango::Vec3{};t.networkAngles=dango::Vec3{};t.eyeAngles=dango::Vec3{0,90,0};t.team=3;t.zombieClass=8;t.health=6000;
 Record a;a.className="CTerrorPlayer";a.handle=0x2002;a.index=2;a.team=2;a.character=6;a.health=100;a.networkOrigin=dango::Vec3{100,0,0};auto b=a;b.handle=0x3003;b.index=3;b.character=4;b.networkOrigin=dango::Vec3{0,100,0};
 Frame f;f.epoch=1;f.tick=1000;f.records={t,a,b};Parameters p;p.range=56;p.arc=180;p.tick=f.tick;Predictor predictor;
 auto out=predictor.update(f,p,cfg);check(out.tanks.size()==1&&out.tanks[0].eye&&out.tanks[0].heading==90&&out.tanks[0].target&&out.tanks[0].target->handle==b.handle&&out.tanks[0].geometry&&out.tanks[0].radius==56);
 // Network look-at is displayed as look-at, never promoted to an AI enemy claim.
 f.records[0].lookAtHandle=a.handle;out=predictor.update(f,p,cfg);check(out.tanks[0].target->handle==a.handle&&!out.tanks[0].target->authoritativeField&&out.tanks[0].target->source=="network-look-at");
 f.records[0].enemyHandle=b.handle;out=predictor.update(f,p,cfg);check(out.tanks[0].target->handle==b.handle&&out.tanks[0].target->authoritativeField);
 f.records[0].enemyHandle.reset();for(unsigned time:{1050u,1100u,1150u,1200u}){f.tick=time;p.tick=time;out=predictor.update(f,p,cfg);}check(out.tanks[0].headingStalled&&out.tanks[0].radius==56&&out.tanks[0].halfArc==90);
 // A near-vertical aim makes yaw unsuitable for ground lock/range claims.
 f.records[0].eyeAngles=dango::Vec3{273.1640625f,90,0};for(unsigned time:{1250u,1300u,1350u,1400u}){f.tick=time;p.tick=time;out=predictor.update(f,p,cfg);}
 check(out.tanks[0].nearVerticalAim&&!out.tanks[0].headingStalled&&!out.tanks[0].geometry&&std::abs(out.tanks[0].pitch+86.8359375f)<.001f);
 f.records[0].eyeAngles=dango::Vec3{0,90,0};f.tick=1450;p.tick=f.tick;out=predictor.update(f,p,cfg);check(out.tanks[0].geometry);
 // Runtime updates alter range; no parameter availability means no invented ring.
 p.range=84;out=predictor.update(f,p,cfg);check(out.tanks[0].radius==84);p.range.reset();out=predictor.update(f,p,cfg);check(!out.tanks[0].geometry);p.range=84;p.tick=0;f.tick=5000;out=predictor.update(f,p,cfg);check(!out.tanks[0].geometry&&!out.tanks[0].headingStalled);
 // A reused entity index with a new serial cannot resolve an old target handle.
 f.records[1].handle=0x4002;f.records[0].lookAtHandle=a.handle;check(!resolveTarget(f.records[0],f));
 f.records[0].eyeAngles.reset();check(aim(f.records[0])&&!aim(f.records[0])->eye);f.records[0].networkAngles.reset();check(!aim(f.records[0]));
 f.epoch=2;f.tick=5100;out=predictor.update(f,p,cfg);check(!out.tanks[0].headingStalled);f.records[0].health=0;check(predictor.update(f,p,cfg).tanks.empty());f.records[0].health=6000;f.records[0].dormant=true;check(predictor.update(f,p,cfg).tanks.empty());
 t.eyeAngles=dango::Vec3{};a.networkOrigin=dango::Vec3{100,5,0};b.networkOrigin=dango::Vec3{100,-5,0};f.records={t,a,b};check(!inferTarget(t,f,0,0));
 Memory m;unsigned object=512;unsigned words[13]{};words[0]=100;words[3]=2000;words[5]=1u<<13;words[6]=100;words[7]=object;words[8]=2100;words[9]=2200;words[10]=16;float raw=56;std::memcpy(words+11,&raw,4);std::memcpy(m.bytes.data()+object,words,sizeof(words));m.text(2000,"tank_swing_range");m.text(2100,"56");m.text(2200,"56");
 auto value=readCvar(m,object,"tank_swing_range",1,512);check(value&&value->value==56);raw=84;m.put(object+44,raw);m.text(2200,"84");check(readCvar(m,object,"tank_swing_range",1,512)->value==84);m.text(2200,"56");check(!readCvar(m,object,"tank_swing_range",1,512));m.text(2200,"nan");check(!readCvar(m,object,"tank_swing_range",1,512));m.text(2200,"84");m.put(object+20,0u);check(!readCvar(m,object,"tank_swing_range",1,512));check(!readCvar(m,16000,"tank_swing_range",1,512));
 unsigned header=4096;m.put(header,0x54534449u);m.put(header+4,48);m.put(header+76,2000);m.put(header+188,2);m.put(header+192,256);
 for(int i=0;i<2;++i){unsigned address=header+256+unsigned(i)*212;m.put(address,-(256+i*212));m.put(address+4,int(header+1000+unsigned(i)*128-address));m.put(address+8,int(header+1300+unsigned(i)*128-address));}
 m.text(header+1000,"Idle");m.text(header+1300,"ACT_IDLE");m.text(header+1128,"Throw_Rock");m.text(header+1428,"ACT_RANGE_ATTACK1");auto seq=sequenceName(m,header,1);check(seq&&seq->label=="Throw_Rock"&&classify(*seq)=="throw-animation");check(!sequenceName(m,header,2));m.put(header+256+212,0);check(!sequenceName(m,header,1));
 // Combat fields are opt-in and only the explicitly embedded collision table is walked.
 using namespace dango::entitywire;Memory schema;Table root;root.props=4000;root.count=3;root.name=3000;schema.text(3000,"DT_TestTank");schema.put(256,root);
 Table collision;collision.props=4300;collision.count=2;collision.name=3032;schema.text(3032,"DT_CollisionProperty");schema.put(320,collision);
 Prop cp;cp.name=3100;cp.type=6;cp.offset=200;cp.table=320;schema.text(3100,"m_Collision");schema.put(4000,cp);
 Prop target;target.name=3132;target.type=0;target.offset=100;schema.text(3132,"m_lookatPlayer");schema.put(4060,target);
 Prop eye;eye.name=3164;eye.type=1;eye.offset=120;schema.text(3164,"m_angEyeAngles[1]");schema.put(4120,eye);
 Prop mins;mins.name=3200;mins.type=2;mins.offset=8;schema.text(3200,"m_vecMins");schema.put(4300,mins);Prop maxs=mins;maxs.name=3232;maxs.offset=20;schema.text(3232,"m_vecMaxs");schema.put(4360,maxs);
 Fields fields;check(dango::entitywire::fields(schema,256,fields)&&fields.empty());check(dango::entitywire::fields(schema,256,fields,true));auto hints=combatSchemaHints(schema,256);check(std::any_of(hints.begin(),hints.end(),[](const auto& s){return s.find("m_lookatPlayer type=0")!=std::string::npos;}));check(fields.at("m_Collision.m_vecMins").offset==208&&fields.at("m_Collision.m_vecMaxs").offset==220&&fields.at("m_lookatPlayer").offset==100&&fields.at("m_angEyeAngles[1]").type==1);
 schema.text(3032,"DT_UnknownProxy");check(!dango::entitywire::fields(schema,256,fields,true));
 std::cout<<"PASS: runtime reach updates, aim/body separation, look-at/enemy distinction, ambiguity and handle reuse, stationary-heading observation, bounded prediction and read-only ConVar/studio validation\n";
}
