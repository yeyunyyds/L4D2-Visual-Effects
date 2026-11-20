#include "entity_occlusion.hpp"
#include "tank_model.hpp"
#include "json.hpp"
#include <stdexcept>
#include <iostream>
using namespace visual;
void check(bool ok){if(!ok)throw std::runtime_error("Tank visibility regression");}
int main(){
 tank::Settings cfg;check(!cfg.enabled&&cfg.occludeHud);
 nlohmann::json j;tank::writeSettings(j,cfg);check(tank::readSettings(j).occludeHud);
 j["tankPrediction"]["occludeHud"]=false;check(!tank::readSettings(j).occludeHud);
 dango::inventory::Record t;t.className="Tank";t.networkOrigin=dango::Vec3{100,0,0};
 t.collisionMins=dango::Vec3{-20,-20,0};t.collisionMaxs=dango::Vec3{20,20,100};
 auto p=occlusion::points(t,true);check(p[0].z>p[1].z&&p[1].z>p[2].z);
 unsigned rays=0;
 auto yes=occlusion::bodyVisible(dango::Vec3{},t,[&](auto from,auto to,auto& hit){
  ++rays;hit.start=from;hit.end=to;hit.fraction=1;return true;
 },true);check(yes&&*yes&&rays==1);
 auto no=occlusion::bodyVisible(dango::Vec3{},t,[](auto from,auto to,auto& hit){
  hit.start=from;hit.end=from+(to-from)*.2f;hit.fraction=.2f;return true;
 },true);check(no&&!*no);
 auto unknown=occlusion::bodyVisible(dango::Vec3{},t,[](auto,auto,auto&){return false;},true);
 check(!unknown);
 auto part=occlusion::bodyVisible(dango::Vec3{},t,[](auto from,auto to,auto& hit){
  hit.start=from;hit.fraction=to.z>70?1.f:.3f;hit.end=from+(to-from)*hit.fraction;return true;
 },true);check(part&&*part);
 std::cout<<"PASS: Tank-only three-point LOS, invalid/blocked/partial, default occlusion\n";
}
