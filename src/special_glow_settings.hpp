#pragma once
#include "special_infected.hpp"
#include "dango/entity_inventory.hpp"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
namespace visual::specialglow {
// m_glowColorOverride uses Windows COLORREF (low byte = red).
inline constexpr std::uint32_t rgb(unsigned r,unsigned g,unsigned b){return (b<<16)|(g<<8)|r;}
inline constexpr std::array<std::uint32_t,SpecialCount> defaults={
 rgb(115,210,255),rgb(255,189,78),rgb(255,93,115),rgb(157,236,90),
 rgb(202,139,255),rgb(255,124,69),rgb(255,72,72),rgb(232,232,255)};
inline constexpr std::array<std::uint32_t,10> palette={
 rgb(255,75,75),rgb(255,156,60),rgb(255,228,90),rgb(128,231,88),
 rgb(74,223,183),rgb(80,185,255),rgb(118,135,255),rgb(201,112,255),
 rgb(255,119,203),rgb(255,255,255)};
struct Rule {bool enabled=false,occlude=false;std::uint32_t color=0;};
struct Settings {
 std::array<Rule,SpecialCount> types{};
 Settings(){for(unsigned i=0;i<SpecialCount;++i)types[i].color=defaults[i];}
 bool any()const{for(const auto& r:types)if(r.enabled)return true;return false;}
 bool usesVisibility()const{for(const auto& r:types)if(r.enabled&&r.occlude)return true;return false;}
};
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
template<class Json> Settings readSettings(const Json& j){
 Settings result;auto root=j.find("specialGlow");if(root==j.end()||!root->is_object())return result;
 for(unsigned i=0;i<SpecialCount;++i){
  auto entry=root->find(SpecialNames[i]);if(entry==root->end()||!entry->is_object())continue;
  auto& s=result.types[i];
  auto toggle=entry->find("enabled");if(toggle!=entry->end()&&toggle->is_boolean())s.enabled=toggle->template get<bool>();
  auto occlude=entry->find("occlude");if(occlude!=entry->end()&&occlude->is_boolean())s.occlude=occlude->template get<bool>();
  auto color=entry->find("color");
  if(color!=entry->end()&&color->is_array()&&color->size()==3){
   unsigned c[3]{};bool okay=true;
   for(unsigned k=0;k<3;++k){const auto& value=(*color)[k];if(!value.is_number_integer()){okay=false;break;}auto n=value.template get<std::int64_t>();if(n<0||n>255){okay=false;break;}c[k]=unsigned(n);}
   if(okay)s.color=rgb(c[0],c[1],c[2]);
  }
 }
 return result;
}
template<class Json> void writeSettings(Json& j,const Settings& s){
 Json root=Json::object();
 for(unsigned i=0;i<SpecialCount;++i){const auto& v=s.types[i];root[SpecialNames[i]]={{"enabled",v.enabled},{"occlude",v.occlude},{"color",Json::array({v.color&255,(v.color>>8)&255,(v.color>>16)&255})}};}
 j["specialGlow"]=std::move(root);
}
inline unsigned channel(std::uint32_t color,unsigned index){return (color>>(index*8))&255u;}
inline std::uint32_t setChannel(std::uint32_t color,unsigned index,unsigned value){auto shift=index*8;return (color&~(255u<<shift))|((value&255u)<<shift);}
inline std::uint32_t stepChannel(std::uint32_t color,unsigned index,int step){return setChannel(color,index,unsigned(std::clamp(int(channel(color,index))+step,0,255)));}
inline std::uint32_t nextColor(std::uint32_t current){
 for(unsigned i=0;i<palette.size();++i)if(palette[i]==current)return palette[(i+1)%palette.size()];
 return palette[0];
}
inline std::string colorText(std::uint32_t color){return "RGB "+std::to_string(channel(color,0))+"/"+std::to_string(channel(color,1))+"/"+std::to_string(channel(color,2));}
template<class Record> bool eligible(const Record& r){
 if(specialCharacter(r)<0||r.index>=4096||r.handle==0xffffffffu||
    (r.handle&4095u)!=r.index||(r.handle>>12)!=r.serial||
    !r.networkOrigin||!std::isfinite(r.networkOrigin->x)||!std::isfinite(r.networkOrigin->y)||!std::isfinite(r.networkOrigin->z))return false;
 if((r.health&&*r.health<=0)||(r.lifeState&&*r.lifeState!=0)||(r.dormant&&*r.dormant))return false;
 return true;
}
} // namespace visual::specialglow
