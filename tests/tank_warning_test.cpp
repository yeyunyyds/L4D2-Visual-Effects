#include "tank_warning.hpp"
#include "tank_warning_geometry.hpp"
#include "dango/hud_menu_policy.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace visual::tank;
void check(bool b){if(!b)throw std::runtime_error("Tank range warning regression");}
struct Memory{std::array<unsigned char,8192> bytes{};bool read(std::uint32_t at,void* out,std::size_t n){if(!at||std::uint64_t(at)+n>bytes.size())return false;std::memcpy(out,bytes.data()+at,n);return true;}template<class T>void put(unsigned at,const T& v){std::memcpy(bytes.data()+at,&v,sizeof(v));}void text(unsigned at,const char* v){std::memcpy(bytes.data()+at,v,std::strlen(v)+1);}};
int main(){
 Settings s;check(!warningRequested(s));nlohmann::json j;s.warnBar=s.warnRange=s.warnFill=s.warnOutline=true;writeSettings(j,s);auto saved=readSettings(j);check(saved.warnBar&&saved.warnRange&&saved.warnFill&&saved.warnOutline&&!saved.enabled);check(dango::hud::opensChild(328)&&!dango::hud::opensChild(334)&&!dango::hud::opensChild(337));
 Record actor;actor.className="Tank";actor.handle=4097;actor.index=1;actor.zombieClass=8;actor.networkOrigin=Vec3{};actor.eyeAngles=Vec3{};actor.health=6000;
 Record player;player.className="CTerrorPlayer";player.handle=8194;player.index=2;player.team=2;player.health=100;player.networkOrigin=Vec3{50,0,0};
 Observation o;o.geometry=true;o.origin={};o.radius=100;o.halfArc=45;o.heading=0;
 auto warning=rangeWarning(o,&player,&actor);check(warning.known&&warning.inside&&warning.distance==50);
 player.networkOrigin=Vec3{-50,0,0};check(!rangeWarning(o,&player,&actor).inside);player.networkOrigin=Vec3{50,50,0};check(rangeWarning(o,&player,&actor).inside);
 player.networkOrigin=Vec3{100,0,0};check(rangeWarning(o,&player,&actor).inside);player.networkOrigin->x=100.01f;check(!rangeWarning(o,&player,&actor).inside);
 player.networkOrigin=Vec3{50,0,500};check(!rangeWarning(o,&player,&actor).inside);player.networkOrigin=Vec3{50,0,0};player.health=0;check(!rangeWarning(o,&player,&actor).known);player.health=100;player.team=1;check(!rangeWarning(o,&player,&actor).known);player.team=2;
 player.collisionMins=Vec3{};player.collisionMaxs=Vec3{16,16,72};actor.collisionMins=Vec3{};actor.collisionMaxs=Vec3{32,32,100};player.networkOrigin->z=101;check(!rangeWarning(o,&player,&actor).inside);player.networkOrigin->z=-50;check(rangeWarning(o,&player,&actor).inside);player.networkOrigin->z=0;
 check(!rangeWarning(o,nullptr,&actor).known);o.geometry=false;check(!rangeWarning(o,&player,&actor).known);o.geometry=true;o.cooldown=2;check(rangeWarning(o,&player,&actor).inside);o.cooldown.reset();check(rangeWarning(o,&player,&actor).inside);
 // Actual local handle, not camera position or the Tank's inferred target. Per-Tank independent.
 Frame f;f.epoch=1;f.tick=1000;auto far=actor;far.handle=12291;far.index=3;far.networkOrigin=Vec3{500,0,0};f.records={actor,player,far};Parameters p;p.range=100;p.arc=90;p.tick=1000;Predictor predictor;
 auto result=predictor.update(f,p,s,player.handle);check(result.tanks.size()==2&&result.tanks[0].warning.inside&&!result.tanks[1].warning.inside);
 check(!predictor.update(f,p,s,player.handle+4096).tanks[0].warning.known);f.epoch=2;f.records[1].health=0;check(!predictor.update(f,p,s,player.handle).tanks[0].warning.known);f.records[1].health=100;p.tick=0;f.tick=5000;check(!predictor.update(f,p,s,player.handle).tanks[0].warning.known);
 GlowLease lease;check(!lease.acquire({2,1,2,3}));check(lease.acquire({0,100,0,123}));auto own=lease.owned(lease.written);check(own[0]&&own[1]&&own[2]&&own[3]);auto foreign=lease.written;foreign[0]=3;foreign[3]=12;own=lease.owned(foreign);check(!own[0]&&own[1]&&own[2]&&!own[3]);
 using namespace dango::entitywire;Memory m;Table root;root.props=1024;root.count=1;root.name=2048;m.text(2048,"DT_BaseEntity");m.put(256,root);Prop glow;glow.name=2080;glow.type=6;glow.offset=400;glow.table=320;m.text(2080,"m_Glow");m.put(1024,glow);Table table;table.props=1200;table.count=4;table.name=2112;m.text(2112,"DT_GlowProperty");m.put(320,table);
 const char* names[]={"m_iGlowType","m_nGlowRange","m_nGlowRangeMin","m_glowColorOverride"};for(unsigned i=0;i<4;++i){Prop prop;prop.name=2200+i*64;prop.type=0;prop.offset=int(4+i*4);m.text(prop.name,names[i]);m.put(1200+i*sizeof(Prop),prop);}
 auto layout=glowLayout(m,256);check(bool(layout)&&layout.property==400&&layout.type==404);Prop bad;check(load(m,1200,bad));bad.offset=8;m.put(1200,bad);check(!glowLayout(m,256));bad.offset=4;bad.type=1;m.put(1200,bad);check(!glowLayout(m,256));
 // Red warning persists even with unknown cooldown; ordinary palette is unchanged.
 std::vector<Segment> normal,red;VisualSettings style;barGeometry(normal,{100,100},style,1,{},false);barGeometry(red,{100,100},style,1,{},true);check(red.size()==normal.size()&&red.front().color==0xff601515&&red.front().color!=normal.front().color);
 dango::GameCameraFrame camera;camera.worldToClip={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 auto polygon=warningTriangle(camera,800,600,{Vec3{-2,0,.5f},Vec3{.5f,2,.5f},Vec3{.5f,-.5f,.5f}});check(polygon.size()>=3);for(auto v:polygon)check(std::isfinite(v.x)&&std::isfinite(v.y)&&v.x>=-.01f&&v.x<=800.01f&&v.y>=-.01f&&v.y<=600.01f);
 check(warningTriangle(camera,800,600,{Vec3{0,0,-1},Vec3{1,0,-1},Vec3{0,1,-1}}).empty());
 camera.worldToClip[15]=0;camera.worldToClip[14]=1;polygon=warningTriangle(camera,800,600,{Vec3{-.01f,0,-1},Vec3{.1f,.1f,1},Vec3{-.1f,-.1f,1}});check(polygon.size()>=3);for(auto v:polygon)check(std::isfinite(v.x)&&std::isfinite(v.y));
 std::cout<<"PASS: local survivor sector/height checks, distance boundaries, per-Tank state, cooldown independence, stale/dead/missing gates, opt-in config, native glow schema and field ownership, red bar and clipped fill\n";
}
