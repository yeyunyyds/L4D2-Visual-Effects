#pragma once
#include "proximity.hpp"
#include <sstream>
namespace visual {
inline std::string menuRow(const std::string& label){return "\""+label+"\" { \"user_data\" \"0\" }\n";}
inline std::string menuItem(const std::string& label,std::uintptr_t id){std::ostringstream s;s<<"0x"<<std::hex<<id;return "\""+label+"\" { \"user_data\" \""+s.str()+"\" }\n";}
inline std::string menuPage(const std::string& title,const std::string& body){return "\""+title+"\" {"+body+"}";}
struct EntityMenuState {
 std::map<int,std::string> known;
 AimSelection aimed;
 void observe(const dango::inventory::Frame& frame,const EntitySelection& selection){
  for(const auto& c:frame.classCatalog)known[c.first]=c.second;
  for(const auto& r:frame.records)if(r.classId>=0&&r.classId<=4096&&!r.className.empty())known[r.classId]=r.className;
  for(const auto& c:selection.names)if(!known.count(c.first))known[c.first]=c.second;
  for(const auto& c:selection.classes)if(!known.count(c.first))known[c.first]="Class ID";
 }
 std::string root()const {
  std::string body;
  for(unsigned g=0;g<EntityGroupCount;++g){unsigned count=0;
   for(const auto& c:known)if(groupForName(c.second)==EntityGroup(g))++count;
   body+=menuItem(std::string(groupName(EntityGroup(g)))+" ("+std::to_string(count)+")",1000+g);
  }
  return menuPage(u8"实体分类",body+menuItem(u8"进入手动识别",104)+menuItem(u8"一键全部关闭（含实验）",121)+menuItem(u8"保存设置",101));
 }
 std::string group(EntityGroup group,const EntitySelection& selection)const {
  std::string body=menuItem(u8"开启本分类全部实体",130+unsigned(group)*2)+menuItem(u8"关闭本分类全部实体",131+unsigned(group)*2);
  for(const auto& c:known)if(groupForName(c.second)==group){
   std::string label="#"+std::to_string(c.first)+" "+c.second;
   if(c.second=="Infected")label+=u8"（普通感染者，衣物可能不随动）";
   if(c.second=="CTerrorPlayer")label+=u8"（含感染者玩家）";
   body+=menuItem(label+(selection.selected(c.first,c.second)?u8"：开":u8"：关"),0x10000+unsigned(c.first));
  }
  return menuPage(groupName(group),body+menuItem(u8"保存设置",101));
 }
 std::string recognition(const EntitySelection& selection,bool experimentActive=false)const {
  std::string body=menuItem(u8"手动识别准星附近实体",122);
  if(aimed.classId>=0){
   body+=menuRow(u8"上次识别 #"+std::to_string(aimed.classId)+" "+aimed.name);
   body+=menuRow(selection.selected(aimed.classId,aimed.name)?u8"手动类别开关：开":u8"手动类别开关：关");
   body+=menuItem(u8"开启该 Class ID 的全部实体",124)+menuItem(u8"关闭该 Class ID 的全部实体",125)+
    menuItem(u8"用作范围实验触发类别",191)+menuItem(u8"清除识别结果",126);
  }else body+=menuRow(u8"尚未识别；对准目标后选择手动识别");
  if(experimentActive)body+=menuRow(u8"范围实验触发中，正临时全开");
  return menuPage(u8"手动识别与开关",body+menuItem(u8"保存设置",101));
 }
 std::string triggers()const {
  std::string body=menuItem(u8"使用 Tank（默认）",190);
  if(aimed.classId>=0)body+=menuItem(u8"使用上次识别："+aimed.name,191);
  for(const auto& c:known)body+=menuItem("#"+std::to_string(c.first)+" "+c.second,0x20000+unsigned(c.first));
  return menuPage(u8"选择范围触发类别",body);
 }
};
inline std::string proximityMenu(const ProximitySettings& p,bool active,float distance){
 return menuPage(u8"实验：目标范围触发",
  menuItem(p.enabled?u8"实验开关：开":u8"实验开关：关（默认）",180)+
  menuItem(u8"触发类别："+p.triggerName+(p.triggerClassId<0?"":" #"+std::to_string(p.triggerClassId)),109)+
  menuRow(u8"触发范围："+std::to_string(p.radius)+u8" 游戏单位")+
  menuItem(u8"范围 -100",181)+menuItem(u8"范围 +100",182)+menuItem(u8"范围 -10",183)+menuItem(u8"范围 +10",184)+
  menuItem(p.includeCommon?u8"实验中拉伸普通感染者：开":u8"实验中拉伸普通感染者：关",185)+
  menuRow(!p.enabled?u8"状态：关闭":active?u8"状态：范围内，临时全开":u8"状态：待机")+
  menuRow(distance<0?u8"附近未发现有效触发目标":u8"最近目标距离："+std::to_string(int(distance))+u8" 游戏单位")+
  menuRow(u8"手持道具排除；离开范围恢复手动分类")+
  menuItem(u8"恢复实验默认（关闭）",186)+menuItem(u8"保存设置",101));
}
} // namespace visual
