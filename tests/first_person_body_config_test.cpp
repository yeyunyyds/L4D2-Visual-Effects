#include "first_person_body_config.hpp"
#include "dango/json.hpp"
#include <cassert>
int main(){
 using nlohmann::json;using namespace visual::firstperson;
 const json empty=json::object();
 // The supplied real-game configuration is the new factory baseline.
 const json baseline={{"firstPersonBody",{
  {"clipEnabled",false},{"clipHeight",81.0},{"dynamicAnchor",true},{"enabled",true},
  {"fallbackEyeHeight",5.0},{"followCameraYaw",true},{"forward",-13.0},
  {"hideArms",true},{"hideHead",true},{"modelProfiles",json::array()},
  {"right",-1.0},{"step",1.0},{"up",-1.0},{"yawTrim",0.0}
 }}};
 Settings expected=readSettings(baseline);
 Settings factory=readSettings(empty);
 assert(expected.enabled==factory.enabled && expected.dynamicAnchor==factory.dynamicAnchor);
 assert(expected.forward==factory.forward&&expected.right==factory.right&&expected.up==factory.up);
 assert(expected.hideArms==factory.hideArms&&expected.hideHead==factory.hideHead);
 assert(expected.followCameraYaw==factory.followCameraYaw&&expected.clipHeight==factory.clipHeight);

 assert(readSettings(empty).enabled&&!readSettings(empty).clipEnabled);
 assert(!readSettings(empty).tltpvCompatibility);
 assert(readSettings(empty).clipHeight==81.f && readSettings(empty).forward==-13.f);
 assert(readSettings(empty).right==-1.f && readSettings(empty).up==-1.f);
 assert(readSettings(empty).hideHead && readSettings(empty).hideArms);
 assert(readSettings(empty).dynamicAnchor && readSettings(empty).followCameraYaw);
 const json invalid={{"firstPersonBody",{{"enabled","true"},{"forward",99999},{"clipHeight",-99}}}};
 auto bad=readSettings(invalid);assert(bad.enabled&&bad.forward==64.f&&bad.clipHeight==0.f);
 auto saved=json{{"firstPersonBody",{{"enabled",true},{"clipEnabled",true},{"clipHeight",71.f},{"forward",3.f},{"right",-2.f},{"up",6.f}}},{"tankPrediction",{{"enabled",false}}}};
 auto cfg=readSettings(saved);assert(cfg.enabled&&cfg.clipEnabled&&cfg.clipHeight==71.f);
 assert(cfg.forward==3.f&&cfg.right==-2.f&&cfg.up==6.f);
 assert(adjust(cfg,870)&&!cfg.clipEnabled);
 assert(adjust(cfg,886)&&cfg.tltpvCompatibility);
 assert(adjust(cfg,876)&&cfg.forward==4.f);
 assert(adjust(cfg,883)&&cfg.forward==0&&cfg.right==0&&cfg.up==0);
 assert(adjust(cfg,882)&&cfg.step==5.f);
 assert(adjust(cfg,873)&&cfg.clipHeight==66.f);
 assert(adjust(cfg,884)&&!cfg.hideHead);
 assert(adjust(cfg,885)&&!cfg.hideArms);
 assert(adjust(cfg,887)&&!cfg.dynamicAnchor);
 assert(adjust(cfg,888)&&!cfg.followCameraYaw);
 assert(adjust(cfg,889)&&cfg.fallbackEyeHeight==6);
 assert(adjust(cfg,891)&&cfg.yawTrim==5);
 assert(!adjust(cfg,860));
 writeSettings(saved,cfg);
 assert(saved.at("tankPrediction").at("enabled")==false);
 assert(readSettings(saved).clipHeight==66.f);
 assert(readSettings(saved).tltpvCompatibility);
 writeSettings(saved,Settings{});
 assert(readSettings(saved).enabled&&!readSettings(saved).clipEnabled);
 assert(!readSettings(saved).tltpvCompatibility);
}
