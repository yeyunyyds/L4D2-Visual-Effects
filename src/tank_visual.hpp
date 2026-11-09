#pragma once
#include "dango/world_camera.hpp"
#include <optional>
#include <string>
#include <vector>
namespace visual::tank {
struct VisualSettings {unsigned preset=2,step=4;bool vertical=true,showDirection=true,showRange=true,showBar=true,showTarget=true;float x=-32,y=-24,length=88,thickness=8,barSize=.55f,arrowWidth=3,rangeWidth=3,arrowSize=1;};
inline const char* visualName(unsigned p){return p==1?u8"琥珀战术":p==2?u8"冰蓝刻度":u8"青金简约";}
inline void normalizeVisual(VisualSettings& s){s.preset=std::min(s.preset,2u);if(s.step!=2&&s.step!=4&&s.step!=8&&s.step!=16)s.step=4;
 auto bound=[](float v,float fallback,float lo,float hi){return std::isfinite(v)?std::clamp(v,lo,hi):fallback;};s.x=bound(s.x,-32,-200,200);s.y=bound(s.y,-24,-200,200);s.length=bound(s.length,88,40,200);s.thickness=bound(s.thickness,8,1,24);s.barSize=bound(s.barSize,.55f,.2f,2);s.arrowWidth=bound(s.arrowWidth,3,.5f,6);s.rangeWidth=bound(s.rangeWidth,3,.5f,6);s.arrowSize=bound(s.arrowSize,1,.25f,3);}
template<class Json> VisualSettings readVisual(const Json& j){VisualSettings s;auto p=j.find("tankVisual");if(p==j.end()||!p->is_object())return s;
 auto integer=[&](const char* name,unsigned fallback,unsigned max){auto i=p->find(name);if(i==p->end())return fallback;if(i->is_number_unsigned())return unsigned(std::min(i->template get<std::uint64_t>(),std::uint64_t(max)));if(i->is_number_integer())return unsigned(std::clamp(i->template get<std::int64_t>(),std::int64_t(0),std::int64_t(max)));return fallback;};
 s.preset=integer("preset",2,2);s.step=integer("step",4,64);
 for(auto value:{std::pair<const char*,bool*>{"vertical",&s.vertical},{"showDirection",&s.showDirection},{"showRange",&s.showRange},{"showBar",&s.showBar},{"showTarget",&s.showTarget}})if(p->contains(value.first)&&(*p)[value.first].is_boolean())*value.second=(*p)[value.first].template get<bool>();
 for(auto pair:{std::pair<const char*,float*>{"x",&s.x},{"y",&s.y},{"length",&s.length},{"thickness",&s.thickness},{"barSize",&s.barSize},{"arrowWidth",&s.arrowWidth},{"rangeWidth",&s.rangeWidth},{"arrowSize",&s.arrowSize}})if(p->contains(pair.first)&&(*p)[pair.first].is_number())*pair.second=(*p)[pair.first].template get<float>();
 normalizeVisual(s);return s;}
template<class Json> void writeVisual(Json& j,VisualSettings s){normalizeVisual(s);j["tankVisual"]={{"preset",s.preset},{"step",s.step},{"vertical",s.vertical},{"x",s.x},{"y",s.y},{"length",s.length},{"thickness",s.thickness},{"barSize",s.barSize},{"arrowWidth",s.arrowWidth},{"rangeWidth",s.rangeWidth},{"arrowSize",s.arrowSize},{"showDirection",s.showDirection},{"showRange",s.showRange},{"showBar",s.showBar},{"showTarget",s.showTarget},{"coordinateSpace","tank-world"}};}
enum class FineField{BarSize,Length,Thickness,ArrowWidth,ArrowSize,RangeWidth};
inline bool finePage(std::uintptr_t page){return page==325||page==326||page==327;}
inline void adjustFine(VisualSettings& s,FineField field,int direction){if(!direction)return;float* value=nullptr;float step=.1f;
 switch(field){case FineField::BarSize:value=&s.barSize;step=.01f;break;case FineField::Length:value=&s.length;step=1;break;case FineField::Thickness:value=&s.thickness;break;case FineField::ArrowWidth:value=&s.arrowWidth;break;case FineField::ArrowSize:value=&s.arrowSize;step=.01f;break;case FineField::RangeWidth:value=&s.rangeWidth;break;}
 *value=std::round((*value+(direction>0?step:-step))/step)*step;normalizeVisual(s);
}
struct Palette{std::uint32_t direction,range,future,progress,frame,background;bool dashed=false,ticks=false;float width=1;};
inline Palette palette(unsigned p){if(p==1)return {0xffffba52,0xffe59a36,0xff75664c,0xffffd98c,0xffd0b48a,0xff171c23,true,true,2};if(p==2)return {0xff8fdcff,0xff71b7ff,0xff465e80,0xff68e0ad,0xff84a9c5,0xff111e2b,false,true,1};return {0xff4ee7df,0xff379b9c,0xff355762,0xffffc76a,0xff6f8b95,0xff121c25,false,false,1};}
struct Point{float x=0,y=0;};struct Segment{Point a,b;std::uint32_t color=0;float width=1;};
inline void stroke(std::vector<Segment>& out,Point a,Point b,std::uint32_t c,float width=1){out.push_back({a,b,c,width});}
inline void rectangle(std::vector<Segment>& out,float x,float y,float w,float h,std::uint32_t c){stroke(out,{x,y},{x+w,y},c);stroke(out,{x+w,y},{x+w,y+h},c);stroke(out,{x+w,y+h},{x,y+h},c);stroke(out,{x,y+h},{x,y},c);}
inline void filled(std::vector<Segment>& out,float x,float y,float w,float h,std::uint32_t c){if(w<=0||h<=0)return;for(float row=0;row<h;row+=1)stroke(out,{x,y+row},{x+w,y+row},c);}
inline Point barCenter(Point tank,VisualSettings s,float scale){normalizeVisual(s);return {tank.x+s.x*scale,tank.y+s.y*scale};}
// Design units are attached to the entity, never added as screen pixels.
struct RenderAnchor{std::uint32_t epoch=0,handle=0;std::uint64_t tick=0;dango::Vec3 position{};};
inline bool validRenderAnchor(const RenderAnchor& a,std::uint32_t epoch,std::uint32_t handle,std::uint64_t now,dango::Vec3 network){return a.epoch==epoch&&a.handle==handle&&now>=a.tick&&now-a.tick<=100&&std::isfinite(a.position.x)&&std::isfinite(a.position.y)&&std::isfinite(a.position.z)&&dango::length(a.position-network)<=256;}
inline constexpr float BarWorldUnit=.75f;
struct BarPlacement{float x=0,y=0,scale=0;dango::Vec3 anchor{};};
inline dango::Vec3 attachedBarAnchor(dango::Vec3 body,float heading,VisualSettings s){normalizeVisual(s);float yaw=heading*3.14159265358979323846f/180;dango::Vec3 right{std::sin(yaw),-std::cos(yaw),0};return body+right*(s.x*BarWorldUnit)+dango::Vec3{0,0,-s.y*BarWorldUnit};}
inline std::optional<BarPlacement> placeBar(const dango::GameCameraFrame& camera,dango::Vec3 body,float width,float height,VisualSettings settings,float heading=0){
 normalizeVisual(settings);if(width<100||height<100||!std::isfinite(heading))return {};
 auto anchor=attachedBarAnchor(body,heading,settings);dango::Vec3 ndc,side;
 if(!dango::transform(camera.worldToClip,anchor,ndc,true)||ndc.z<0||ndc.z>1)return {};
 auto right=dango::normalize({camera.worldToView[0],camera.worldToView[1],camera.worldToView[2]});if(dango::length(right)<.9f)return {};
 if(!dango::transform(camera.worldToClip,anchor+right*BarWorldUnit,side,true))return {};
 float dx=(side.x-ndc.x)*width*.5f,dy=(side.y-ndc.y)*height*.5f;float scale=std::sqrt(dx*dx+dy*dy);
 // Bound near-plane geometry work; never impose a minimum apparent size at range.
 if(!std::isfinite(scale)||scale<=0||scale*settings.barSize*(std::max(settings.length,settings.thickness)+4)>4096)return {};
 float x=(ndc.x+1)*width*.5f,y=(1-ndc.y)*height*.5f;
 float halfW=((settings.vertical?settings.thickness:settings.length)*.5f+2)*scale*settings.barSize,halfH=((settings.vertical?settings.length:settings.thickness)*.5f+2)*scale*settings.barSize;
 // Normal clipping replaces edge snapping, which detached the bar from its entity.
 if(x+halfW<0||x-halfW>width||y+halfH<0||y-halfH>height)return {};
 return BarPlacement{x,y,scale,anchor};
}
// Both the in-game bar and the menu preview use this exact geometry.
inline void barGeometry(std::vector<Segment>& out,Point center,VisualSettings s,float scale,std::optional<float> fraction,bool danger=false){normalizeVisual(s);scale*=s.barSize;auto p=palette(s.preset);if(danger){p.progress=0xffff6868;p.frame=0xffff3030;p.background=0xff601515;}float w=(s.vertical?s.thickness:s.length)*scale,h=(s.vertical?s.length:s.thickness)*scale;float x=center.x-w*.5f,y=center.y-h*.5f;
 float padding=2*scale;filled(out,x-padding,y-padding,w+padding*2,h+padding*2,p.background);rectangle(out,x-padding,y-padding,w+padding*2,h+padding*2,p.frame);
 if(fraction){float f=std::clamp(*fraction,0.f,1.f);if(s.vertical)filled(out,x,y+h*(1-f),w,h*f,p.progress);else filled(out,x,y,w*f,h,p.progress);}
 if(p.ticks)for(unsigned i=1;i<4;++i){float f=i*.25f;if(s.vertical)stroke(out,{x+w-3*scale,y+h*f},{x+w,y+h*f},p.background);else stroke(out,{x+w*f,y+h-3*scale},{x+w*f,y+h},p.background);}
}
inline void demoRange(std::vector<Segment>& out,Point c,float scale,VisualSettings settings){normalizeVisual(settings);auto p=palette(settings.preset);float aw=p.width*settings.arrowWidth,rw=p.width*settings.rangeWidth;constexpr float pi=3.14159265358979323846f;float r=102*scale;
 for(unsigned i=0;i<40;++i){float a=(-160.f+i*3.5f)*pi/180,b=(-160.f+(i+1)*3.5f)*pi/180;if(!p.dashed||i%2==0)stroke(out,{c.x+std::cos(a)*r,c.y+std::sin(a)*r},{c.x+std::cos(b)*r,c.y+std::sin(b)*r},p.range,rw);
 if(p.ticks&&i%10==0)stroke(out,{c.x+std::cos(a)*r*.94f,c.y+std::sin(a)*r*.94f},{c.x+std::cos(a)*r*1.04f,c.y+std::sin(a)*r*1.04f},p.range,settings.rangeWidth);
 }float arrowScale=scale*settings.arrowSize;stroke(out,c,{c.x,c.y-82*arrowScale},p.direction,aw);stroke(out,{c.x,c.y-82*arrowScale},{c.x-7*arrowScale,c.y-70*arrowScale},p.direction,aw);stroke(out,{c.x,c.y-82*arrowScale},{c.x+7*arrowScale,c.y-70*arrowScale},p.direction,aw);
}
inline void demoRange(std::vector<Segment>& out,Point c,float scale,unsigned preset){VisualSettings settings;settings.preset=preset;demoRange(out,c,scale,settings);}
struct OverlayGate {
 bool paused=false,escapeHeld=false,wasEnabled=false;std::uint32_t epoch=0;std::uint64_t tick=0;dango::Matrix4 view{};
 void poll(const dango::GameCameraFrame& camera,bool escape,bool pluginMenu,bool foreground,bool enabled,bool reset=false){
  if(!enabled||!camera.active||camera.epoch!=epoch||!wasEnabled||reset)paused=false;
  if(enabled&&camera.active&&foreground&&escape&&!escapeHeld&&!pluginMenu)paused=true;
  // No OS-cursor guesses: DXVK relative input can report the wrong cursor state.
  // A new in-game view rotation after Escape is our bounded recovery signal.
  if(paused&&foreground&&!escape&&!pluginMenu&&camera.active&&camera.epoch==epoch&&camera.tick>tick){float change=0;for(unsigned i:{0u,1u,2u,4u,5u,6u,8u,9u,10u})change=std::max(change,std::abs(camera.worldToView[i]-view[i]));if(change>.0001f)paused=false;}
  escapeHeld=escape;wasEnabled=enabled&&camera.active;epoch=camera.epoch;tick=camera.tick;view=camera.worldToView;
 }
};
}
