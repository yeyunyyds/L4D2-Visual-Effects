#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
namespace visual::firstperson {
struct ModelProfile {
 std::string model;
 std::uint32_t checksum=0;
 float forward=0,right=0,up=0,yaw=0;
};
inline bool validModelKey(const std::string& name){
 return !name.empty()&&name.size()<=180&&std::all_of(name.begin(),name.end(),[](unsigned char c){return c>=32 && c!=127;});
}
struct Settings {
 bool enabled=false,clipEnabled=false,hideHead=true,hideArms=false;
 // Deliberately OFF until tested in-game; no change to the proven body gate.
 bool dynamicAnchor=false,followCameraYaw=false;
 float clipHeight=65.f,forward=0,right=0,up=0,step=1;
 float fallbackEyeHeight=5.f,yawTrim=0.f;
 std::vector<ModelProfile> modelProfiles;
};
inline void normalize(Settings& s){
 auto clamp=[](float v,float lower,float upper,float fallback){return std::isfinite(v)?std::clamp(v,lower,upper):fallback;};
 s.clipHeight=clamp(s.clipHeight,0.f,128.f,65.f);
 s.forward=clamp(s.forward,-64.f,64.f,0.f);
 s.right=clamp(s.right,-64.f,64.f,0.f);
 s.up=clamp(s.up,-64.f,64.f,0.f);
 s.step=(s.step==.5f||s.step==1.f||s.step==5.f)?s.step:1.f;
 s.fallbackEyeHeight=clamp(s.fallbackEyeHeight,-8.f,24.f,5.f);
 s.yawTrim=clamp(s.yawTrim,-40.f,40.f,0.f);
 std::vector<ModelProfile> clean;
 for(auto p:s.modelProfiles){
  if(!validModelKey(p.model))continue;
  p.forward=clamp(p.forward,-64,64,0);
  p.right=clamp(p.right,-64,64,0);
  p.up=clamp(p.up,-64,64,0);
  p.yaw=clamp(p.yaw,-40,40,0);
  bool already=false;
  for(const auto& e:clean)if(e.model==p.model&&e.checksum==p.checksum){already=true;break;}
  if(!already && clean.size()<64)clean.push_back(std::move(p));
 }
 s.modelProfiles=std::move(clean);
}
inline const ModelProfile* profile(const Settings& s,const std::string& key,std::uint32_t checksum) {
 for(const auto& p:s.modelProfiles)if(p.model==key&&p.checksum==checksum)return &p;
 return nullptr;
}
inline bool storeForModel(Settings& s,const std::string& key,std::uint32_t checksum){
 if(!validModelKey(key))return false;
 auto* found=const_cast<ModelProfile*>(profile(s,key,checksum));
 if(!found){
  if(s.modelProfiles.size()>=64)return false;
  s.modelProfiles.push_back(ModelProfile{key,checksum});
  found=&s.modelProfiles.back();
 }
 found->forward+=s.forward;found->right+=s.right;found->up+=s.up;found->yaw+=s.yawTrim;
 s.forward=s.right=s.up=s.yawTrim=0.f;
 normalize(s);return true;
}
inline bool eraseForModel(Settings& s,const std::string& key,std::uint32_t checksum){
 auto old=s.modelProfiles.size();
 s.modelProfiles.erase(std::remove_if(s.modelProfiles.begin(),s.modelProfiles.end(),[&](const ModelProfile& p){
  return p.model==key&&p.checksum==checksum;
 }),s.modelProfiles.end());
 return s.modelProfiles.size()!=old;
}
template<class Json> Settings readSettings(const Json& root){
 Settings s{};auto it=root.find("firstPersonBody");
 if(it==root.end()||!it->is_object())return s;
 auto readBool=[&](const char* key,bool& value){auto v=it->find(key);if(v!=it->end()&&v->is_boolean())value=v->template get<bool>();};
 auto readFloat=[&](const char* key,float& value){auto v=it->find(key);if(v!=it->end()&&v->is_number())value=v->template get<float>();};
 readBool("enabled",s.enabled);readBool("clipEnabled",s.clipEnabled);
 readBool("hideHead",s.hideHead);readBool("hideArms",s.hideArms);
 readBool("dynamicAnchor",s.dynamicAnchor);readBool("followCameraYaw",s.followCameraYaw);
 readFloat("clipHeight",s.clipHeight);readFloat("forward",s.forward);
 readFloat("right",s.right);readFloat("up",s.up);readFloat("step",s.step);
 readFloat("fallbackEyeHeight",s.fallbackEyeHeight);readFloat("yawTrim",s.yawTrim);
 auto profiles=it->find("modelProfiles");
 if(profiles!=it->end()&&profiles->is_array())for(const auto& entry:*profiles){
  if(!entry.is_object()||s.modelProfiles.size()>=64)break;
  auto m=entry.find("model"),c=entry.find("checksum");
  if(m==entry.end()||!m->is_string()||c==entry.end()||!c->is_number_unsigned())continue;
  ModelProfile p;p.model=m->template get<std::string>();
  if(!validModelKey(p.model))continue;
  auto checksum=c->template get<std::uint64_t>();if(checksum>UINT32_MAX)continue;
  p.checksum=static_cast<std::uint32_t>(checksum);
  auto read=[&](const char* k,float& v){auto value=entry.find(k);if(value!=entry.end()&&value->is_number())v=value->template get<float>();};
  read("forward",p.forward);read("right",p.right);read("up",p.up);read("yaw",p.yaw);
  s.modelProfiles.push_back(std::move(p));
 }
 normalize(s);return s;
}
template<class Json> void writeSettings(Json& root,const Settings& settings){
 Settings s=settings;normalize(s);
 Json array=Json::array();
 for(const auto& p:s.modelProfiles)
  array.push_back(Json{{"model",p.model},{"checksum",p.checksum},{"forward",p.forward},
   {"right",p.right},{"up",p.up},{"yaw",p.yaw}});
 root["firstPersonBody"]=Json{{"enabled",s.enabled},{"clipEnabled",s.clipEnabled},
  {"hideHead",s.hideHead},{"hideArms",s.hideArms},
  {"dynamicAnchor",s.dynamicAnchor},{"followCameraYaw",s.followCameraYaw},
  {"clipHeight",s.clipHeight},{"forward",s.forward},{"right",s.right},
  {"up",s.up},{"step",s.step},{"fallbackEyeHeight",s.fallbackEyeHeight},
  {"yawTrim",s.yawTrim},{"modelProfiles",std::move(array)}};
}
inline bool adjust(Settings& s,std::uintptr_t id){
 switch(id){
 case 870:s.clipEnabled=!s.clipEnabled;break;
 case 871:s.clipHeight-=1;break;case 872:s.clipHeight+=1;break;
 case 873:s.clipHeight-=5;break;case 874:s.clipHeight+=5;break;
 case 875:s.clipHeight=65;break;
 case 876:s.forward+=s.step;break;case 877:s.forward-=s.step;break;
 case 878:s.right+=s.step;break;case 879:s.right-=s.step;break;
 case 880:s.up+=s.step;break;case 881:s.up-=s.step;break;
 case 882:s.step=s.step==.5f?1.f:s.step==1.f?5.f:.5f;break;
 case 883:s.forward=s.right=s.up=0;break;
 case 884:s.hideHead=!s.hideHead;break;case 885:s.hideArms=!s.hideArms;break;
 case 887:s.dynamicAnchor=!s.dynamicAnchor;break;
 case 888:s.followCameraYaw=!s.followCameraYaw;break;
 case 889:s.fallbackEyeHeight+=1;break;case 890:s.fallbackEyeHeight-=1;break;
 case 891:s.yawTrim+=5;break;case 892:s.yawTrim-=5;break;
 case 893:s.yawTrim=0;break;
 default:return false;
 }
 normalize(s);return true;
}
}
