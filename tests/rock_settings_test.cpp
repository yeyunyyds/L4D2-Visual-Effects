#include "rock_settings.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace visual::rocks;using nlohmann::json;
 auto defaults=readSettings(json::object());assert(!defaults.enabled&&defaults.trajectory&&defaults.landingCircle&&defaults.horizonMs==500&&defaults.landingOpacity==.5f);
 json legacy={{"tankRockDisplay",{{"errors",true}}},{"tankRockMelee",{{"enabled",true}}},{"tankPrediction",{{"enabled",true}}}};assert(!readSettings(legacy).enabled);
 auto cfg=defaults;cfg.enabled=true;cfg.opacity=.4f;cfg.lineWidth=4;cfg.landingRadius=48;writeSettings(legacy,cfg);auto back=readSettings(legacy);assert(back.enabled&&back.opacity==.4f&&back.lineWidth==4&&back.landingRadius==48);assert(legacy["tankPrediction"]["enabled"]==true);
 auto bad=readSettings({{"tankRockPrediction",{{"enabled","yes"},{"trajectory",1},{"horizonMs",-500},{"color",99},{"opacity",9},{"lineWidth",-1},{"landingRadius",500},{"landingOpacity",0}}}});assert(!bad.enabled&&bad.trajectory&&bad.horizonMs==100&&bad.color==2&&bad.opacity==1&&bad.lineWidth==1&&bad.landingRadius==96&&bad.landingOpacity==.1f);
 assert(nextHorizon(500)==750&&nextHorizon(1000)==100);std::cout<<"PASS: release rock defaults, namespace migration, bounded display settings and save roundtrip\n";
}
