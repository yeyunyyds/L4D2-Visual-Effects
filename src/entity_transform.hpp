#pragma once
#include "entity_selection.hpp"
#include "stretch.hpp"
#include <algorithm>
#include <vector>
namespace visual::deform {
struct Rule {bool enabled=false;float size=1,x=1,y=1,z=1;};
inline void normalize(Rule& r){for(auto p:{&r.size,&r.x,&r.y,&r.z})*p=std::isfinite(*p)?std::clamp(*p,.1f,5.f):1.f;}
inline std::array<float,3> factors(Rule r){normalize(r);return {std::clamp(r.size*r.x,.1f,5.f),std::clamp(r.size*r.y,.1f,5.f),std::clamp(r.size*r.z,.1f,5.f)};}
inline bool identity(const Rule& r){auto s=factors(r);return std::abs(s[0]-1)<.0001f&&std::abs(s[1]-1)<.0001f&&std::abs(s[2]-1)<.0001f;}
inline bool uniform(const Rule& r){auto s=factors(r);return std::abs(s[0]-s[1])<.0001f&&std::abs(s[1]-s[2])<.0001f;}
struct Settings{bool enabled=false,studioOnly=false;Rule all;std::map<std::string,Rule> rules;};
template<class Record> std::vector<std::string> keys(const Record& r){std::vector<std::string> k;auto special=specialCharacter(r);if(special>=0)k.push_back("s:"+std::to_string(special));if(survivorRecord(r))k.push_back("v:"+std::to_string(survivorCharacter(r)));k.push_back("c:"+std::to_string(r.classId)+":"+r.className);k.push_back("t:"+r.className);k.push_back("g:"+std::to_string(unsigned(special>=0?EntityGroup::Special:survivorRecord(r)?EntityGroup::Survivors:groupForName(r.className))));return k;}
template<class Record> Rule selected(const Settings& s,const Record& r){for(const auto& k:keys(r)){auto i=s.rules.find(k);if(i!=s.rules.end())return i->second;}return s.all;}
inline bool any(const Settings& s){if(!s.enabled)return false;if(s.all.enabled&&!identity(s.all))return true;for(const auto& i:s.rules)if(i.second.enabled&&!identity(i.second))return true;return false;}
inline bool transform(const Matrix3x4* src,Matrix3x4* dst,std::size_t count,const float* pivot,const Rule& rule,const unsigned* flags=nullptr,int mask=-1){
 if(!src||!dst||!count||count>256||!pivot)return false;
 auto from=reinterpret_cast<uintptr_t>(src),to=reinterpret_cast<uintptr_t>(dst);auto bytes=count*sizeof(Matrix3x4);if(src!=dst&&from<to+bytes&&to<from+bytes)return false;
 auto s=factors(rule);for(unsigned r=0;r<3;++r)if(!std::isfinite(pivot[r]))return false;
 for(std::size_t i=0;i<count;++i)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){if(flags&&!(flags[i]&unsigned(mask)))continue;float v=src[i].m[r][c];if(!std::isfinite(v))return false;if((!flags||(flags[i]&unsigned(mask)))&&!std::isfinite(c<3?v*s[r]:pivot[r]+(v-pivot[r])*s[r]))return false;}
 for(std::size_t i=0;i<count;++i){dst[i]=src[i];if(flags&&!(flags[i]&unsigned(mask)))continue;for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)dst[i].m[r][c]*=s[r];dst[i].m[r][3]=pivot[r]+(src[i].m[r][3]-pivot[r])*s[r];}}return true;
}
inline bool boneFlagVector(const std::array<std::uint32_t,5>& v,unsigned count){return count&&count<=256&&v[0]&&v[0]==v[4]&&v[3]==count&&v[1]>=count&&v[1]<=4096&&v[2]<=4096&&std::uint64_t(v[0])+count*4<=0x100000000ull;}
inline bool centroid(const Matrix3x4* bones,std::size_t count,float* pivot,const unsigned* flags=nullptr,int mask=-1){if(!bones||!count||count>256)return false;unsigned n=0;for(unsigned r=0;r<3;++r)pivot[r]=0;for(std::size_t i=0;i<count;++i)if(!flags||(flags[i]&unsigned(mask))){for(unsigned r=0;r<3;++r){if(!std::isfinite(bones[i].m[r][3]))return false;pivot[r]+=bones[i].m[r][3];}++n;}if(!n)return false;for(unsigned r=0;r<3;++r)pivot[r]/=n;return true;}
inline Rule inherited(const Settings& s,const std::string& key){
 auto own=s.rules.find(key);if(own!=s.rules.end())return own->second;std::string group;
 if(key.rfind("s:",0)==0)group="g:0";else if(key.rfind("v:",0)==0)group="g:1";
 else if(key.rfind("t:",0)==0)group="g:"+std::to_string(unsigned(groupForName(key.substr(2))));
 else if(key.rfind("c:",0)==0){auto colon=key.find(':',2);if(colon!=std::string::npos)group="g:"+std::to_string(unsigned(groupForName(key.substr(colon+1))));}
 auto parent=s.rules.find(group);return parent==s.rules.end()?s.all:parent->second;
}
template<class Json> Settings readSettings(const Json& j){Settings s;auto i=j.find("entityTransform");if(i==j.end()||!i->is_object())return s;auto parse=[](const auto& v){Rule r;if(!v.is_object())return r;if(v.contains("enabled")&&v["enabled"].is_boolean())r.enabled=v["enabled"].template get<bool>();for(auto p:{std::pair<const char*,float*>{"size",&r.size},{"x",&r.x},{"y",&r.y},{"z",&r.z}})if(v.contains(p.first)&&v[p.first].is_number())*p.second=v[p.first].template get<float>();normalize(r);return r;};if(i->contains("enabled")&&(*i)["enabled"].is_boolean())s.enabled=(*i)["enabled"].template get<bool>();if(i->contains("studioOnly")&&(*i)["studioOnly"].is_boolean())s.studioOnly=(*i)["studioOnly"].template get<bool>();if(i->contains("all"))s.all=parse((*i)["all"]);if(i->contains("rules")&&(*i)["rules"].is_object())for(auto r=(*i)["rules"].begin();r!=(*i)["rules"].end()&&s.rules.size()<8192;++r)if(r.key().size()<256)s.rules[r.key()]=parse(r.value());return s;}
template<class Json> void writeSettings(Json& j,const Settings& s){auto emit=[](Rule r){normalize(r);return Json{{"enabled",r.enabled},{"size",r.size},{"x",r.x},{"y",r.y},{"z",r.z}};};auto out=Json{{"enabled",s.enabled},{"studioOnly",s.studioOnly},{"all",emit(s.all)},{"rules",Json::object()}};for(const auto& r:s.rules)out["rules"][r.first]=emit(r.second);j["entityTransform"]=std::move(out);}
}
