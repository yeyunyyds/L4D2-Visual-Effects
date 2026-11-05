#pragma once
#include "dango/source_trace_contract.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>
namespace visual::pictures {
inline constexpr unsigned MaxEdge=2048,MaxPictures=8,MaxScenePictures=32;
inline constexpr std::size_t CacheBudget=64u*1024u*1024u;
inline bool imageKey(const std::string& key){
 if(key.empty()||key.size()>240||key=="."||key=="..")return false;
 for(unsigned char c:key)if(c<32||c=='/'||c=='\\'||c==':'||c=='"')return false;
 auto dot=key.find_last_of('.');if(dot==std::string::npos)return false;
 std::string ext=key.substr(dot);for(char& c:ext)if(c>='A'&&c<='Z')c=char(c+32);
 return ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp";
}
struct Settings {bool enabled=false;std::string selected;unsigned size=60,rotation=0,opacity=10;bool bindT=false,autoNext=false;unsigned maximum=2;std::map<std::string,unsigned> sizes;};
inline Settings normalized(Settings s){
 s.size=std::clamp(s.size,10u,512u);s.maximum=std::clamp(s.maximum,1u,MaxPictures);
 for(auto i=s.sizes.begin();i!=s.sizes.end();)if(!imageKey(i->first))i=s.sizes.erase(i);else{i->second=std::clamp(i->second,10u,512u);++i;}
 s.rotation=(s.rotation%360)/15*15;s.opacity=std::clamp(s.opacity,1u,10u);
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
 auto bind=p->find("bindT");if(bind!=p->end()&&bind->is_boolean())s.bindT=bind->template get<bool>();
 auto next=p->find("autoNext");if(next!=p->end()&&next->is_boolean())s.autoNext=next->template get<bool>();
 auto maximum=p->find("maximum");if(maximum!=p->end()&&maximum->is_number_unsigned())s.maximum=maximum->template get<unsigned>();
 auto sizes=p->find("sizes");if(sizes!=p->end()&&sizes->is_object())for(auto i=sizes->begin();i!=sizes->end()&&s.sizes.size()<128;++i)if(imageKey(i.key())&&i.value().is_number_unsigned())s.sizes[i.key()]=i.value().template get<unsigned>();
 if(!p->contains("sizeProfileVersion")){auto old=p->find("size");if(old!=p->end()&&old->is_number_integer()&&*old>0)s.size=std::max(10u,s.size/2);}
 return normalized(s);
}
template<class Json> void writeSettings(Json& j,const Settings& s){auto p=normalized(s);j["wallPictures"]={{"enabled",p.enabled},{"selected",p.selected},{"size",p.size},{"rotation",p.rotation},{"opacity",p.opacity},{"bindT",p.bindT},{"autoNext",p.autoNext},{"maximum",p.maximum},{"sizes",p.sizes},{"sizeProfileVersion",1}};}
inline unsigned effectiveSize(const Settings& settings,const std::string& key);
struct Pixels {unsigned width=0,height=0;std::uint64_t revision=0;std::vector<unsigned char> bgra;};
struct Placement {std::uint64_t id=0;std::string key;dango::wall::Panel panel;unsigned opacity=10;bool visible=false;std::uint64_t visibleTick=0;std::uint64_t owner=0;};
struct Scene {
 std::uint32_t epoch=0;std::uint64_t sequence=0;std::vector<Placement> items;
 void observe(std::uint32_t next){if(epoch!=next){items.clear();epoch=next;}}
 void clear(){items.clear();}
 void undo(){for(auto i=items.end();i!=items.begin();){--i;if(!i->owner){items.erase(i);break;}}}
 void limit(unsigned maximum){
  std::map<std::uint64_t,unsigned> counts;for(auto i=items.rbegin();i!=items.rend();)if(++counts[i->owner]>maximum)i=std::vector<Placement>::reverse_iterator(items.erase(std::next(i).base()));else ++i;
  while(items.size()>MaxScenePictures){auto remote=std::find_if(items.begin(),items.end(),[](const auto& p){return p.owner!=0;});items.erase(remote==items.end()?items.begin():remote);}
 }
 template<class Trace> unsigned resize(const Settings& settings,Trace cast){unsigned changed=0;
  for(auto& item:items){if(item.owner)continue;float longest=std::max(item.panel.width,item.panel.height),target=float(effectiveSize(settings,item.key));
   if(longest<=0||std::abs(longest-target)<.001f)continue;
   auto candidate=item.panel;candidate.width*=target/longest;candidate.height*=target/longest;
   if(dango::trace::supported(candidate,cast)){item.panel=candidate;item.visible=false;item.visibleTick=0;++changed;}
  }return changed;
 }
 template<class Trace> bool place(const Settings& raw,unsigned width,unsigned height,dango::Vec3 hit,dango::Vec3 normal,Trace cast){
  auto settings=normalized(raw);if(!settings.enabled||settings.selected.empty()||!width||!height||width>MaxEdge||height>MaxEdge||!epoch)return false;
  float longest=float(std::max(width,height));dango::wall::Panel panel;
  if(!dango::wall::makePanel(hit,normal,float(effectiveSize(settings,settings.selected))*float(width)/longest,float(effectiveSize(settings,settings.selected))*float(height)/longest,epoch,panel))return false;
  constexpr float pi=3.14159265358979323846f;float angle=settings.rotation*pi/180.f,c=std::cos(angle),s=std::sin(angle);
  auto right=panel.right,up=panel.up;panel.right=right*c+up*s;panel.up=up*c-right*s;
  if(!dango::trace::supported(panel,cast))return false;
  items.push_back({++sequence,settings.selected,panel,settings.opacity,false,0});limit(settings.maximum);return true;
 }
};
inline bool visibleLease(const Placement& p,const dango::GameCameraFrame& camera,std::uint64_t tick){return camera.active&&camera.epoch==p.panel.epoch&&p.visible&&tick>=p.visibleTick&&tick-p.visibleTick<=350;}
inline unsigned effectiveSize(const Settings& settings,const std::string& key){auto i=settings.sizes.find(key);return i==settings.sizes.end()?settings.size:i->second;}
inline unsigned textureEdge(unsigned edge){unsigned n=1;while(n<edge&&n<MaxEdge)n*=2;return n;}
} // namespace visual::pictures
