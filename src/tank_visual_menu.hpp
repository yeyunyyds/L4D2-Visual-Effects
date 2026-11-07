#pragma once
#include "tank_visual_state.hpp"
#include "entity_menu.hpp"
#include "imgui/imgui.h"
#include <windows.h>
namespace visual::tank {
inline std::atomic<FineField> barField{FineField::BarSize},arrowField{FineField::ArrowWidth};
inline std::string fineNumber(float value){int t=int(std::lround(value*10));return std::to_string(t/10)+"."+std::to_string(t%10);}
inline std::string percent(float value){return std::to_string(int(std::lround(value*100)))+"%";}
inline std::string visualMenu(){auto s=std::atomic_load(&visualSettings);return menuPage(u8"Tank 样式与位置预览",
 menuItem(std::string(u8"预设：")+visualName(s->preset),360)+menuItem(s->vertical?u8"计时条：竖条（切换）":u8"计时条：横条（切换）",361)+menuItem(u8"移动步长："+std::to_string(s->step),362)+menuItem(u8"进度条大小与粗细",325)+menuItem(u8"朝向箭头大小与粗细",326)+menuItem(u8"攻击范围线宽",327)+menuItem(u8"重置位置与方向",363)+menuItem(u8"保存设置",101)+menuRow(u8"方向键移动 · X "+std::to_string(int(s->x))+u8" / Y "+std::to_string(int(s->y))));}
inline std::string visualFineMenu(std::uintptr_t page){auto s=std::atomic_load(&visualSettings);auto selected=page==325?barField.load():page==326?arrowField.load():FineField::RangeWidth;
 auto label=[&](FineField field,const std::string& text){return (selected==field?std::string("> "):std::string())+text;};
 if(page==325)return menuPage(u8"Tank · 进度条精调",menuItem(label(FineField::BarSize,u8"整体大小："+percent(s->barSize)),380)+menuItem(label(FineField::Length,u8"长度："+fineNumber(s->length)),381)+menuItem(label(FineField::Thickness,u8"厚度："+fineNumber(s->thickness)),382)+menuItem(s->vertical?u8"方向：竖条（切换）":u8"方向：横条（切换）",361)+menuItem(u8"恢复进度条尺寸默认",383)+menuItem(u8"保存设置",101)+menuRow(u8"先选参数，再按 - / = 微调")+menuRow(u8"大小每次 1%；长度 1；厚度 0.1"));
 if(page==326)return menuPage(u8"Tank · 朝向箭头精调",menuItem(label(FineField::ArrowWidth,u8"线宽倍率："+fineNumber(s->arrowWidth)),384)+menuItem(label(FineField::ArrowSize,u8"箭头大小："+percent(s->arrowSize)),385)+menuItem(u8"恢复箭头默认",387)+menuItem(u8"保存设置",101)+menuRow(u8"先选参数，再按 - / = 微调")+menuRow(u8"线宽每次 0.1 倍；大小每次 1%"));
 return menuPage(u8"Tank · 攻击范围精调",menuItem(label(FineField::RangeWidth,u8"线宽倍率："+fineNumber(s->rangeWidth)),386)+menuItem(u8"恢复范围线宽默认",388)+menuItem(u8"保存设置",101)+menuRow(u8"按 - / = 每次微调 0.1 倍")+menuRow(u8"仅调整显示线宽，不改变攻击距离"));
}
inline bool visualAdjust(std::uintptr_t page,int delta){if(!finePage(page))return false;if(!delta)return true;auto s=*std::atomic_load(&visualSettings);auto field=page==325?barField.load():page==326?arrowField.load():FineField::RangeWidth;adjustFine(s,field,delta);std::atomic_store(&visualSettings,std::make_shared<const VisualSettings>(s));return true;}
inline bool visualAction(std::uintptr_t id){if((id<360||id>366)&&(id<370||id>373)&&(id<380||id>388))return false;auto s=*std::atomic_load(&visualSettings);
 if(id==360)s.preset=(s.preset+1)%3;if(id==361)s.vertical=!s.vertical;if(id==362)s.step=s.step==2?4:s.step==4?8:s.step==8?16:2;
 if(id==363){s.x=0;s.y=-58;s.vertical=false;}
 if(id==364)display::resumeRequested=true;if(id==365)s.length=s.length<88?88:s.length<112?112:s.length<136?136:64;if(id==366)s.thickness=s.thickness<6?6:s.thickness<8?8:s.thickness<10?10:4;
 if(id==370)s.showDirection=!s.showDirection;if(id==371)s.showRange=!s.showRange;if(id==372)s.showBar=!s.showBar;if(id==373)s.showTarget=!s.showTarget;
 if(id==380)barField=FineField::BarSize;if(id==381)barField=FineField::Length;if(id==382)barField=FineField::Thickness;if(id==384)arrowField=FineField::ArrowWidth;if(id==385)arrowField=FineField::ArrowSize;
 if(id==383){s.barSize=.8f;s.length=88;s.thickness=8;}if(id==387){s.arrowWidth=2;s.arrowSize=1;}if(id==388)s.rangeWidth=2;
 normalizeVisual(s);std::atomic_store(&visualSettings,std::make_shared<const VisualSettings>(s));return true;}
inline ImU32 previewColor(std::uint32_t c){return IM_COL32((c>>16)&255,(c>>8)&255,c&255,(c>>24)&255);}
inline bool visualPreviewKeys(std::uintptr_t id){return id==321||finePage(id);}
inline void visualPreview(std::uintptr_t page,float width,float height,bool focused){
 static std::array<bool,4> held{};static std::array<std::uint64_t,4> nextRepeat{};bool editing=page==321;
 if(!editing){held.fill(false);nextRepeat.fill(0);if(!finePage(page))return;}
 auto settings=*std::atomic_load(&visualSettings);bool changed=false;const int codes[]={VK_LEFT,VK_UP,VK_RIGHT,VK_DOWN};auto now=GetTickCount64();
 for(unsigned i=0;i<4;++i){bool down=editing&&focused&&(GetAsyncKeyState(codes[i])&0x8000)!=0;if(down&&(!held[i]||now>=nextRepeat[i])){if(i==0)settings.x-=settings.step;if(i==1)settings.y-=settings.step;if(i==2)settings.x+=settings.step;if(i==3)settings.y+=settings.step;changed=true;nextRepeat[i]=now+(held[i]?90:300);}held[i]=down;}
 if(changed){normalizeVisual(settings);std::atomic_store(&visualSettings,std::make_shared<const VisualSettings>(settings));}
 float w=std::min(540.f,width*.47f),h=std::min(520.f,height-24);ImGui::SetNextWindowPos({width-w-12,(height-h)*.5f},ImGuiCond_Always);ImGui::SetNextWindowSize({w,h},ImGuiCond_Always);ImGui::SetNextWindowBgAlpha(.94f);
 ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.025f,.045f,.065f,1));ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,10.f);
 if(ImGui::Begin("Tank visual preview",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar)){
  auto pos=ImGui::GetWindowPos();auto list=ImGui::GetWindowDrawList();float scale=std::max(.1f,std::min((w-30)/540.f,(h-90)/460.f));Point center{pos.x+w*.5f,pos.y+h*.52f};auto colors=palette(settings.preset);
  ImGui::TextColored(ImVec4(.8f,.9f,1,1),"%s",visualName(settings.preset));ImGui::TextUnformatted(u8"实体相对位置 · 远近透视缩放");
  for(int i=-4;i<=4;++i){float d=i*50*scale;list->AddLine({center.x+d,center.y-220*scale},{center.x+d,center.y+220*scale},IM_COL32(35,50,66,110));list->AddLine({center.x-250*scale,center.y+d},{center.x+250*scale,center.y+d},IM_COL32(35,50,66,110));}
  std::vector<Segment> lines;demoRange(lines,center,scale,settings);float fraction=1-float(now%2000)/2000.f;auto bar=barCenter(center,settings,scale);barGeometry(lines,bar,settings,scale,fraction);
  for(const auto& l:lines)list->AddLine({l.a.x,l.a.y},{l.b.x,l.b.y},previewColor(l.color),std::max(1.f,l.width*scale));
  list->AddCircleFilled({center.x,center.y-38*scale},12*scale,IM_COL32(226,235,242,255));list->AddRectFilled({center.x-24*scale,center.y-23*scale},{center.x+24*scale,center.y+28*scale},IM_COL32(150,173,190,255),5*scale);list->AddLine({center.x-34*scale,center.y-7*scale},{center.x+34*scale,center.y-7*scale},IM_COL32(226,235,242,255),4*scale);list->AddLine({center.x-10*scale,center.y+28*scale},{center.x-16*scale,center.y+60*scale},IM_COL32(150,173,190,255),5*scale);list->AddLine({center.x+10*scale,center.y+28*scale},{center.x+16*scale,center.y+60*scale},IM_COL32(150,173,190,255),5*scale);
  list->AddText({center.x-20*scale,center.y+72*scale},IM_COL32(220,233,242,255),"TANK");list->AddCircle({center.x,center.y},3*scale,previewColor(colors.direction));
  ImGui::SetCursorPos({12,h-56});ImGui::TextUnformatted(editing?u8"方向键移动 · 横竖切换立即预览":u8"- / = 微调选中参数 · 即时预览");ImGui::TextUnformatted(u8"左右跟随 Tank 朝向；上下固定世界高度");
 }ImGui::End();ImGui::PopStyleVar();ImGui::PopStyleColor();
}
}
