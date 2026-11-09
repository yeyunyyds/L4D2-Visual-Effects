#pragma once
#include "dango/json.hpp"
#include <algorithm>
#include <cmath>
namespace visual::rocks {
struct Settings {
 bool enabled=false,trajectory=true,landingCircle=true;
 unsigned horizonMs=500,color=0;float opacity=.8f,lineWidth=2,landingRadius=32,landingOpacity=.5f;
};
inline Settings sanitize(Settings s){
 s.horizonMs=std::clamp(s.horizonMs,100u,1000u);s.color=std::min(s.color,2u);
 auto number=[](float x,float fallback,float lo,float hi){return std::isfinite(x)?std::clamp(x,lo,hi):fallback;};
 s.opacity=number(s.opacity,.8f,.1f,1);s.lineWidth=number(s.lineWidth,2,1,5);s.landingRadius=number(s.landingRadius,32,8,96);s.landingOpacity=number(s.landingOpacity,.5f,.1f,.8f);return s;
}
inline Settings readSettings(const nlohmann::json& root){
 Settings s;auto it=root.find("tankRockPrediction");if(it==root.end()||!it->is_object())return s;const auto& j=*it;
 auto boolean=[&](const char* k,bool d){return j.contains(k)&&j[k].is_boolean()?j[k].get<bool>():d;};
 auto number=[&](const char* k,float d){return j.contains(k)&&j[k].is_number()?j[k].get<float>():d;};
 // Only the new namespace can enable the feature. Demo switches/knife tuning are ignored.
 s.enabled=boolean("enabled",false);s.trajectory=boolean("trajectory",true);s.landingCircle=boolean("landingCircle",true);
 float horizon=number("horizonMs",500),color=number("color",0);s.horizonMs=std::isfinite(horizon)?unsigned(std::clamp(horizon,100.f,1000.f)):500;s.color=std::isfinite(color)?unsigned(std::clamp(color,0.f,2.f)):0;
 s.opacity=number("opacity",.8f);s.lineWidth=number("lineWidth",2);s.landingRadius=number("landingRadius",32);s.landingOpacity=number("landingOpacity",.5f);return sanitize(s);
}
inline void writeSettings(nlohmann::json& root,const Settings& raw){auto s=sanitize(raw);root["tankRockPrediction"]={{"enabled",s.enabled},{"trajectory",s.trajectory},{"landingCircle",s.landingCircle},{"horizonMs",s.horizonMs},{"color",s.color},{"opacity",s.opacity},{"lineWidth",s.lineWidth},{"landingRadius",s.landingRadius},{"landingOpacity",s.landingOpacity}};}
inline unsigned nextHorizon(unsigned h){for(unsigned value:{100u,200u,300u,500u,750u,1000u})if(value>h)return value;return 100;}
}
