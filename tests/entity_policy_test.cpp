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
 inventory::Frame frame;frame.epoch=7;frame.tick=1000;
 auto tank=record(0x9001,260,"Tank",{0,0,500});tank.health=100;tank.lifeState=0;frame.records={tank};
 auto camera=cameraAt(0);settings=ProximitySettings{};
 check(!evaluateProximity(frame,camera,settings,1100).active);settings.enabled=true;
 auto near=evaluateProximity(frame,camera,settings,1100);check(near.active&&near.nearest==500);
 frame.records[0].networkOrigin=Vec3{0,0,501};check(!evaluateProximity(frame,camera,settings,1100).active);
 check(evaluateProximity(frame,cameraAt(10),settings,1100).active);
 frame.records[0].health=0;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].health=100;
 frame.records[0].lifeState=1;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].lifeState=0;
 frame.records[0].dormant=true;check(!evaluateProximity(frame,camera,settings,1100).active);frame.records[0].dormant=false;
 check(!evaluateProximity(frame,camera,settings,1251).active&&!evaluateProximity(frame,camera,settings,999).active);
 frame.epoch=8;check(!evaluateProximity(frame,camera,settings,1100).active);frame.epoch=7;
 settings.triggerClassId=270;check(!evaluateProximity(frame,camera,settings,1100).active);
 settings.triggerClassId=260;settings.radius=600;check(evaluateProximity(frame,camera,settings,1100).active);
 settings.triggerClassId=30;auto triggerProp=record(0x9010,30,"CPhysicsProp",{0,0,100});triggerProp.health=0;
 frame.records={triggerProp};check(evaluateProximity(frame,camera,settings,1100).active);
 // Temporary range rules cover all modeled classes while protecting held items.
 auto actor=record(0x9002,1,"CTerrorPlayer",{0,0,100});actor.activeWeapon=0x9003;actor.viewModel=0x9004;
 auto heldGun=record(0x9003,10,"CPistol",{0,0,100});heldGun.owner=actor.handle;
 auto view=record(0x9004,20,"CTerrorViewModel",{0,0,10});
 auto groundGun=record(0x9005,10,"CPistol",{0,0,200});
 auto prop=record(0x9006,30,"CPhysicsProp",{0,0,200});
 auto common=record(0x9007,40,"Infected",{0,0,200});
 auto attachment=record(0x9008,50,"CDynamicProp",{0,0,0});attachment.parent=view.handle;
 auto clothing=record(0x9009,50,"CDynamicProp",{0,0,0});clothing.parent=common.handle;
 frame.records={tank,actor,heldGun,view,groundGun,prop,common,attachment,clothing};
 selection.setAll(false);auto excluded=experimentExcludedHandles(frame,false);
 for(const auto* r:{&tank,&actor,&groundGun,&prop})check(selectedForEffect(*r,selection,settings,true,excluded));
 for(const auto* r:{&heldGun,&view,&common,&attachment,&clothing})check(!selectedForEffect(*r,selection,settings,true,excluded));
 settings.includeCommon=true;excluded=experimentExcludedHandles(frame,true);check(selectedForEffect(common,selection,settings,true,excluded));
 check(!selectedForEffect(heldGun,selection,settings,true,excluded));
 check(!selectedForEffect(prop,selection,settings,false,excluded));selection.classes[prop.classId]=true;
 check(selectedForEffect(prop,selection,settings,false,excluded)); // Leaving the radius restores manual rules.
 check(!selection.selected(tank.classId,tank.className)); // Experiment never writes category configuration.
 EntityMenuState menus;menus.observe(frame,selection);
 auto root=hud::document(menus.root());check(root.rows.size()==8&&hasAction(root,121)&&!hasAction(root,120)&&hasAction(root,104));
 for(unsigned group=0;group<EntityGroupCount;++group){check(hasAction(root,1000+group));auto doc=hud::document(menus.group(EntityGroup(group),selection));check(hasAction(doc,130+group*2)&&hasAction(doc,131+group*2));}
 auto recognition=hud::document(menus.recognition(selection));check(hasAction(recognition,122)&&!hasAction(recognition,124)&&menus.aimed.classId==-1);
 menus.aimed={260,tank.handle,"Tank",0};recognition=hud::document(menus.recognition(selection));
 check(hasAction(recognition,122)&&hasAction(recognition,124)&&hasAction(recognition,125)&&hasAction(recognition,126)&&hasAction(recognition,191));
 for(auto id:{104u,108u,109u,1000u,1004u})check(hud::opensChild(id));
 for(auto id:{121u,122u,124u,125u,130u,139u,180u,191u,0x10000u,0x20000u})check(!hud::opensChild(id));
 check(hud::KeyGate::Count==14); // Arrow keys no longer participate in menu pagination.
 std::cout<<"PASS: categorized rules and bulk actions, explicit identification menus, +/- pagination, proximity boundaries/death/maps, held-item and common exclusions, config migration and manual restoration\n";
}
