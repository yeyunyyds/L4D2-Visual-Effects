#pragma once
#include "animation.hpp"
namespace visual {
template<class Json> AnimationSettings readAnimationSettings(const Json& j){
 AnimationSettings a;
 // New defaults apply once when migrating old UI-only / five-speed settings.
 if(j.value("animationSettingsVersion",0)==1){a.periodMs=j.value("animationPeriodMs",500u);a.minTenths=j.value("animationMinTenths",5);a.maxTenths=j.value("animationMaxTenths",11);a.preset=j.value("animationPreset",0u);}
 return normalized(a);
}
template<class Json> void writeAnimationSettings(Json& j,const AnimationSettings& settings){
 auto a=normalized(settings);j["animationSettingsVersion"]=1;j["animationPeriodMs"]=a.periodMs;j["animationMinTenths"]=a.minTenths;j["animationMaxTenths"]=a.maxTenths;j["animationPreset"]=a.preset;
}
}
