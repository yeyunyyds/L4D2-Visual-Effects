#pragma once
#include "dango/source_trace_contract.hpp"
#include <string>
#include <vector>
#include <memory>
namespace visual::pictures {
inline constexpr unsigned MaxEdge=2048,MaxPictures=8;
inline constexpr std::size_t CacheBudget=64u*1024u*1024u;
inline bool imageKey(const std::string& key){
 if(key.empty()||key.size()>240||key=="."||key=="..")return false;
 for(unsigned char c:key)if(c<32||c=='/'||c=='\\'||c==':'||c=='"')return false;
 auto dot=key.find_last_of('.');if(dot==std::string::npos)return false;
 std::string ext=key.substr(dot);for(char& c:ext)if(c>='A'&&c<='Z')c=char(c+32);
 return ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp";
}
struct Settings {bool enabled=false;std::string selected;unsigned size=120,rotation=0,opacity=10;};
inline Settings normalized(Settings s){
 s.size=std::clamp(s.size,20u,512u);s.rotation=(s.rotation%360)/15*15;s.opacity=std::clamp(s.opacity,1u,10u);
 if(!imageKey(s.selected))s.selected.clear();
 return s;
}
template<class Json> Settings readSettings(const Json& j){
 Settings s;auto p=j.find("wallPictures");if(p==j.end()||!p->is_object())return s;
 auto enabled=p->find("enabled");if(enabled!=p->end()&&enabled->is_boolean())s.enabled=enabled->template get<bool>();
 auto selected=p->find("selected");if(selected!=p->end()&&selected->is_string())s.selected=selected->template get<std::string>();
 for(auto pair:{std::pair<const char*,unsigned*>{"size",&s.size},{"rotation",&s.rotation},{"opacity",&s.opacity}}){
  auto value=p->find(pair.first);if(value!=p->end()&&value->is_number_integer()&&*value>=0&&*value<=10000)*pair.second=value->template get<unsigned>();
 }
 return normalized(s);
}
template<class Json> void writeSettings(Json& j,const Settings& s){auto p=normalized(s);j["wallPictures"]={{"enabled",p.enabled},{"selected",p.selected},{"size",p.size},{"rotation",p.rotation},{"opacity",p.opacity}};}
struct Pixels {unsigned width=0,height=0;std::uint64_t revision=0;std::vector<unsigned char> bgra;};
struct Placement {std::uint64_t id=0;std::string key;dango::wall::Panel panel;unsigned opacity=10;bool visible=false;std::uint64_t visibleTick=0;};
struct Scene {
 std::uint32_t epoch=0;std::uint64_t sequence=0;std::vector<Placement> items;
 void observe(std::uint32_t next){if(epoch!=next){items.clear();epoch=next;}}
 void clear(){items.clear();}
 void undo(){if(!items.empty())items.pop_back();}
 template<class Trace> bool place(const Settings& raw,unsigned width,unsigned height,dango::Vec3 hit,dango::Vec3 normal,Trace cast){
  auto settings=normalized(raw);if(!settings.enabled||settings.selected.empty()||!width||!height||width>MaxEdge||height>MaxEdge||!epoch||items.size()>=MaxPictures)return false;
  float longest=float(std::max(width,height));dango::wall::Panel panel;
  if(!dango::wall::makePanel(hit,normal,settings.size*float(width)/longest,settings.size*float(height)/longest,epoch,panel))return false;
  constexpr float pi=3.14159265358979323846f;float angle=settings.rotation*pi/180.f,c=std::cos(angle),s=std::sin(angle);
  auto right=panel.right,up=panel.up;panel.right=right*c+up*s;panel.up=up*c-right*s;
  if(!dango::trace::supported(panel,cast))return false;
  items.push_back({++sequence,settings.selected,panel,settings.opacity,false,0});return true;
 }
};
inline bool visibleLease(const Placement& p,const dango::GameCameraFrame& camera,std::uint64_t tick){return camera.active&&camera.epoch==p.panel.epoch&&p.visible&&tick>=p.visibleTick&&tick-p.visibleTick<=350;}
inline unsigned textureEdge(unsigned edge){unsigned n=1;while(n<edge&&n<MaxEdge)n*=2;return n;}
} // namespace visual::pictures
