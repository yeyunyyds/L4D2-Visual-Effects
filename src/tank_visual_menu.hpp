#pragma once
#include "tank_visual_state.hpp"
#include "entity_menu.hpp"
#include "imgui/imgui.h"
#include <windows.h>
namespace visual::tank {
inline std::string visualMenu(){auto s=std::atomic_load(&visualSettings);return menuPage(u8"Tank 样式与位置预览",
 menuRow(u8"方向键移动 · X "+std::to_string(int(s->x))+u8" / Y "+std::to_string(int(s->y)))+
 menuItem(std::string(u8"预设：")+visualName(s->preset),360)+menuItem(s->vertical?u8"计时条：竖条（切换）":u8"计时条：横条（切换）",361)+menuItem(u8"移动步长："+std::to_string(s->step)+" px",362)+menuItem(u8"长度："+std::to_string(int(s->length))+u8" px（切换）",365)+menuItem(u8"厚度："+std::to_string(int(s->thickness))+u8" px（切换）",366)+menuItem(u8"重置位置与方向",363)+menuItem(u8"恢复 Tank 显示",364)+menuItem(u8"保存设置",101));}
inline bool visualAction(std::uintptr_t id){if(id<360||id>366)return false;auto s=*std::atomic_load(&visualSettings);
 if(id==360)s.preset=(s.preset+1)%3;if(id==361)s.vertical=!s.vertical;if(id==362)s.step=s.step==2?4:s.step==4?8:s.step==8?16:2;
 if(id==363){auto preset=s.preset;s=VisualSettings{};s.preset=preset;}
 if(id==364)display::resumeRequested=true;if(id==365)s.length=s.length<88?88:s.length<112?112:s.length<136?136:64;if(id==366)s.thickness=s.thickness<6?6:s.thickness<8?8:s.thickness<10?10:4;
 normalizeVisual(s);std::atomic_store(&visualSettings,std::make_shared<const VisualSettings>(s));return true;}
inline ImU32 previewColor(std::uint32_t c){return IM_COL32((c>>16)&255,(c>>8)&255,c&255,(c>>24)&255);}
inline bool visualPreviewKeys(std::uintptr_t id){return id==321;}
inline void visualPreview(std::uintptr_t page,float width,float height,bool focused){
 static std::array<bool,4> held{};static std::array<std::uint64_t,4> nextRepeat{};bool editing=page==321;
 if(!editing){held.fill(false);nextRepeat.fill(0);return;}
 auto settings=*std::atomic_load(&visualSettings);bool changed=false;const int codes[]={VK_LEFT,VK_UP,VK_RIGHT,VK_DOWN};auto now=GetTickCount64();
 for(unsigned i=0;i<4;++i){bool down=focused&&(GetAsyncKeyState(codes[i])&0x8000)!=0;if(down&&(!held[i]||now>=nextRepeat[i])){if(i==0)settings.x-=settings.step;if(i==1)settings.y-=settings.step;if(i==2)settings.x+=settings.step;if(i==3)settings.y+=settings.step;changed=true;nextRepeat[i]=now+(held[i]?90:300);}held[i]=down;}
 if(changed){normalizeVisual(settings);std::atomic_store(&visualSettings,std::make_shared<const VisualSettings>(settings));}
 float w=std::min(540.f,width*.47f),h=std::min(520.f,height-24);ImGui::SetNextWindowPos({width-w-12,(height-h)*.5f},ImGuiCond_Always);ImGui::SetNextWindowSize({w,h},ImGuiCond_Always);ImGui::SetNextWindowBgAlpha(.94f);
 ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.025f,.045f,.065f,1));ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,10.f);
 if(ImGui::Begin("Tank visual preview",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar)){
  auto pos=ImGui::GetWindowPos();auto list=ImGui::GetWindowDrawList();float scale=std::max(.1f,std::min((w-30)/540.f,(h-90)/460.f));Point center{pos.x+w*.5f,pos.y+h*.52f};auto colors=palette(settings.preset);
  ImGui::TextColored(ImVec4(.8f,.9f,1,1),"%s",visualName(settings.preset));ImGui::TextUnformatted(u8"模拟预览 · 无需地图中的 Tank");
  for(int i=-4;i<=4;++i){float d=i*50*scale;list->AddLine({center.x+d,center.y-220*scale},{center.x+d,center.y+220*scale},IM_COL32(35,50,66,110));list->AddLine({center.x-250*scale,center.y+d},{center.x+250*scale,center.y+d},IM_COL32(35,50,66,110));}
  std::vector<Segment> lines;demoRange(lines,center,scale,settings.preset);float fraction=1-float(now%2000)/2000.f;auto bar=barCenter(center,settings,scale);barGeometry(lines,bar,settings,scale,fraction);
  for(const auto& l:lines)list->AddLine({l.a.x,l.a.y},{l.b.x,l.b.y},previewColor(l.color),std::max(1.f,l.width*scale));
  list->AddCircleFilled({center.x,center.y-16*scale},9*scale,IM_COL32(226,235,242,255));list->AddRectFilled({center.x-16*scale,center.y-5*scale},{center.x+16*scale,center.y+24*scale},IM_COL32(150,173,190,255),5*scale);list->AddLine({center.x-23*scale,center.y},{center.x+23*scale,center.y},IM_COL32(226,235,242,255),4*scale);list->AddLine({center.x-10*scale,center.y+24*scale},{center.x-16*scale,center.y+39*scale},IM_COL32(150,173,190,255),5*scale);list->AddLine({center.x+10*scale,center.y+24*scale},{center.x+16*scale,center.y+39*scale},IM_COL32(150,173,190,255),5*scale);
  list->AddText({center.x-20*scale,center.y+46*scale},IM_COL32(220,233,242,255),"TANK");list->AddCircle({center.x,center.y},3*scale,previewColor(colors.direction));
  ImGui::SetCursorPos({12,h-56});ImGui::TextUnformatted(u8"方向键移动 · 横竖切换立即预览");ImGui::TextUnformatted(u8"两秒演示循环；范围为样式示意");
 }ImGui::End();ImGui::PopStyleVar();ImGui::PopStyleColor();
}
}
