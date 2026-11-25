#include "first_person_body_config.hpp"
#include "dango/json.hpp"
#include <cassert>
int main(){
 using nlohmann::json;using namespace visual::firstperson;
 const json empty=json::object();
 // Factory defaults require an explicit user opt-in for native body display.
 const json baseline={{"firstPersonBody",{
  {"dynamicAnchor",true},{"enabled",false},
  {"fallbackEyeHeight",5.0},{"followCameraYaw",true},{"forward",-13.0},
  {"hideArms",true},{"hideHead",true},{"modelProfiles",json::array()},
  {"right",-1.0},{"step",1.0},{"up",-1.0},{"yawTrim",0.0},
  {"thirdPersonCompatibility",false}
 }}};
 Settings expected=readSettings(baseline),factory=readSettings(empty);
 assert(!factory.enabled&&!factory.thirdPersonCompatibility);
 assert(expected.enabled==factory.enabled && expected.dynamicAnchor==factory.dynamicAnchor);
 assert(expected.forward==factory.forward&&expected.right==factory.right&&expected.up==factory.up);
 assert(expected.hideArms==factory.hideArms&&expected.hideHead==factory.hideHead);
 assert(expected.followCameraYaw==factory.followCameraYaw);
 assert(factory.forward==-13.f && factory.right==-1.f && factory.up==-1.f);
 assert(factory.hideHead&&factory.hideArms&&factory.dynamicAnchor&&factory.followCameraYaw);

 const json invalid={{"firstPersonBody",{{"enabled","true"},{"forward",99999},{"clipHeight",-99},{"clipEnabled",true}}}};
 auto bad=readSettings(invalid);assert(!bad.enabled&&bad.forward==64.f);

 // Explicit old user preference is never overwritten by the new default.
 auto saved=json{{"firstPersonBody",{{"enabled",true},{"clipEnabled",true},{"clipHeight",71.f},
    {"forward",3.f},{"right",-2.f},{"up",6.f},{"tltpvCompatibility",true}}},{"tankPrediction",{{"enabled",false}}}};
 auto cfg=readSettings(saved);assert(cfg.enabled&&cfg.thirdPersonCompatibility);
 assert(cfg.forward==3.f&&cfg.right==-2.f&&cfg.up==6.f);
 assert(!adjust(cfg,862)&&!adjust(cfg,870)&&!adjust(cfg,875));
 assert(adjust(cfg,886)&&!cfg.thirdPersonCompatibility);
 assert(adjust(cfg,886)&&cfg.thirdPersonCompatibility);
 assert(adjust(cfg,876)&&cfg.forward==4.f);
 assert(adjust(cfg,883)&&cfg.forward==0&&cfg.right==0&&cfg.up==0);
 assert(adjust(cfg,882)&&cfg.step==5.f);
 assert(adjust(cfg,884)&&!cfg.hideHead);
 assert(adjust(cfg,885)&&!cfg.hideArms);
 assert(adjust(cfg,887)&&!cfg.dynamicAnchor);
 assert(adjust(cfg,888)&&!cfg.followCameraYaw);
 assert(adjust(cfg,889)&&cfg.fallbackEyeHeight==6);
 assert(adjust(cfg,891)&&cfg.yawTrim==5);
 assert(!adjust(cfg,860));
 assert(!cfg.dynamicAnchor2);assert(!adjust(cfg,896)&&!cfg.dynamicAnchor2);assert(adjust(cfg,MeanToggleAction)&&cfg.dynamicAnchor2);assert(!adjust(cfg,MeanResetAction));
 writeSettings(saved,cfg);
 assert(saved.at("tankPrediction").at("enabled")==false);
 assert(!saved.at("firstPersonBody").contains("clipEnabled"));
 assert(!saved.at("firstPersonBody").contains("clipHeight"));
 assert(!saved.at("firstPersonBody").contains("tltpvCompatibility"));
 assert(saved.at("firstPersonBody").at("thirdPersonCompatibility")==true);
 assert(readSettings(saved).thirdPersonCompatibility);
 assert(readSettings(saved).dynamicAnchor2);

 // If both legacy and neutral names exist, the neutral name has precedence.
 json conflicting={{"firstPersonBody",{{"tltpvCompatibility",true},{"thirdPersonCompatibility",false}}}};
 assert(!readSettings(conflicting).thirdPersonCompatibility);
 writeSettings(saved,Settings{});
 assert(!readSettings(saved).enabled&&!readSettings(saved).thirdPersonCompatibility);
}
