#include "entity_menu.hpp"
#include "native_menu_model.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("special type regression");}
int main(){using namespace visual;using namespace dango;
 EntitySelection selection;ProximitySettings manual;ProximityResult none;
 inventory::Record npc; npc.classId=20;npc.className="Tank";npc.team=3;
 selection.specials[specialIndex("Tank")]=true;check(selection.anyEnabled()&&selection.selectedRecord(npc));
 for(unsigned role=0;role<SpecialCount;++role){npc.className=SpecialNames[role];check(specialCharacter(npc)==int(role));check(selection.selectedRecord(npc)==(int(role)==specialIndex("Tank")));}
 inventory::Record player;player.classId=100;player.className="CTerrorPlayer";player.team=3;player.character=5;
 for(int zombie=1;zombie<=8;++zombie){player.zombieClass=zombie;check(selection.selectedRecord(player)==(zombie==8));}
 player.team=2;player.character=5;player.zombieClass=8;check(!selection.selectedRecord(player));
 player.team=3;player.zombieClass.reset();check(!selection.selectedRecord(player));
 player.zombieClass=0;check(!selection.selectedRecord(player));player.zombieClass=9;check(!selection.selectedRecord(player));
 selection.setGroup(EntityGroup::Special,true);for(int zombie=1;zombie<=8;++zombie){player.zombieClass=zombie;check(selection.selectedRecord(player));}
 selection.specials[specialIndex("Boomer")]=false;player.zombieClass=2;check(!selection.selectedRecord(player));
 player.zombieClass=8;check(selection.selectedRecord(player));
 nlohmann::json j;writeEntitySelection(j,selection);check(j["entitySelection"]["specials"]["Boomer"]==false);
 auto restored=readEntitySelection(nlohmann::json::parse(j.dump()));player.zombieClass=2;check(!restored.selectedRecord(player));
 player.zombieClass=8;check(restored.selectedRecord(player));restored.setGroup(EntityGroup::Special,false);check(!restored.selectedRecord(player));
 restored.setAll(false);check(restored.specials.empty()&&!restored.anyEnabled());
 EntityMenuState menu;auto doc=hud::document(menu.group(EntityGroup::Special,selection));check(doc.rows.size()==11);
 for(unsigned role=0;role<SpecialCount;++role){unsigned found=0;for(const auto& row:doc.rows)if(row.label.find(std::string(SpecialNames[role])+u8"：")==0){++found;check(row.action==0x60000+role);}check(found==1);}
 for(const auto& row:doc.rows)check(row.label.find("NextBotCombatCharacter")==std::string::npos&&row.label.find("CTerrorPlayer")==std::string::npos);
 // The existing Tank-centered experiment recognizes a player-controlled Tank too.
 inventory::Frame frame;frame.epoch=7;frame.tick=1000;player.handle=0x1001;player.modelIndex=5;player.networkOrigin=Vec3{0,0,100};
 player.networkAngles=Vec3{};player.parent=0xffffffffu;player.parentAttachment=0;player.dormant=false;player.health=100;player.lifeState=0;
 auto prop=player;prop.handle=0x1002;prop.classId=200;prop.className="CPhysicsProp";prop.zombieClass.reset();prop.networkOrigin=Vec3{0,0,200};
 frame.records={player,prop};GameCameraFrame camera;camera.epoch=7;camera.active=1;camera.tick=1000;
 ProximitySettings proximity;proximity.enabled=true;auto targets=evaluateProximity(frame,camera,proximity,1100);
 check(targets.sources==1&&targets.inRange.count(prop.handle));
 frame.records[0].zombieClass=1;check(!evaluateProximity(frame,camera,proximity,1100).active);
 std::cout<<"PASS: eight actual infected types, NPC/player shared-class matching, survivor exclusion, unknown type rejection, independent/bulk saved switches and player-Tank range centers\n";
}
