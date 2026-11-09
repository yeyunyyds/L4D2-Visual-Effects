#pragma once
#include "proximity.hpp"
#include <sstream>
namespace visual {
inline std::string menuRow(const std::string& label){return "\""+label+"\" { \"user_data\" \"0\" }\n";}
inline std::string menuItem(const std::string& label,std::uintptr_t id){std::ostringstream s;s<<"0x"<<std::hex<<id;return "\""+label+"\" { \"user_data\" \""+s.str()+"\" }\n";}
inline std::string menuPage(const std::string& title,const std::string& body){return "\""+title+"\" {"+body+"}";}
struct EntityMenuState : AimSession {
 std::map<int,std::string> known;
 void observe(const dango::inventory::Frame& frame,const EntitySelection& selection){
  if(aimEpoch!=frame.epoch)known.clear();
  validateEpoch(frame.epoch);
  for(const auto& c:frame.classCatalog)known[c.first]=c.second;
  for(const auto& r:frame.records)if(r.classId>=0&&r.classId<=4096&&!r.className.empty())known[r.classId]=r.className;
  for(const auto& c:selection.names)if(!known.count(c.first))known[c.first]=c.second;
  for(const auto& c:selection.classes)if(!known.count(c.first))known[c.first]="Class ID";
 }
 std::string root()const {
  std::string body;
  for(unsigned g=0;g<EntityGroupCount;++g){unsigned count=0;
   for(auto name:BuiltinEntityClasses)if(groupForName(name)==EntityGroup(g))++count;
   for(const auto& c:known)if(builtinClassIndex(c.second)<0&&groupForName(c.second)==EntityGroup(g))++count;
   if(EntityGroup(g)==EntityGroup::Survivors)count=SurvivorCount;
   if(EntityGroup(g)==EntityGroup::Special)count=SpecialCount;
   body+=menuItem(std::string(groupName(EntityGroup(g)))+" ("+std::to_string(count)+")",1000+g);
  }
  return menuPage(u8"实体分类",body+menuItem(u8"进入手动识别",104)+menuItem(u8"一键全部关闭（含实验）",121)+menuItem(u8"保存设置",101));
 }
 std::string group(EntityGroup group,const EntitySelection& selection)const {
  std::string body=menuItem(u8"开启本分类全部实体",130+unsigned(group)*2)+menuItem(u8"关闭本分类全部实体",131+unsigned(group)*2);
  if(group==EntityGroup::Special){
   for(unsigned role=0;role<SpecialCount;++role)body+=menuItem(std::string(SpecialNames[role])+(selection.selectedSpecial(int(role))?u8"：开":u8"：关"),0x60000+role);
   return menuPage(u8"特感 · 八种类型",body+menuItem(u8"保存设置",101));
  }
  if(group==EntityGroup::Survivors){
   for(unsigned role=0;role<SurvivorCount;++role)body+=menuItem(std::string(SurvivorNames[role])+(selection.selectedSurvivor(int(role))?u8"：开":u8"：关"),0x50000+role);
   return menuPage(u8"求生者 · 八名角色",body+menuItem(u8"保存设置",101));
  }
  for(unsigned i=0;i<BuiltinEntityClassCount;++i){std::string name=BuiltinEntityClasses[i];if(groupForName(name)!=group)continue;
   auto match=std::find_if(known.begin(),known.end(),[&](const auto& c){return c.second==name;});
   bool on=match==known.end()?selection.selectedType(name):selection.selected(match->first,name);
   std::string label=name;
   if(name=="Infected")label+=u8"（普通感染者，衣物可能不随动）";
   if(name=="CTerrorPlayer")label+=u8"（含感染者玩家）";
   body+=menuItem(label+(on?u8"：开":u8"：关"),0x30000+i);
  }
  for(const auto& c:known)if(builtinClassIndex(c.second)<0&&groupForName(c.second)==group)
   body+=menuItem("#"+std::to_string(c.first)+" "+c.second+(selection.selected(c.first,c.second)?u8"：开":u8"：关"),0x10000+unsigned(c.first));
  return menuPage(groupName(group),body+menuItem(u8"保存设置",101));
 }
 std::string recognition(const EntitySelection& selection,bool experimentActive=false)const {
  std::string body=menuItem(handsOnly?u8"识别模式：本机手持（切换）":u8"识别模式：场景本体（切换）",127)+menuItem(pending?u8"正在识别（点击重试）":u8"手动识别准星目标",122);
  if(aimed.classId>=0){
   body+=menuItem(u8"切换候选："+std::to_string(candidate+1)+" / "+std::to_string(candidates.empty()?1:candidates.size())+" · "+aimLabel(aimed),128);
   body+=menuRow(u8"当前识别 #"+std::to_string(aimed.classId)+" "+aimed.name);
   body+=menuRow(selection.selectedAim(aimed)?u8"手动类别开关：开":u8"手动类别开关：关");
   body+=menuItem(u8"开启该类型或角色",124)+menuItem(u8"关闭该类型或角色",125)+
    menuItem(u8"用作范围实验触发类别",191)+menuItem(u8"清除识别结果",126);
  }else body+=menuRow(u8"尚未识别；对准目标后选择手动识别");
  if(!aimStatus.empty())body+=menuRow(aimStatus);
  if(aimed.classId>=0&&aimed.attachment)body+=menuRow(u8"已手动选择附属实体，设置不会作用于角色本体");
  if(experimentActive)body+=menuRow(u8"范围实验已开，仅范围内实体生效");
  return menuPage(u8"手动识别与开关",body+menuItem(u8"保存设置",101));
 }
 std::string triggers()const {
  std::string body=menuItem(u8"使用 Tank（默认）",190);
  if(aimed.classId>=0)body+=menuItem(u8"使用上次识别："+aimed.name,191);
  for(unsigned i=0;i<BuiltinEntityClassCount;++i)body+=menuItem(BuiltinEntityClasses[i],0x40000+i);
  for(const auto& c:known)if(builtinClassIndex(c.second)<0)body+=menuItem("#"+std::to_string(c.first)+" "+c.second,0x20000+unsigned(c.first));
  return menuPage(u8"选择范围触发类别",body);
 }
};
inline std::string proximityMenu(const ProximitySettings& p,bool active,unsigned count,unsigned sources){
 return menuPage(u8"实验：目标范围触发",
  menuItem(p.enabled?u8"实验开关：开":u8"实验开关：关（默认）",180)+
  menuItem(u8"触发类别："+p.triggerName+(p.triggerClassId<0?"":" #"+std::to_string(p.triggerClassId)),109)+
  menuRow(u8"触发范围："+std::to_string(p.radius)+u8" 游戏单位")+
  menuItem(u8"范围 -100",181)+menuItem(u8"范围 +100",182)+menuItem(u8"范围 -10",183)+menuItem(u8"范围 +10",184)+
  menuItem(p.includeCommon?u8"实验中拉伸普通感染者：开":u8"实验中拉伸普通感染者：关",185)+
  menuRow(!p.enabled?u8"状态：关闭":active?u8"状态：范围内实体正在拉伸":u8"状态：待机")+
  menuRow(u8"有效触发目标："+std::to_string(sources)+u8"；范围内实体："+std::to_string(count))+
  menuRow(u8"逐实体计算距离；范围外关闭，手持道具排除")+
  menuItem(u8"恢复实验默认（关闭）",186)+menuItem(u8"保存设置",101));
}
} // namespace visual
