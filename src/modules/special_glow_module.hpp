#pragma once
#include "../module_host.hpp"
#include "../ui_services.hpp"
#include "../special_glow_settings.hpp"
#include "../entity_visibility_runtime.hpp"
#include "../tank_outline.hpp"
namespace visual::specialglowfeature {
inline std::string page(std::uintptr_t id){
 auto settings=std::atomic_load(&specialglow::options);
 if(id==940){
  std::string rows;
  for(unsigned i=0;i<SpecialCount;++i)
   rows+=ui::item(std::string(SpecialNames[i])+(settings->types[i].enabled?u8" [开]":u8" [关]"),950+i);
  return "\"特感描边 · 独立配置\" {"+rows+ui::item(u8"保存设置",101)+
   ui::row(u8"默认全部关闭，独立设置颜色及是否穿墙")+"}";
 }
 unsigned role=unsigned(id-950);
 if(id<950||role>=SpecialCount)return {};
 const auto& s=settings->types[role];
 std::string out=ui::item(s.enabled?u8"描边：[开]":u8"描边：[关]",1000+role);
 out+=ui::item(s.occlude?u8"建筑遮挡：[开] · 不透视":u8"建筑遮挡：[关] · 穿墙",1010+role);
 out+=ui::item(u8"轮换预设颜色："+specialglow::colorText(s.color),1020+role);
 for(unsigned component=0;component<3;++component){
  std::string name=component==0?"R":component==1?"G":"B";
  unsigned idBase=1300+role*6+component*2;
  out+=ui::item(name+" +16",idBase);
  out+=ui::item(name+" -16",idBase+1);
 }
 return "\""+std::string(SpecialNames[role])+u8" · 原生描边\" {"+out+
  ui::item(u8"保存设置",101)+ui::row(u8"遮挡仅检测世界几何；非逐像素遮挡")+
  ui::row(u8"原生 Type 2 在部分模型上可能不生效")+"}";
}
inline bool action(std::uintptr_t id){
 unsigned role=SpecialCount,kind=0,component=0;int step=0;
 if(id>=1000&&id<1000+SpecialCount){role=unsigned(id-1000);kind=1;}
 else if(id>=1010&&id<1010+SpecialCount){role=unsigned(id-1010);kind=2;}
 else if(id>=1020&&id<1020+SpecialCount){role=unsigned(id-1020);kind=3;}
 else if(id>=1300&&id<1300+SpecialCount*6){role=unsigned(id-1300)/6;auto change=unsigned(id-1300)%6;kind=4;component=change/2;step=(change%2)?-16:16;}
 if(role>=SpecialCount)return false;
 auto updated=*std::atomic_load(&specialglow::options);auto& current=updated.types[role];
 if(kind==1)current.enabled=!current.enabled;
 if(kind==2)current.occlude=!current.occlude;
 if(kind==3)current.color=specialglow::nextColor(current.color);
 if(kind==4)current.color=specialglow::stepChannel(current.color,component,step);
 std::atomic_store(&specialglow::options,std::make_shared<const specialglow::Settings>(std::move(updated)));
 ui::saveMessage=u8"特感描边设置未保存";
 return true;
}
inline modules::Module create(){
 modules::Module m;m.id="special-glow";m.menuOrder=2;
 m.rootItem=[] {return ui::item(std::atomic_load(&specialglow::options)->any()?
  u8"特感描边 [部分开启]":u8"特感描边 [全关]",940);};
 m.page=[](std::uintptr_t id)->std::optional<std::string>{
  if(id==940||(id>=950&&id<950+SpecialCount))return page(id);return std::nullopt;
 };
 m.action=action;
 m.needsEntities=[] {return std::atomic_load(&specialglow::options)->any()||
  visibility::requested()||!tank::outline::clean.load();};
 m.frame=[](const dango::GameCameraFrame& camera){
  visibility::onFrame(camera);
  tank::outline::onFrame(camera);
 };
 m.load=[](const nlohmann::json& j){std::atomic_store(&specialglow::options,
  std::make_shared<const specialglow::Settings>(specialglow::readSettings(j)));};
 m.save=[](nlohmann::json& j){specialglow::writeSettings(j,*std::atomic_load(&specialglow::options));};
 m.diagnostics=[] {return "active="+std::to_string(tank::outline::activeCount.load())+
  " LOS="+std::string(visibility::status.load());};
 m.quiesce=[] {tank::outline::stop();};
 return m;
}
} // namespace visual::specialglowfeature
