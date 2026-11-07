#pragma once
#include "entity_transform_runtime.hpp"
#include "entity_menu.hpp"
namespace visual::deform {
inline AimTarget aimed;
inline const char* notice=u8"实体形变设置未保存";
inline std::string editKey,editLabel=u8"所有实体默认值";inline unsigned editField=0;inline float step=.05f;
inline std::map<int,std::string> known;inline std::map<std::uintptr_t,std::pair<std::string,std::string>> choices;
inline Rule editRule(const Settings& s){if(editKey.empty())return s.all;if(aimed.valid&&editKey==aimed.key)return aimed.rule(s);return inherited(s,editKey);}
inline void store(std::shared_ptr<Settings> s){std::atomic_store(&options,std::shared_ptr<const Settings>(std::move(s)));}
inline std::string percent(float f){return std::to_string(int(std::round(f*100)))+"%";}
inline bool editPage(std::uintptr_t page){return page==820||page==0x700004||page==0x700006||(page>=0x710000&&page<0x720000);}
inline bool isPage(std::uintptr_t page){return page==800||editPage(page)||page==822||page==0x700005||(page>=810&&page<815);}
inline Rule selectedAimRule(const Settings& s){return aimed.rule(s);}
inline std::string page(std::uintptr_t id){auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::snapshot);for(const auto& c:frame->classCatalog)known[c.first]=c.second;for(const auto& r:frame->records)known[r.classId]=r.className;
 aimed.validate(frame->epoch);
 if(id==800){std::string rows=menuItem(cfg->enabled?u8"实体形变总开关：开":u8"实体形变总开关：关（默认）",0x700001)+menuItem(cfg->studioOnly?u8"处理路径：Studio兼容":u8"处理路径：自动（构建优先）",0x700002);
 rows+=menuItem(u8"骨骼上限："+std::to_string(boneLimit.load())+u8"（切换128/256）",118)+menuItem(u8"准星识别与调整",0x700005);
 for(unsigned g=0;g<EntityGroupCount;++g)rows+=menuItem(groupName(EntityGroup(g)),810+g);
 return menuPage(u8"实体缩放与拉伸",rows+menuItem(u8"调整所有实体默认值",0x700004)+menuItem(u8"一键关闭所有类别",0x700003)+menuItem(u8"运行状态",822)+menuItem(u8"保存设置",101));}
 if(id>=810&&id<815){auto group=EntityGroup(id-810);std::string rows;choices.clear();auto add=[&](const std::string& key,const std::string& label){auto action=uintptr_t(0x710000+choices.size());choices[action]={key,label};rows+=menuItem(label,action);};add("g:"+std::to_string(unsigned(group)),u8"本分类默认值");
 if(group==EntityGroup::Special){for(unsigned i=0;i<SpecialCount;++i)add("s:"+std::to_string(i),SpecialNames[i]);}
 else if(group==EntityGroup::Survivors){for(unsigned i=0;i<SurvivorCount;++i)add("v:"+std::to_string(i),SurvivorNames[i]);}
 else {for(auto name:BuiltinEntityClasses)if(groupForName(name)==group)add("t:"+std::string(name),name);for(const auto& c:known)if(c.first>=0&&builtinClassIndex(c.second)<0&&groupForName(c.second)==group)add("c:"+std::to_string(c.first)+":"+c.second,"#"+std::to_string(c.first)+" "+c.second);}
 return menuPage(std::string(u8"实体形变 · ")+groupName(group),rows);}
 if(id==0x700005){std::string rows=menuItem(u8"手动识别准星附近实体",0x700030);
 if(!aimed.valid)return menuPage(u8"实体形变 · 准星识别",rows+menuRow(u8"暂无识别结果，请对准目标后手动识别")+menuRow(u8"识别只选择类别，不会自动开启形变"));
 auto r=selectedAimRule(*cfg);rows+=menuItem(r.enabled?u8"本类别：开（点击关闭）":u8"本类别：关（点击开启）",0x700031)+menuItem(u8"调整本类别大小与拉伸",0x700006)+menuItem(u8"清除识别结果",0x700032)+menuItem(u8"保存设置",101);
 rows+=menuRow(u8"识别类型："+aimed.label)+menuRow("Class ID: "+std::to_string(aimed.classId)+" / "+aimed.className)+menuRow(u8"整体 / X / Y / Z："+percent(r.size)+" / "+percent(r.x)+" / "+percent(r.y)+" / "+percent(r.z));
 rows+=menuRow(cfg->enabled?u8"总开关已开启":u8"总开关关闭中，类别设置暂不生效")+menuRow(u8"影响同类型或角色的全部实体，非单只实体");
 return menuPage(std::string(u8"实体形变 · ")+aimed.label,rows);}
 if(id==0x700006&&!aimed.valid)return menuPage(u8"实体形变 · 识别已失效",menuRow(u8"请返回识别页面，重新手动识别"));
 if(editPage(id)){auto r=editRule(*cfg);std::string rows=menuItem(r.enabled?u8"本项：开":u8"本项：关",0x700010);const char* labels[]={u8"整体大小",u8"世界X轴比例",u8"世界Y轴比例",u8"世界Z轴比例"};float values[]={r.size,r.x,r.y,r.z};for(unsigned i=0;i<4;++i)rows+=menuItem(std::string(i==editField?"> ":"  ")+labels[i]+" "+percent(values[i]),0x700020+i);
 return menuPage(std::string(u8"形变参数 · ")+editLabel,rows+menuItem(u8"调整步长："+percent(step),0x700011)+menuItem(u8"恢复本项原大小并关闭",0x700012)+menuItem(u8"删除本项独立设置（继承分类）",0x700013)+menuItem(u8"保存设置",101)+menuRow(u8"先选择参数，按 - / = 调整；默认100%无变化")+menuRow(u8"整体大小与各轴倍率相乘，最终限制10%至500%"));}
 std::string rows=menuRow(status.load())+menuRow(u8"构建 / 字段 / Studio："+std::to_string(builds.load())+" / "+std::to_string(fields.load())+" / "+std::to_string(studioDraws.load()))+menuRow(u8"还原 / 拒绝："+std::to_string(restores.load())+" / "+std::to_string(failures.load()));
 std::map<std::string,const char*> reasons;std::map<std::string,std::array<unsigned,5>> counts;auto active=std::atomic_load(&targets);for(const auto& t:active->bases){++counts[t.second->label][std::min(4u,t.second->state->route.load())];reasons[t.second->label]=t.second->state->reason.load();}
 for(const auto& c:counts){rows+=menuRow(c.first+u8"：等待 / 构建 / 字段 / Studio / 拒绝 = "+std::to_string(c.second[0])+" / "+std::to_string(c.second[1])+" / "+std::to_string(c.second[2])+" / "+std::to_string(c.second[3])+" / "+std::to_string(c.second[4]));rows+=menuRow(reasons[c.first]);}
 return menuPage(u8"实体形变 · 状态",rows+menuRow(u8"无Studio模型的地图刷子、光束等不支持此骨骼形变")+menuRow(u8"第一人称手与枪一同缩放，不改服务器碰撞盒"));
}
inline bool action(std::uintptr_t id){if(id>=0x710000&&id<0x720000){auto i=choices.find(id);if(i!=choices.end()){editKey=i->second.first;editLabel=i->second.second;}return true;}
 if(id<0x700001||id>0x700032)return false;
 notice=u8"实体形变设置未保存";
 auto f=std::atomic_load(&source_entities::snapshot);aimed.validate(f->epoch);
 if(id==0x700005){notice=u8"对准目标后选择手动识别";return true;}
 if(id==0x700030){aimed.clear();auto c=std::atomic_load(&source_camera::snapshot);auto aim=identifyAim(*f,*c,GetTickCount64());for(const auto& r:f->records)if(aim.classId>=0&&r.handle==aim.handle&&r.classId==aim.classId&&r.className==aim.name){aimed.remember(r,f->epoch);break;}notice=aimed.valid?u8"已识别，选择类别开关或调整参数":u8"未识别到目标，请对准实体后重试";return true;}
 if(id==0x700032){aimed.clear();notice=u8"已清除识别结果";return true;}
 if(id==0x700006){if(aimed.valid){editKey=aimed.key;editLabel=aimed.label;}notice=u8"先选参数，再按 - / = 调整";return true;}
 auto s=std::make_shared<Settings>(*std::atomic_load(&options));
 if(id==0x700031){if(aimed.valid){auto r=selectedAimRule(*s);r.enabled=!r.enabled;s->rules[aimed.key]=r;}else {notice=u8"请先手动识别目标";return true;}}
 else if(id==0x700001)s->enabled=!s->enabled;else if(id==0x700002)s->studioOnly=!s->studioOnly;else if(id==0x700003){s->all.enabled=false;for(auto& r:s->rules)r.second.enabled=false;}else if(id==0x700004){editKey.clear();editLabel=u8"所有实体默认值";}

 else if(id>=0x700020&&id<=0x700023)editField=unsigned(id-0x700020);else if(id==0x700011)step=step<.02f?.05f:step<.09f?.1f:.01f;else if(id==0x700013){if(!editKey.empty())s->rules.erase(editKey);else s->all=Rule{};}else {auto r=editRule(*s);if(id==0x700010)r.enabled=!r.enabled;if(id==0x700012)r=Rule{};if(editKey.empty())s->all=r;else s->rules[editKey]=r;}
 store(s);return true;
}
inline bool adjust(std::uintptr_t page,int delta){if(!editPage(page)||(page==0x700006&&!aimed.valid))return false;if(!delta)return true;auto s=std::make_shared<Settings>(*std::atomic_load(&options));auto r=editRule(*s);float* values[]={&r.size,&r.x,&r.y,&r.z};*values[editField]=std::round((*values[editField]+step*(delta>0?1:-1))*100)/100;normalize(r);if(editKey.empty())s->all=r;else s->rules[editKey]=r;store(s);return true;}
}
