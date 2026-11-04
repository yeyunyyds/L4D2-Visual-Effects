#include "entity_menu.hpp"
#include "native_menu_model.hpp"
#include "hud_menu_policy.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
using namespace visual;
using namespace dango;
void check(bool condition){if(!condition)throw std::runtime_error("entity policy regression");}
inventory::Record record(std::uint32_t handle,int id,const char* name,Vec3 at){
 inventory::Record r;r.handle=handle;r.index=handle&4095;r.serial=handle>>12;r.classId=id;r.className=name;r.modelIndex=10;
 r.networkOrigin=at;r.networkAngles=Vec3{};r.parent=0xffffffffu;r.parentAttachment=0;r.dormant=false;return r;
}
GameCameraFrame cameraAt(float z){
 GameCameraFrame c;c.active=1;c.epoch=7;c.tick=1000;c.width=1920;c.height=1080;
 c.worldToView={1,0,0,0,0,1,0,0,0,0,1,-z,0,0,0,1};
 c.worldToClip={1,0,0,0,0,1,0,0,0,0,1.0001f,-1.0001f-z*1.0001f,0,0,1,-z};return c;
}
bool hasAction(const hud::Document& doc,std::uintptr_t action){for(const auto& r:doc.rows)if(r.action==action)return true;return false;}
int main(){
 for(auto name:{"Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Tank","Witch"})check(groupForName(name)==EntityGroup::Special);
 for(auto name:{"CTerrorPlayer","SurvivorBot"})check(groupForName(name)==EntityGroup::Survivors);
 for(auto name:{"CPistol","CGnome","CGasCan","CWeaponSpawn","CFirstAidKit"})check(groupForName(name)==EntityGroup::Pickups);
 for(auto name:{"CTerrorViewModel","CBaseViewModel","CBaseWeaponWorldModel"})check(groupForName(name)==EntityGroup::Hands);
 for(auto name:{"Infected","CPhysicsProp","CustomClass"})check(groupForName(name)==EntityGroup::Other);
 EntitySelection selection;selection.names={{260,"Tank"},{270,"Witch"},{10,"CPistol"}};
 selection.classes[260]=false;selection.classes[10]=true;
 selection.setGroup(EntityGroup::Special,true);
 check(selection.selected(260,"Tank")&&selection.selected(270,"Witch")&&selection.selected(271,"Boomer"));
 check(selection.selected(10,"CPistol")&&!selection.selected(11,"CGnome"));
 selection.classes[260]=false;check(!selection.selected(260,"Tank")&&selection.selected(270,"Witch"));
 selection.setGroup(EntityGroup::Special,false);check(!selection.selected(260,"Tank")&&!selection.selected(270,"Witch")&&selection.selected(10,"CPistol"));
 nlohmann::json j={{"font",27}};writeEntitySelection(j,selection);
 auto restored=readEntitySelection(nlohmann::json::parse(j.dump()));check(!restored.selected(270,"Witch")&&restored.selected(10,"CPistol")&&j["font"]==27);
 restored.setAll(false);check(!restored.anyEnabled()&&restored.groups.empty()&&restored.classes.empty());
 auto old=readEntitySelection(nlohmann::json{{"entitySelection",{{"all",true},{"classes",{{"260",false}}}}}});
 check(old.selected(10,"CPistol")&&!old.selected(260,"Tank"));
 ProximitySettings settings;check(!settings.enabled&&!settings.includeCommon&&settings.radius==500&&settings.triggerName=="Tank"&&settings.triggerClassId==-1);
 auto defaults=readProximity(nlohmann::json::object());check(!defaults.enabled&&!defaults.includeCommon);
 settings.enabled=true;settings.radius=700;settings.triggerClassId=270;settings.triggerName="Witch";
 writeProximity(j,settings);auto loaded=readProximity(nlohmann::json::parse(j.dump()));
 check(loaded.enabled&&loaded.radius==700&&loaded.triggerClassId==270&&loaded.triggerName=="Witch"&&!loaded.includeCommon);
 auto invalid=readProximity(nlohmann::json{{"proximityStretch",{{"enabled","yes"},{"includeCommon",1},{"radius",-1},{"triggerClassId",5000}}}});
 check(!invalid.enabled&&!invalid.includeCommon&&invalid.radius==100&&invalid.triggerClassId==-1);
 invalid=readProximity(nlohmann::json{{"proximityStretch",{{"radius",999999}}}});check(invalid.radius==5000);
 invalid=readProximity(nlohmann::json{{"proximityStretch",{{"radius","big"}}}});check(invalid.radius==500);
 // A fixed menu must work before any entity/class catalog has been observed.
 EntityMenuState coldMenu;EntitySelection futureRules;inventory::Frame emptyFrame;
 coldMenu.observe(emptyFrame,futureRules);
 auto coldSpecial=hud::document(coldMenu.group(EntityGroup::Special,futureRules));
 for(auto name:{"Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Tank","Witch"}){
  bool found=false;for(const auto& row:coldSpecial.rows)if(row.label.find(name)==0)found=true;check(found);
 }
 for(auto name:{"CPistol","CGnome","CTerrorViewModel","Infected"}){
  auto page=hud::document(coldMenu.group(groupForName(name),futureRules));bool found=false;
  for(const auto& row:page.rows){if(row.label.find(name)==0)found=true;}
  check(found);
 }
 futureRules.setType("Tank",true);check(futureRules.anyEnabled());
 writeEntitySelection(j,futureRules);futureRules=readEntitySelection(nlohmann::json::parse(j.dump()));
 check(futureRules.selected(260,"Tank")&&!futureRules.selected(270,"Witch"));
 futureRules.names[260]="Tank";futureRules.classes[260]=false;futureRules.setType("Tank",true);check(futureRules.selected(260,"Tank"));
 futureRules.setClass(260,"Tank",false);check(!futureRules.selected(260,"Tank"));
 futureRules.setType("Tank",true);futureRules.setGroup(EntityGroup::Special,false);check(!futureRules.selected(260,"Tank"));
 futureRules.setAll(false);check(futureRules.types.empty()&&!futureRules.anyEnabled());
 inventory::Frame frame;frame.epoch=7;frame.tick=1000;
 auto tank=record(0x9001,260,"Tank",{0,0,500});tank.health=100;tank.lifeState=0;
 auto inside=record(0x9010,30,"CPhysicsProp",{0,0,900});
 auto boundary=record(0x9011,30,"CPhysicsProp",{0,0,1000});
 auto outside=record(0x9012,30,"CPhysicsProp",{0,0,1000.1f});
 frame.records={tank,inside,boundary,outside};auto camera=cameraAt(0);settings=ProximitySettings{};
 check(!evaluateProximity(frame,camera,settings,1100).active);settings.enabled=true;
 auto targets=evaluateProximity(frame,camera,settings,1100);
 check(targets.sources==1&&targets.active&&targets.inRange.count(tank.handle)&&targets.inRange.count(inside.handle)&&targets.inRange.count(boundary.handle)&&!targets.inRange.count(outside.handle));
 // Moving the viewer near/far cannot change which entities are inside Tank's radius.
 check(evaluateProximity(frame,cameraAt(500),settings,1100).inRange==targets.inRange);
 check(evaluateProximity(frame,cameraAt(10000),settings,1100).inRange==targets.inRange);
 // Even an explicitly enabled class must remain off OUTSIDE the experiment's radius.
 selection.setAll(true);check(!selectedForEffect(outside,selection,settings,targets));
 check(selectedForEffect(inside,selection,settings,targets));
 frame.records[0].health=0;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].health=100;
 frame.records[0].lifeState=1;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].lifeState=0;
 frame.records[0].dormant=true;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].dormant=false;
 frame.records[0].effects=32;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].effects.reset();
 check(!evaluateProximity(frame,camera,settings,1251).active&&!evaluateProximity(frame,camera,settings,999).active);
 frame.epoch=8;check(!evaluateProximity(frame,camera,settings,1100).active);frame.epoch=7;
 settings.triggerClassId=270;check(!evaluateProximity(frame,camera,settings,1100).active);settings.triggerClassId=-1;
 auto secondTank=record(0x9013,260,"Tank",{0,0,1400});frame.records.push_back(secondTank);
 targets=evaluateProximity(frame,camera,settings,1100);check(targets.sources==2&&targets.inRange.count(outside.handle));
 frame.records.erase(frame.records.begin()); // Removing the first center drops entities outside all remaining radii.
 frame.records[0].networkOrigin=Vec3{0,0,800};targets=evaluateProximity(frame,camera,settings,1100);check(!targets.inRange.count(inside.handle));
 frame.records={inside};check(!evaluateProximity(frame,camera,settings,1100).active);
 settings.triggerClassId=30;inside.health=0;frame.records={inside};check(evaluateProximity(frame,camera,settings,1100).active);
 settings=ProximitySettings{};settings.enabled=true;
 auto actor=record(0x9002,1,"CTerrorPlayer",{0,0,600});actor.activeWeapon=0x9003;actor.viewModel=0x9004;
 auto heldGun=record(0x9003,10,"CPistol",{0,0,600});heldGun.owner=actor.handle;
 auto view=record(0x9004,20,"CTerrorViewModel",{0,0,600});
 auto groundGun=record(0x9005,10,"CPistol",{0,0,700});
 auto farGun=record(0x9015,10,"CPistol",{0,0,1600});
 auto prop=record(0x9006,30,"CPhysicsProp",{0,0,900});
 auto common=record(0x9007,40,"Infected",{0,0,800});
 auto attachment=record(0x9008,50,"CDynamicProp",{0,0,0});attachment.parent=view.handle;
 auto clothing=record(0x9009,50,"CDynamicProp",{0,0,0});clothing.parent=common.handle;
 frame.records={tank,actor,heldGun,view,groundGun,farGun,prop,common,attachment,clothing};
 selection.setAll(false);targets=evaluateProximity(frame,cameraAt(10000),settings,1100);
 for(const auto* r:{&tank,&actor,&groundGun,&prop})check(selectedForEffect(*r,selection,settings,targets));
 for(const auto* r:{&heldGun,&view,&farGun,&common,&attachment,&clothing})check(!selectedForEffect(*r,selection,settings,targets));
 settings.includeCommon=true;targets=evaluateProximity(frame,camera,settings,1100);
 check(selectedForEffect(common,selection,settings,targets)&&selectedForEffect(clothing,selection,settings,targets));
 check(!selectedForEffect(heldGun,selection,settings,targets)&&!selectedForEffect(farGun,selection,settings,targets));
 settings.enabled=false;check(!selectedForEffect(prop,selection,settings,targets));selection.classes[prop.classId]=true;
 check(selectedForEffect(prop,selection,settings,targets)); // Only disabling the experiment restores manual rules.
 check(!selection.selected(tank.classId,tank.className));
 EntityMenuState menus;menus.observe(frame,selection);
 auto root=hud::document(menus.root());check(root.rows.size()==8&&hasAction(root,121)&&!hasAction(root,120)&&hasAction(root,104));
 for(unsigned group=0;group<EntityGroupCount;++group){check(hasAction(root,1000+group));auto doc=hud::document(menus.group(EntityGroup(group),selection));check(hasAction(doc,130+group*2)&&hasAction(doc,131+group*2));}
 auto recognition=hud::document(menus.recognition(selection));check(hasAction(recognition,122)&&!hasAction(recognition,124)&&menus.aimed.classId==-1);
 menus.aimed={260,tank.handle,"Tank",0};recognition=hud::document(menus.recognition(selection));
 check(hasAction(recognition,122)&&hasAction(recognition,124)&&hasAction(recognition,125)&&hasAction(recognition,126)&&hasAction(recognition,191));
 for(auto id:{104u,108u,109u,1000u,1004u})check(hud::opensChild(id));
 for(auto id:{121u,122u,124u,125u,130u,139u,180u,191u,0x10000u,0x20000u,0x30000u,0x40000u})check(!hud::opensChild(id));
 check(hud::KeyGate::Count==14); // Arrow keys no longer participate in menu pagination.
 std::cout<<"PASS: categorized rules and bulk actions, explicit identification menus, +/- pagination, static type lists without live entities, entity-to-Tank distances independent of camera, same-class inside/outside separation, multiple sources/death/maps, exclusions and config migration\n";
}
