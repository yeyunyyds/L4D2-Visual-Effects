#include "entity_menu.hpp"
#include "native_menu_model.hpp"
#include "source_entity_contract.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
void check(bool value){if(!value)throw std::runtime_error("survivor role regression");}
int main(){using namespace visual;using namespace dango;
 EntitySelection selection;ProximitySettings manual;ProximityResult empty;
 inventory::Record actor;actor.classId=100;actor.className="CTerrorPlayer";actor.team=2;
 for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(survivorCharacter(actor)==int(role));check(!selection.selectedRecord(actor));}
 check(survivorIndex("Bill")==4&&survivorIndex("Zoey")==5&&survivorIndex("Nick")==0);
 selection.survivors[5]=true;check(selection.anyEnabled());
 for(auto name:{"CTerrorPlayer","SurvivorBot","NextBotCombatCharacter"}){
  actor.className=name;
  for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(selectedForEffect(actor,selection,manual,empty)==(role==5));}
 }
 actor.className="CTerrorPlayer";actor.team=3;actor.character=5;check(!selection.selectedRecord(actor));
 actor.team=2;actor.character.reset();check(!selection.selectedRecord(actor));actor.character=8;check(!selection.selectedRecord(actor));
 actor.character=-1;check(!selection.selectedRecord(actor));
 selection.setGroup(EntityGroup::Survivors,true);
 for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(selection.selectedRecord(actor));}
 selection.survivors[4]=false;
 for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(selection.selectedRecord(actor)==(role!=4));}
 nlohmann::json j={{"font",27}};writeEntitySelection(j,selection);
 check(j["entitySelection"]["survivors"]["Bill"]==false&&j["entitySelection"]["survivors"]["Zoey"]==true);
 auto restored=readEntitySelection(nlohmann::json::parse(j.dump()));
 for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(restored.selectedRecord(actor)==(role!=4));}
 check(j["font"]==27);
 // Old generic class rules survive; new character switches have higher priority only for survivors.
 auto old=readEntitySelection(nlohmann::json{{"entitySelection",{{"all",true}}}});
 actor.character=5;check(old.selectedRecord(actor));old.survivors[5]=false;check(!old.selectedRecord(actor));
 actor.team=3;actor.zombieClass=1;check(old.selectedRecord(actor));actor.team.reset();check(!old.selectedRecord(actor));actor.team=2;
 old.setGroup(EntityGroup::Survivors,false);for(unsigned role=0;role<SurvivorCount;++role){actor.character=int(role);check(!old.selectedRecord(actor));}
 actor.team=3;actor.zombieClass=1;check(old.selectedRecord(actor));old.setAll(false);check(old.survivors.empty()&&!old.anyEnabled());actor.team=2;
 EntityMenuState menu;auto page=hud::document(menu.group(EntityGroup::Survivors,selection));check(page.rows.size()==11);
 for(unsigned role=0;role<SurvivorCount;++role){unsigned found=0;for(const auto& row:page.rows)if(row.label.find(std::string(SurvivorNames[role])+u8"：")==0){++found;check(row.action==0x50000+role);}check(found==1);}
 for(const auto& row:page.rows)check(row.label.find("CTerrorPlayer")==std::string::npos&&row.label.find("SurvivorBot")==std::string::npos);
 inventory::Frame scene;scene.records={actor};menu.observe(scene,selection);check(hud::document(menu.group(EntityGroup::Survivors,selection)).rows.size()==11);
 auto root=hud::document(menu.root());bool count=false;for(const auto& row:root.rows)if(row.label==u8"求生者 (8)")count=true;check(count);
 // Exercise the real RecvTable allowlist: merely requesting the field in the producer is insufficient.
 struct Reader{
  unsigned char data[1024]{};
  bool read(std::uint32_t address,void* out,std::size_t count){if(address<0x1000||std::uint64_t(address-0x1000)+count>sizeof(data))return false;std::memcpy(out,data+address-0x1000,count);return true;}
 } reader;
 entitywire::Table table{};table.props=0x1100;table.count=2;table.name=0x1240;
 entitywire::Prop prop{};prop.name=0x1200;prop.type=0;prop.offset=0x120;
 std::memcpy(reader.data,&table,sizeof(table));std::memcpy(reader.data+0x100,&prop,sizeof(prop));
 prop.name=0x1280;prop.offset=0x124;std::memcpy(reader.data+0x100+sizeof(prop),&prop,sizeof(prop));
 std::strcpy(reinterpret_cast<char*>(reader.data+0x280),"m_zombieClass");
 std::strcpy(reinterpret_cast<char*>(reader.data+0x200),"m_survivorCharacter");std::strcpy(reinterpret_cast<char*>(reader.data+0x240),"DT_Survivor");
 entitywire::Fields fields;check(entitywire::fields(reader,0x1000,fields));check(fields.at("m_survivorCharacter").offset==0x120);check(fields.at("m_zombieClass").offset==0x124);
 std::cout<<"PASS: eight fixed survivor roles, shared-Class-ID separation, humans/bots/team gates, independent switches and bulk operations, legacy/save compatibility, real character field extraction\n";
}
