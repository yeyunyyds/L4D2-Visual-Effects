#include "special_glow_settings.hpp"
#include "entity_occlusion.hpp"
#include "json.hpp"
#include <stdexcept>
#include <iostream>
using namespace visual;
static void check(bool good){if(!good)throw std::runtime_error("special glow / occlusion regression");}
int main(){
 specialglow::Settings cfg;
 check(!cfg.any()&&!cfg.usesVisibility());
 for(unsigned i=0;i<SpecialCount;++i)check(!cfg.types[i].enabled&&!cfg.types[i].occlude&&cfg.types[i].color==specialglow::defaults[i]);
 cfg.types[0].enabled=true;cfg.types[0].occlude=true;cfg.types[0].color=specialglow::rgb(17,150,255);
 cfg.types[7].enabled=true;cfg.types[7].color=specialglow::rgb(100,20,1);
 check(cfg.any()&&cfg.usesVisibility());
 nlohmann::json saved;specialglow::writeSettings(saved,cfg);
 auto decoded=specialglow::readSettings(saved);
 check(decoded.types[0].enabled&&decoded.types[0].occlude&&decoded.types[0].color==specialglow::rgb(17,150,255));
 check(decoded.types[7].enabled&&!decoded.types[7].occlude&&decoded.types[7].color==specialglow::rgb(100,20,1));
 saved["specialGlow"]["Smoker"]["color"]={-1,999,2};
 check(specialglow::readSettings(saved).types[0].color==specialglow::defaults[0]);
 check(specialglow::stepChannel(specialglow::rgb(250,0,0),0,+16)==specialglow::rgb(255,0,0));
 check(specialglow::stepChannel(specialglow::rgb(250,0,0),1,-16)==specialglow::rgb(250,0,0));
 for(unsigned i=0;i<SpecialCount;++i){
  dango::inventory::Record record;record.className=SpecialNames[i];record.handle=4097;record.index=1;record.serial=1;record.networkOrigin=dango::Vec3{100,0,0};
  check(specialglow::eligible(record));
  record.dormant=true;check(!specialglow::eligible(record));record.dormant=false;
  record.health=0;check(!specialglow::eligible(record));
 }
 dango::inventory::Record tank;tank.className="Tank";tank.handle=4097;tank.index=1;tank.serial=1;tank.networkOrigin=dango::Vec3{100,0,0};
 tank.collisionMins=dango::Vec3{-20,-20,0};tank.collisionMaxs=dango::Vec3{20,20,100};
 auto samples=occlusion::points(tank,true);
 check(samples[0].z>samples[1].z&&samples[1].z>samples[2].z);
 unsigned rays=0;dango::Vec3 eye{};
 auto fullyVisible=occlusion::bodyVisible(eye,tank,[&](auto from,auto to,auto& hit){
  ++rays;hit.start=from;hit.end=to;hit.fraction=1;return true;
 },true);
 check(fullyVisible&&*fullyVisible&&rays==1);
 auto blocked=occlusion::bodyVisible(eye,tank,[&](auto from,auto to,auto& hit){
  hit.start=from;hit.end=from+(to-from)*.4f;hit.fraction=.4f;return true;
 },true);
 check(blocked&&!*blocked);
 auto partial=occlusion::bodyVisible(eye,tank,[&](auto from,auto to,auto& hit){
  hit.start=from;hit.fraction=to.z>70?1.f:.2f;hit.end=from+(to-from)*hit.fraction;return true;
 },true);
 check(partial&&*partial);
 auto unknown=occlusion::bodyVisible(eye,tank,[](auto,auto,auto&){return false;},true);
 check(!unknown);
 std::cout<<"PASS: 8 independent roles, RGB config/validation, per-role occlusion, 3-sample body LOS fail-closed\n";
}
