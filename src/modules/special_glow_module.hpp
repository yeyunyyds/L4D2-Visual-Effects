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
  std::string rows=ui::item(settings->all()?u8"一键关闭全部特感描边":u8"一键开启全部特感描边",4480);
  rows+=ui::item(settings->allOccluded()?u8"一键关闭全部墙壁遮蔽":u8"一键开启全部墙壁遮蔽",4481);
  for(unsigned i=0;i<SpecialCount;++i)
   rows+=ui::item(std::string(SpecialNames[i])+(settings->types[i].enabled?u8" [开]":u8" [关]"),950+i);
  return "\"特感描边 · 独立配置\" {"+rows+ui::item(u8"保存设置",101)+
   ui::row(u8"批量操作不更改独立颜色；单类设置可覆盖")+"}";
 }
 if(id>=960&&id<960+SpecialCount){
  unsigned role=unsigned(id-960);
  const auto& current=settings->types[role];
  std::string rows=ui::row(u8"当前颜色："+specialglow::colorText(current.color));
  for(unsigned i=0;i<specialglow::palette.size();++i){
   std::string label=std::string(current.color==specialglow::palette[i]?u8"[已选择] ":u8"")+
      specialglow::paletteNames[i]+" · "+specialglow::colorText(specialglow::palette[i]);
   rows+=ui::item(label,4400+role*unsigned(specialglow::palette.size())+i);
  }
  return "\""+std::string(SpecialNames[role])+u8" · 颜色预设\" {"+rows+
   ui::item(u8"保存设置",101)+ui::row(u8"选择立即生效；返回角色页可继续微调 RGB")+"}";
 }
 if(id<950||id>=950+SpecialCount)return {};
 unsigned role=unsigned(id-950);
 const auto& current=settings->types[role];
 std::string out=ui::item(current.enabled?u8"描边：[开]":u8"描边：[关]",4000+role);
 out+=ui::item(current.occlude?u8"建筑遮挡：[开] · 不透视":u8"建筑遮挡：[关] · 穿墙",4010+role);
 out+=ui::item(u8"颜色预设 · 选择（"+specialglow::colorText(current.color)+")",960+role);
 for(unsigned component=0;component<3;++component){
  std::string name=component==0?"R":component==1?"G":"B";
  unsigned idBase=4100+role*6+component*2;
  out+=ui::item(name+" +16",idBase);
  out+=ui::item(name+" -16",idBase+1);
 }
 return "\""+std::string(SpecialNames[role])+u8" · 原生描边\" {"+out+
  ui::item(u8"保存设置",101)+ui::row(u8"遮挡仅检测世界几何；非逐像素遮挡")+
  ui::row(u8"原生 Type 2 在部分模型上可能不生效")+"}";
}
inline bool action(std::uintptr_t id){
 if(id==4480||id==4481){
  auto updated=*std::atomic_load(&specialglow::options);
  if(id==4480)updated.setAllEnabled(!updated.all());
  else updated.setAllOccluded(!updated.allOccluded());
  std::atomic_store(&specialglow::options,std::make_shared<const specialglow::Settings>(std::move(updated)));
  ui::saveMessage=u8"特感描边批量设置未保存";
  return true;
 }
 unsigned role=SpecialCount,kind=0,component=0,preset=0;int step=0;
 if(id>=4000&&id<4000+SpecialCount){role=unsigned(id-4000);kind=1;}
 else if(id>=4010&&id<4010+SpecialCount){role=unsigned(id-4010);kind=2;}
 else if(id>=4100&&id<4100+SpecialCount*6){role=unsigned(id-4100)/6;auto change=unsigned(id-4100)%6;kind=3;component=change/2;step=(change%2)?-16:16;}
 else if(id>=4400&&id<4400+SpecialCount*specialglow::palette.size()){
  auto index=unsigned(id-4400);role=index/unsigned(specialglow::palette.size());preset=index%unsigned(specialglow::palette.size());kind=4;
 }
 if(role>=SpecialCount)return false;
 auto updated=*std::atomic_load(&specialglow::options);auto& current=updated.types[role];
 if(kind==1)current.enabled=!current.enabled;
 if(kind==2)current.occlude=!current.occlude;
 if(kind==3)current.color=specialglow::stepChannel(current.color,component,step);
 if(kind==4)current.color=specialglow::palette[preset];
 std::atomic_store(&specialglow::options,std::make_shared<const specialglow::Settings>(std::move(updated)));
 ui::saveMessage=u8"特感描边设置未保存";
 return true;
}
inline modules::Module create(){
 modules::Module m;m.id="special-glow";m.menuOrder=2;
 m.rootItem=[] {auto cfg=std::atomic_load(&specialglow::options);
  return ui::item(cfg->all()?u8"特感描边 [全开]":
    cfg->any()?u8"特感描边 [部分开启]":u8"特感描边 [全关]",940);};
 m.page=[](std::uintptr_t id)->std::optional<std::string>{
  if(id==940||(id>=950&&id<950+SpecialCount)||(id>=960&&id<960+SpecialCount))return page(id);return std::nullopt;
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
