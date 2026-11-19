#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace visual::firstperson {
struct Settings {
 bool enabled=false,clipEnabled=false;
 float clipHeight=65.f;
 float forward=0.f,right=0.f,up=0.f,step=1.f;
};
inline void normalize(Settings& s) {
 auto clamp=[](float v,float lower,float upper,float fallback){return std::isfinite(v)?std::clamp(v,lower,upper):fallback;};
 s.clipHeight=clamp(s.clipHeight,0.f,128.f,65.f);
 s.forward=clamp(s.forward,-64.f,64.f,0.f);
 s.right=clamp(s.right,-64.f,64.f,0.f);
 s.up=clamp(s.up,-64.f,64.f,0.f);
 s.step=(s.step==.5f||s.step==1.f||s.step==5.f)?s.step:1.f;
}
template<class Json> Settings readSettings(const Json& root) {
 Settings s{};auto it=root.find("firstPersonBody");
 if(it==root.end()||!it->is_object())return s;
 auto readBool=[&](const char* key,bool& value){auto v=it->find(key);if(v!=it->end()&&v->is_boolean())value=v->template get<bool>();};
 auto readFloat=[&](const char* key,float& value){auto v=it->find(key);if(v!=it->end()&&v->is_number())value=v->template get<float>();};
 readBool("enabled",s.enabled);readBool("clipEnabled",s.clipEnabled);
 readFloat("clipHeight",s.clipHeight);readFloat("forward",s.forward);
 readFloat("right",s.right);readFloat("up",s.up);readFloat("step",s.step);
 normalize(s);return s;
}
template<class Json> void writeSettings(Json& root,const Settings& settings) {
 Settings s=settings;normalize(s);
 root["firstPersonBody"]=Json{{"enabled",s.enabled},{"clipEnabled",s.clipEnabled},
 {"clipHeight",s.clipHeight},{"forward",s.forward},{"right",s.right},{"up",s.up},{"step",s.step}};
}
inline bool adjust(Settings& s,std::uintptr_t id) {
 switch(id) {
 case 870:s.clipEnabled=!s.clipEnabled;break;
 case 871:s.clipHeight-=1.f;break;
 case 872:s.clipHeight+=1.f;break;
 case 873:s.clipHeight-=5.f;break;
 case 874:s.clipHeight+=5.f;break;
 case 875:s.clipHeight=65.f;break;
 case 876:s.forward+=s.step;break;
 case 877:s.forward-=s.step;break;
 case 878:s.right+=s.step;break;
 case 879:s.right-=s.step;break;
 case 880:s.up+=s.step;break;
 case 881:s.up-=s.step;break;
 case 882:s.step=s.step==.5f?1.f:s.step==1.f?5.f:.5f;break;
 case 883:s.forward=s.right=s.up=0;break;
 default:return false;
 }
 normalize(s);return true;
}
}
