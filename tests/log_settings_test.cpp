#include "log_settings.hpp"
#include "dango/json.hpp"
#include <cassert>
using nlohmann::json;

int main() {
 using namespace visual::logging;
 const json empty=json::object();
 assert(readLevel(empty)==Level::Info);
 assert(readLevel(json::object({{"logging",json::object({{"level","info"}})}}))==Level::Info);
 assert(readLevel(json::object({{"logging",json::object({{"level","debug"}})}}))==Level::Debug);
 assert(readLevel(json::object({{"logging",json::object({{"level","DEBUG"}})}}))==Level::Info);
 assert(readLevel(json::object({{"logging",json::object({{"level",true}})}}))==Level::Info);
 assert(readLevel(json::object({{"logging","debug"}}))==Level::Info);
 assert(readLevel(json::array())==Level::Info);

 level.store(Level::Info);
 assert(!periodicDebugEnabled());
 assert(toggle()==Level::Debug&&periodicDebugEnabled());
 json saved={{"firstPersonBody",{{"enabled",true}}},{"tankPrediction",{{"enabled",false}}}};
 writeSettings(saved);
 assert(saved.at("logging").at("level")=="debug");
 assert(saved.at("firstPersonBody").at("enabled")==true);
 assert(saved.at("tankPrediction").at("enabled")==false);
 assert(readLevel(saved)==Level::Debug);
 assert(toggle()==Level::Info&&!periodicDebugEnabled());
 writeSettings(saved);
 assert(saved.at("logging").at("level")=="info");
 assert(readLevel(saved)==Level::Info);
}
