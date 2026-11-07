#pragma once
#include "entity_transform_runtime.hpp"
#include "entity_menu.hpp"
namespace visual::deform {
inline bool aimValid=true;
inline std::string editKey,editLabel=u8"所有实体默认值";inline unsigned editField=0;inline float step=.05f;
inline std::map<int,std::string> known;inline std::map<std::uintptr_t,std::pair<std::string,std::string>> choices;
inline Rule editRule(const Settings& s){if(editKey.empty())return s.all;return inherited(s,editKey);}
inline void store(std::shared_ptr<Settings> s){std::atomic_store(&options,std::shared_ptr<const Settings>(std::move(s)));}
inline std::string percent(float f){return std::to_string(int(std::round(f*100)))+"%";}
inline bool editPage(std::uintptr_t page){return page==820||page==0x700004||page==0x700005||(page>=0x710000&&page<0x720000);}
inline bool isPage(std::uintptr_t page){return page==800||editPage(page)||page==822||(page>=810&&page<815);}
inline std::string page(std::uintptr_t id){auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::snapshot);for(const auto& c:frame->classCatalog)known[c.first]=c.second;for(const auto& r:frame->records)known[r.classId]=r.className;
 if(id==800){std::string rows=menuItem(cfg->enabled?u8"实体形变总开关：开":u8"实体形变总开关：关（默认）",0x700001)+menuItem(cfg->studioOnly?u8"处理路径：Studio兼容":u8"处理路径：自动（构建优先）",0x700002);
 rows+=menuItem(u8"骨骼上限："+std::to_string(boneLimit.load())+u8"（切换128/256）",118);
 for(unsigned g=0;g<EntityGroupCount;++g)rows+=menuItem(groupName(EntityGroup(g)),810+g);
 return menuPage(u8"实体缩放与拉伸",rows+menuItem(u8"调整所有实体默认值",0x700004)+menuItem(u8"准星识别后调整",0x700005)+menuItem(u8"一键关闭所有类别",0x700003)+menuItem(u8"运行状态",822)+menuItem(u8"保存设置",101));}
 if(id>=810&&id<815){auto group=EntityGroup(id-810);std::string rows;choices.clear();auto add=[&](const std::string& key,const std::string& label){auto action=uintptr_t(0x710000+choices.size());choices[action]={key,label};rows+=menuItem(label,action);};add("g:"+std::to_string(unsigned(group)),u8"本分类默认值");
 if(group==EntityGroup::Special){for(unsigned i=0;i<SpecialCount;++i)add("s:"+std::to_string(i),SpecialNames[i]);}
 else if(group==EntityGroup::Survivors){for(unsigned i=0;i<SurvivorCount;++i)add("v:"+std::to_string(i),SurvivorNames[i]);}
 else {for(auto name:BuiltinEntityClasses)if(groupForName(name)==group)add("t:"+std::string(name),name);for(const auto& c:known)if(c.first>=0&&builtinClassIndex(c.second)<0&&groupForName(c.second)==group)add("c:"+std::to_string(c.first)+":"+c.second,"#"+std::to_string(c.first)+" "+c.second);}
 return menuPage(std::string(u8"实体形变 · ")+groupName(group),rows);}
 if(id==0x700005&&!aimValid)return menuPage(u8"实体形变 · 识别",menuRow(u8"未识别到模型实体，请对准目标后重试")+menuItem(u8"重新识别",0x700005));
 if(editPage(id)){auto r=editRule(*cfg);std::string rows=menuItem(r.enabled?u8"本项：开":u8"本项：关",0x700010);const char* labels[]={u8"整体大小",u8"世界X轴比例",u8"世界Y轴比例",u8"世界Z轴比例"};float values[]={r.size,r.x,r.y,r.z};for(unsigned i=0;i<4;++i)rows+=menuItem(std::string(i==editField?"> ":"  ")+labels[i]+" "+percent(values[i]),0x700020+i);
 return menuPage(std::string(u8"形变参数 · ")+editLabel,rows+menuItem(u8"调整步长："+percent(step),0x700011)+menuItem(u8"恢复本项原大小并关闭",0x700012)+menuItem(u8"删除本项独立设置（继承分类）",0x700013)+menuItem(u8"保存设置",101)+menuRow(u8"先选择参数，按 - / = 调整；默认100%无变化")+menuRow(u8"整体大小与各轴倍率相乘，最终限制10%至500%"));}
 std::string rows=menuRow(status.load())+menuRow(u8"构建 / 字段 / Studio："+std::to_string(builds.load())+" / "+std::to_string(fields.load())+" / "+std::to_string(studioDraws.load()))+menuRow(u8"还原 / 拒绝："+std::to_string(restores.load())+" / "+std::to_string(failures.load()));
 std::map<std::string,const char*> reasons;std::map<std::string,std::array<unsigned,5>> counts;auto active=std::atomic_load(&targets);for(const auto& t:active->bases){++counts[t.second->label][std::min(4u,t.second->state->route.load())];reasons[t.second->label]=t.second->state->reason.load();}
 for(const auto& c:counts){rows+=menuRow(c.first+u8"：等待 / 构建 / 字段 / Studio / 拒绝 = "+std::to_string(c.second[0])+" / "+std::to_string(c.second[1])+" / "+std::to_string(c.second[2])+" / "+std::to_string(c.second[3])+" / "+std::to_string(c.second[4]));rows+=menuRow(reasons[c.first]);}
 return menuPage(u8"实体形变 · 状态",rows+menuRow(u8"无Studio模型的地图刷子、光束等不支持此骨骼形变")+menuRow(u8"第一人称手与枪一同缩放，不改服务器碰撞盒"));
}
inline bool action(std::uintptr_t id){if(id>=0x710000&&id<0x720000){auto i=choices.find(id);if(i!=choices.end()){editKey=i->second.first;editLabel=i->second.second;}return true;}
 if(id<0x700001||id>0x700023)return false;auto s=std::make_shared<Settings>(*std::atomic_load(&options));
 if(id==0x700001)s->enabled=!s->enabled;else if(id==0x700002)s->studioOnly=!s->studioOnly;else if(id==0x700003){s->all.enabled=false;for(auto& r:s->rules)r.second.enabled=false;}else if(id==0x700004){editKey.clear();editLabel=u8"所有实体默认值";}else if(id==0x700005){aimValid=false;auto f=std::atomic_load(&source_entities::snapshot);auto c=std::atomic_load(&source_camera::snapshot);auto aim=identifyAim(*f,*c,GetTickCount64());for(const auto& r:f->records)if(r.handle==aim.handle&&aim.classId>=0){aimValid=true;editKey=keys(r).front();editLabel="#"+std::to_string(r.classId)+" "+r.className;break;}}
 else if(id>=0x700020&&id<=0x700023)editField=unsigned(id-0x700020);else if(id==0x700011)step=step<.02f?.05f:step<.09f?.1f:.01f;else if(id==0x700013){if(!editKey.empty())s->rules.erase(editKey);else s->all=Rule{};}else {auto r=editRule(*s);if(id==0x700010)r.enabled=!r.enabled;if(id==0x700012)r=Rule{};if(editKey.empty())s->all=r;else s->rules[editKey]=r;}
 store(s);return true;
}
inline bool adjust(std::uintptr_t page,int delta){if(!editPage(page)||(page==0x700005&&!aimValid))return false;if(!delta)return true;auto s=std::make_shared<Settings>(*std::atomic_load(&options));auto r=editRule(*s);float* values[]={&r.size,&r.x,&r.y,&r.z};*values[editField]=std::round((*values[editField]+step*(delta>0?1:-1))*100)/100;normalize(r);if(editKey.empty())s->all=r;else s->rules[editKey]=r;store(s);return true;}
}
