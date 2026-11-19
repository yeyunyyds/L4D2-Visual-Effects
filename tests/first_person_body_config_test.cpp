#include "first_person_body_config.hpp"
#include "dango/json.hpp"
#include <cassert>
int main(){
 using nlohmann::json;
 const json empty=json::object();
 assert(!visual::firstperson::readSettings(empty).enabled);
 const json invalid={{"firstPersonBody",{{"enabled","true"}}}};
 assert(!visual::firstperson::readSettings(invalid).enabled);
 auto enabled=json{{"firstPersonBody",{{"enabled",true}}},{"tankPrediction",{{"enabled",false}}}};
 assert(visual::firstperson::readSettings(enabled).enabled);
 const auto cfg=visual::firstperson::readSettings(enabled);
 visual::firstperson::writeSettings(enabled,cfg);
 assert(enabled.at("firstPersonBody").at("enabled")==true);
 assert(enabled.at("tankPrediction").at("enabled")==false);
 visual::firstperson::writeSettings(enabled,visual::firstperson::Settings{});
 assert(enabled.at("firstPersonBody").at("enabled")==false);
}
