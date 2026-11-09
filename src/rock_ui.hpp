#pragma once
#include "rock_runtime.hpp"
#include "tank_visual.hpp"
#include "imgui/imgui.h"
#include <sstream>
namespace visual::rockui {
inline visual::tank::OverlayGate displayGate;inline bool foreground=false;
inline void pollDisplay(const dango::GameCameraFrame& camera,bool escape,bool pluginMenu,bool focused){foreground=focused;displayGate.poll(camera,escape,pluginMenu,focused,rockruntime::active());}
inline void readSettings(const nlohmann::json& j){std::atomic_store(&rockruntime::options,std::make_shared<const rocks::Settings>(rocks::readSettings(j)));}
inline void writeSettings(nlohmann::json& j){rocks::writeSettings(j,*std::atomic_load(&rockruntime::options));}
inline std::string row(const std::string& label,uintptr_t id){std::ostringstream out;out<<'"'<<label<<"\" { \"user_data\" \"0x"<<std::hex<<id<<"\" }\n";return out.str();}
inline std::string toggle(const char* label,bool enabled){return std::string(label)+(enabled?u8"：开":u8"：关");}
inline std::string title(){auto cfg=std::atomic_load(&rockruntime::options);return std::string(u8"Tank 石头预测")+(rockruntime::faulted?u8"[不可用]":cfg->enabled?u8"[开]":u8"[关]");}
inline std::string menu(uintptr_t page){auto s=std::atomic_load(&rockruntime::options);
 if(page==701){constexpr const char* colors[]={u8"绿色",u8"橙色",u8"蓝色"};return "\"显示外观\" {"+row(u8"轨迹颜色："+std::string(colors[std::min(s->color,2u)]),755)+row(u8"轨迹不透明度："+std::to_string(int(s->opacity*100))+"%",756)+row(u8"轨迹线宽："+std::to_string(int(s->lineWidth))+" px",757)+row(u8"落点圆大小："+std::to_string(int(s->landingRadius)),758)+row(u8"落点圆不透明度："+std::to_string(int(s->landingOpacity*100))+"%",759)+row(u8"保存设置",101)+"}";}
 return "\"Tank 石头预测\" {"+row(toggle(u8"石头预测",s->enabled),750)+row(toggle(u8"飞行轨迹",s->trajectory),751)+row(toggle(u8"落点标记",s->landingCircle),752)+row(u8"轨迹长度："+std::to_string(s->horizonMs)+" ms",753)+row(u8"显示外观",701)+row(u8"恢复本功能默认设置",754)+row(u8"保存设置",101)+(rockruntime::faulted?row(u8"当前接口不可用，请关闭后重试",0):std::string{})+"}";
}
inline bool action(uintptr_t id){if(id<750||id>759)return false;auto s=*std::atomic_load(&rockruntime::options);
 switch(id){case 750:s.enabled=!s.enabled;break;case 751:s.trajectory=!s.trajectory;break;case 752:s.landingCircle=!s.landingCircle;break;case 753:s.horizonMs=rocks::nextHorizon(s.horizonMs);break;case 754:s=rocks::Settings{};break;case 755:s.color=(s.color+1)%3;break;case 756:s.opacity=s.opacity>=.99f?.1f:s.opacity+.1f;break;case 757:s.lineWidth=s.lineWidth>=5?1:s.lineWidth+1;break;case 758:s.landingRadius=s.landingRadius>=96?8:s.landingRadius+8;break;case 759:s.landingOpacity=s.landingOpacity>=.79f?.1f:s.landingOpacity+.1f;break;default:break;}
 std::atomic_store(&rockruntime::options,std::make_shared<const rocks::Settings>(rocks::sanitize(s)));return true;
}
inline bool needed(){
 if(!rockruntime::active()||!foreground||displayGate.paused)return false;auto f=std::atomic_load(&rockruntime::snapshot);auto c=std::atomic_load(&source_camera::snapshot);auto now=GetTickCount64();
 if(f->epoch!=c->epoch||!dango::usableCamera(*c,now)||now<f->tick||now-f->tick>rocks::SampleLifetimeMs)return false;
 auto settings=std::atomic_load(&rockruntime::options);for(const auto& item:f->tracks){const auto& t=item.second;if(!rocks::visibleFlight(t,now))continue;if(settings->trajectory)return true;auto hit=f->landings.find(item.first);if(settings->landingCircle&&hit!=f->landings.end()&&rocks::landingSecondsFor(hit->second,*t.motion,now))return true;}return false;
}

inline bool project(const dango::GameCameraFrame& camera,dango::Vec3 p,float width,float height,ImVec2& out){dango::Vec3 ndc;if(!dango::transform(camera.worldToClip,p,ndc,true)||ndc.z<0||ndc.z>1||std::abs(ndc.x)>1.2f||std::abs(ndc.y)>1.2f)return false;out={(ndc.x+1)*width*.5f,(1-ndc.y)*height*.5f};return true;}
inline void draw(float width,float height){try{
 if(!needed())return;auto frame=std::atomic_load(&rockruntime::snapshot);auto camera=std::atomic_load(&source_camera::snapshot);auto s=std::atomic_load(&rockruntime::options);auto now=GetTickCount64();
 const ImVec4 palette[]={{.2f,1,.4f,s->opacity},{1,.65f,.15f,s->opacity},{.2f,.75f,1,s->opacity}};auto color=ImGui::ColorConvertFloat4ToU32(palette[s->color]);auto list=ImGui::GetBackgroundDrawList();
 for(const auto& item:frame->tracks){const auto& t=item.second;if(!rocks::visibleFlight(t,now))continue;
  auto found=frame->landings.find(item.first);auto impactSeconds=found==frame->landings.end()?std::optional<float>{}:rocks::landingSecondsFor(found->second,*t.motion,now);bool impact=bool(impactSeconds);
  if(s->trajectory){ImVec2 previous;bool ready=false;for(const auto& p:t.trajectory){if(p.seconds>s->horizonMs/1000.f+.001f)break;if(impact&&p.seconds>*impactSeconds){ImVec2 end;if(ready&&project(*camera,found->second.centerPosition,width,height,end))list->AddLine(previous,end,color,s->lineWidth);break;}ImVec2 next;if(project(*camera,p.position,width,height,next)){if(ready)list->AddLine(previous,next,color,s->lineWidth);previous=next;ready=true;}else ready=false;}}
  // A circle always denotes a valid world intersection. No theoretical-endpoint fallback.
  if(s->landingCircle&&impact){const auto& hit=found->second;auto ring=rocks::landingDisk(hit.position,hit.normal,s->landingRadius);std::array<ImVec2,32> polygon{};bool visible=true;for(size_t i=0;i<ring.size();++i)if(!project(*camera,ring[i],width,height,polygon[i])){visible=false;break;}
   auto fill=ImGui::ColorConvertFloat4ToU32({1,.08f,.08f,s->landingOpacity});auto edge=ImGui::ColorConvertFloat4ToU32({1,.15f,.15f,.9f});
   if(visible){float area=0;for(size_t i=0;i<polygon.size();++i){auto a=polygon[i],b=polygon[(i+1)%polygon.size()];area+=a.x*b.y-b.x*a.y;}if(area<0)std::reverse(polygon.begin(),polygon.end());list->AddConvexPolyFilled(polygon.data(),int(polygon.size()),fill);list->AddPolyline(polygon.data(),int(polygon.size()),edge,ImDrawFlags_Closed,2);}
   else{ImVec2 center;if(project(*camera,hit.position,width,height,center)){list->AddCircleFilled(center,14,fill,32);list->AddCircle(center,14,edge,32,2);}}
  }
 }
}catch(...){rockruntime::faulted=true;}
}
}
