#pragma once
#include "tank_cvars.hpp"
#include "diagnostics.hpp"
#include "tank_sequence.hpp"
#include "entity_menu.hpp"
#include <mutex>
namespace visual::tank {
inline std::atomic<bool> faulted{false};
inline std::shared_ptr<const Result> emptyResult=std::make_shared<const Result>();
inline std::shared_ptr<const Settings> options=std::make_shared<const Settings>();
inline std::shared_ptr<const Result> snapshot=std::make_shared<const Result>();
struct Animation{std::uint32_t epoch=0,handle=0;int sequence=-1;SequenceName names;};
inline std::mutex animationMutex;inline std::map<std::uint32_t,Animation> animations;
// CPU metadata only, called from the already verified model callback. No D3D,
// SetupBones, simulation changes or additional engine method dispatch.
inline void observeModel(std::uint32_t header,int index){
 if(faulted||!std::atomic_load(&options)->enabled)return;auto frame=std::atomic_load(&source_entities::combatSnapshot);const Record* record=nullptr;
 for(const auto& r:frame->records)if(r.index==unsigned(index)&&tank(r)){record=&r;break;}if(!record||!record->sequence)return;
 auto model=dango::entitywire::text(source_entities::reader,header+12,64);for(char& c:model)if(c>='A'&&c<='Z')c=char(c+32);if(model.find("hulk")==std::string::npos&&model.find("tank")==std::string::npos)return;
 std::lock_guard<std::mutex> lock(animationMutex);auto old=animations.find(record->handle);if(old!=animations.end()&&old->second.epoch==frame->epoch&&old->second.sequence==*record->sequence)return;
 if(animations.size()>16)animations.clear();auto name=sequenceName(source_entities::reader,header,*record->sequence);
 animations[record->handle]={frame->epoch,record->handle,*record->sequence,name?*name:SequenceName{}};
}
inline void onFrame(const dango::GameCameraFrame& camera)try{
 if(faulted)return;
 static Predictor predictor;static std::shared_ptr<const Frame> previous;
 auto cfg=std::atomic_load(&options);auto frame=std::atomic_load(&source_entities::combatSnapshot);
 if(!cfg->enabled||!camera.active){predictor.reset();previous.reset();if(std::atomic_load(&snapshot)!=emptyResult)std::atomic_store(&snapshot,emptyResult);return;}
 if(previous==frame||frame->epoch!=camera.epoch||camera.tick<frame->tick||camera.tick-frame->tick>250)return;previous=frame;
 auto out=predictor.update(*frame,*std::atomic_load(&parameters),*cfg);
 {std::lock_guard<std::mutex> lock(animationMutex);for(auto& o:out.tanks){auto a=animations.find(o.handle);if(a!=animations.end()&&a->second.epoch==frame->epoch&&o.sequence&&a->second.sequence==*o.sequence&&!a->second.names.label.empty())o.action=classify(a->second.names)+": "+a->second.names.label;}}
 std::atomic_store(&snapshot,std::make_shared<const Result>(std::move(out)));
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
inline std::string menu(){auto cfg=std::atomic_load(&options);auto f=std::atomic_load(&snapshot);auto p=std::atomic_load(&parameters);
 auto number=[](std::optional<float> n){return n?std::to_string(int(*n*10)/10.f):std::string(u8"未读取");};
 std::string rows=menuItem(cfg->enabled?u8"Tank 预测：开":u8"Tank 预测：关（默认）",330)+menuItem(cfg->draw?u8"方向与范围绘制：开":u8"方向与范围绘制：关",331)+menuItem(cfg->logging?u8"诊断记录：开":u8"诊断记录：关",332)+menuItem(u8"预测时间："+std::to_string(cfg->horizonMs)+u8"毫秒（切换）",333)+menuRow(u8"手长参数 "+number(p->range)+u8" / 弧度参数 "+number(p->arc))+menuRow(p->source);
 for(const auto& o:f->tanks){rows+=menuRow("Tank #"+std::to_string(o.handle&4095)+" / "+(o.eye?u8"瞄准角":u8"身体角回退")+" / "+o.action);
  if(o.target)rows+=menuRow(std::string(o.target->source=="network-enemy"?u8"网络敌对目标：":o.target->source=="network-look-at"?u8"注视目标（非锁敌保证）：":u8"朝向推测目标：")+o.target->name);else rows+=menuRow(u8"当前目标未知或存在歧义");
  if(o.gaze&&o.target&&o.gaze->handle!=o.target->handle)rows+=menuRow(u8"另一个注视目标："+o.gaze->name);
  if(o.cooldown)rows+=menuRow(u8"可读计时差（估计） "+std::to_string(*o.cooldown)+u8"秒");
  if(o.headingStalled)rows+=menuRow(u8"朝向停滞：可能锁向，尚未证实");rows+=menuRow(o.reason);
 }
 if(faulted)rows+=menuRow(u8"预测处理失败，已暂停；关闭后重开以重试");
 return menuPage(u8"Tank 行为预测 · L4N 客户端实验",rows+menuItem(u8"保存设置",101)+menuRow(u8"仅预测；不是服务器精确命中边界"));
}
inline bool action(std::uintptr_t id){if(id<330||id>333)return false;auto s=*std::atomic_load(&options);if(id==330){source_entities::resetRequested=true;s.enabled=!s.enabled;if(s.enabled)faulted=false;}if(id==331)s.draw=!s.draw;if(id==332)s.logging=!s.logging;if(id==333)s.horizonMs=s.horizonMs>=500?0:s.horizonMs+100;std::atomic_store(&options,std::make_shared<const Settings>(s));return true;}
inline void poll(std::uint64_t now)try{if(faulted)return;auto cfg=std::atomic_load(&options);pollCvars(now,cfg->enabled);static std::uint64_t next=0,last=0;
 if(!cfg->enabled||!cfg->logging||now<next)return;next=now+100;auto f=std::atomic_load(&snapshot);if(!f->tick||f->tick==last)return;last=f->tick;
 nlohmann::json j={{"schema",1},{"epoch",f->epoch},{"tick",f->tick},{"parameterSource",f->parameters.source},{"tanks",nlohmann::json::array()}};
 if(f->parameters.range)j["range"]=*f->parameters.range;if(f->parameters.arc)j["arc"]=*f->parameters.arc;
 for(const auto& o:f->tanks){nlohmann::json r={{"handle",o.handle},{"position",{o.origin.x,o.origin.y,o.origin.z}},{"yaw",o.heading},{"eyeAngle",o.eye},{"headingRate",o.headingRate},{"possibleHeadingLock",o.headingStalled},{"action",o.action},{"geometry",o.geometry},{"reason",o.reason}};
  if(o.cooldown)r["estimatedCooldown"]=*o.cooldown;if(o.sequence)r["sequence"]=*o.sequence;if(o.cycle)r["cycle"]=*o.cycle;if(o.target)r["target"]={{"handle",o.target->handle},{"name",o.target->name},{"source",o.target->source}};j["tanks"].push_back(r);
 }
 auto actors=std::atomic_load(&source_entities::combatSnapshot);j["actors"]=nlohmann::json::array();
 if(actors->epoch==f->epoch)for(const auto& a:actors->records){nlohmann::json r={{"handle",a.handle},{"class",a.className}};
  auto vector=[&](const char* key,const std::optional<Vec3>& v){if(v)r[key]={v->x,v->y,v->z};};vector("position",a.networkOrigin);vector("bodyAngles",a.networkAngles);vector("eyeAngles",a.eyeAngles);vector("collisionMins",a.collisionMins);vector("collisionMaxs",a.collisionMaxs);
  if(a.health)r["health"]=*a.health;if(a.lookAtHandle)r["lookAtHandle"]=*a.lookAtHandle;if(a.enemyHandle)r["enemyHandle"]=*a.enemyHandle;if(a.activeWeapon)r["activeWeapon"]=*a.activeWeapon;if(a.nextPrimaryAttack)r["nextPrimaryAttack"]=*a.nextPrimaryAttack;if(a.nextAttack)r["nextAttack"]=*a.nextAttack;if(a.simulationTime)r["simulationTime"]=*a.simulationTime;j["actors"].push_back(r);
 }
 auto path=visual::diagnostics::path.parent_path()/L"tank-prediction.jsonl";std::error_code e;auto size=std::filesystem::file_size(path,e);if(!e&&size>=4u*1024u*1024u){auto old=path;old.replace_filename(L"tank-prediction.previous.jsonl");if(!MoveFileExW(path.c_str(),old.c_str(),MOVEFILE_REPLACE_EXISTING))return;}
 std::ofstream file(path,std::ios::app|std::ios::binary);file<<j.dump(-1,' ',false,nlohmann::json::error_handler_t::replace)<<'\n';
}
catch(...){faulted=true;std::atomic_store(&snapshot,emptyResult);}
}
