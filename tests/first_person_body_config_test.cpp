#include "first_person_body_config.hpp"
#include "dango/json.hpp"
#include <cassert>
int main(){
 using nlohmann::json;using namespace visual::firstperson;
 const json empty=json::object();
 assert(!readSettings(empty).enabled&&!readSettings(empty).clipEnabled);
 const json invalid={{"firstPersonBody",{{"enabled","true"},{"forward",99999},{"clipHeight",-99}}}};
 auto bad=readSettings(invalid);assert(!bad.enabled&&bad.forward==64.f&&bad.clipHeight==0.f);
 auto saved=json{{"firstPersonBody",{{"enabled",true},{"clipEnabled",true},{"clipHeight",71.f},{"forward",3.f},{"right",-2.f},{"up",6.f}}},{"tankPrediction",{{"enabled",false}}}};
 auto cfg=readSettings(saved);assert(cfg.enabled&&cfg.clipEnabled&&cfg.clipHeight==71.f);
 assert(cfg.forward==3.f&&cfg.right==-2.f&&cfg.up==6.f);
 assert(adjust(cfg,870)&&!cfg.clipEnabled);
 assert(adjust(cfg,876)&&cfg.forward==4.f);
 assert(adjust(cfg,883)&&cfg.forward==0&&cfg.right==0&&cfg.up==0);
 assert(adjust(cfg,882)&&cfg.step==5.f);
 assert(adjust(cfg,873)&&cfg.clipHeight==66.f);
 assert(!adjust(cfg,860));
 writeSettings(saved,cfg);
 assert(saved.at("tankPrediction").at("enabled")==false);
 assert(readSettings(saved).clipHeight==66.f);
 writeSettings(saved,Settings{});
 assert(!readSettings(saved).enabled&&!readSettings(saved).clipEnabled);
}
