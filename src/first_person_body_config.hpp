#pragma once
// Fail-closed, independent user preference for an ABI-unverified experiment.
namespace visual::firstperson {
struct Settings { bool enabled=false; };
template<class Json> Settings readSettings(const Json& root) {
 Settings s{};
 auto it=root.find("firstPersonBody");
 if(it!=root.end()&&it->is_object()) {
  auto enabled=it->find("enabled");
  if(enabled!=it->end()&&enabled->is_boolean())s.enabled=enabled->template get<bool>();
 }
 return s;
}
template<class Json> void writeSettings(Json& root,const Settings& settings) {
 root["firstPersonBody"]=Json{{"enabled",settings.enabled}};
}
}
