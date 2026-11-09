#include "aim_selection.hpp"
#include "entity_transform.hpp"
#include "entity_menu.hpp"
#include "json.hpp"
#include "dango/source_entity_contract.hpp"
#include "dango/hud_menu_policy.hpp"
#include <iostream>
#include <stdexcept>
using namespace visual;using namespace dango;
void check(bool b){if(!b)throw std::runtime_error("Aim body/ownership regression");}
inventory::Record record(unsigned index,int id,const char* name,Vec3 origin){
 inventory::Record r;r.index=index;r.serial=1;r.handle=4096+index;r.classId=id;r.className=name;r.modelIndex=2;r.networkOrigin=origin;r.networkAngles=Vec3{};r.parent=0xffffffffu;r.parentAttachment=0;r.dormant=false;r.effects=0;r.health=100;r.lifeState=0;return r;
}
GameCameraFrame camera(Vec3 forward={1,0,0}){
 GameCameraFrame c;c.epoch=7;c.tick=1000;c.active=1;c.width=1920;c.height=1080;forward=normalize(forward);Vec3 up{-forward.z,0,forward.x};float oz=-forward.z*64,oy=-up.z*64;
 c.worldToView={0,-1,0,0,up.x,0,up.z,oy,forward.x,0,forward.z,oz,0,0,0,1};
 c.worldToClip={0,-1,0,0,up.x,0,up.z,oy,forward.x*1.0001f,0,forward.z*1.0001f,oz*1.0001f-1.0001f,forward.x,0,forward.z,oz};return c;
}
struct Memory{std::array<unsigned char,4096> bytes{};bool read(unsigned at,void* out,std::size_t n){if(!at||std::uint64_t(at)+n>bytes.size())return false;std::memcpy(out,bytes.data()+at,n);return true;}template<class T>void put(unsigned at,const T& value){std::memcpy(bytes.data()+at,&value,sizeof(value));}void text(unsigned at,const char* value){std::memcpy(bytes.data()+at,value,std::strlen(value)+1);}};
int main(){
 inventory::Frame f;f.epoch=7;f.tick=1000;auto c=camera();
 auto local=record(1,260,"CTerrorPlayer",{});local.team=2;local.character=0;
 auto tank=record(2,261,"Tank",{100,0,0});tank.collisionMins=Vec3{-32,-32,0};tank.collisionMaxs=Vec3{32,32,110};
 auto claw=record(3,262,"ExampleClawWeapon",{100,0,64});claw.owner=tank.handle;claw.weaponOwner=tank.handle;tank.activeWeapon=claw.handle;
 AimQuery q;q.localHandle=local.handle;f.records={local,tank,claw};auto hits=identifyAimCandidates(f,c,1100,q);
 check(hits.size()==2&&hits.front().handle==tank.handle&&hits.back().handle==claw.handle&&hits.back().attachment);
 q.hitHandle=claw.handle;check(identifyAimCandidates(f,c,1100,q).front().handle==tank.handle);q.hitHandle.reset();
 // All eight SI and all eight survivor roles are selected at chest height.
 for(auto name:SpecialNames){auto body=tank;body.className=name;f.records={local,body};check(identifyAimCandidates(f,c,1100,q).front().handle==body.handle);}
 for(int role=0;role<8;++role){auto body=tank;body.className="CTerrorPlayer";body.team=2;body.character=role;f.records={local,body};hits=identifyAimCandidates(f,c,1100,q);check(!hits.empty()&&aimLabel(hits.front())==SurvivorNames[role]);}
 for(int zombie:{1,2,3,4,5,6,8,7}){auto body=tank;body.className="CTerrorPlayer";body.team=3;body.zombieClass=zombie;f.records={local,body};hits=identifyAimCandidates(f,c,1100,q);check(!hits.empty()&&aimLabel(hits.front())==SpecialNames[zombie==8?6:zombie==7?7:zombie-1]);}
 auto witch=record(2,261,"Witch",{100,0,0});witch.collisionMins=Vec3{-16,-16,0};witch.collisionMaxs=Vec3{16,16,48};f.records={local,witch};hits=identifyAimCandidates(f,camera({100,0,-32}),1100,q);check(!hits.empty()&&hits.front().name=="Witch");
 // Near props, visible body whose origin is below view, and nearest volume.
 auto near=record(4,270,"CPhysicsProp",{20,0,64});f.records={local,near};check(identifyAimCandidates(f,c,1100,q).front().handle==near.handle);
 auto foreground=record(4,270,"CPhysicsProp",{100,10,64});foreground.collisionMins=Vec3{-16,-16,-16};foreground.collisionMaxs=Vec3{16,16,16};auto far=record(5,271,"CGnome",{400,0,64});f.records={local,foreground,far};check(identifyAimCandidates(f,c,1100,q).front().handle==foreground.handle);
 q.blocked=true;check(identifyAimCandidates(f,c,1100,q).empty());q.blocked=false;q.limit=10;check(identifyAimCandidates(f,c,1100,q).empty());q.limit=1200;
 auto rotated=record(15,302,"CDynamicProp",{100,20,64});rotated.networkAngles=Vec3{0,90,0};rotated.collisionMins=Vec3{-40,-2,-5};rotated.collisionMaxs=Vec3{40,2,5};f.records={local,rotated};hits=identifyAimCandidates(f,c,1100,q);check(!hits.empty()&&hits.front().handle==rotated.handle&&!hits.front().approximate);
 // Verified native hit still selects a moved/attached body with no resolvable network pose.
 tank.parent=99999;tank.parentAttachment=1;f.records={local,tank};q.hitHandle=tank.handle;hits=identifyAimCandidates(f,c,1100,q);check(!hits.empty()&&hits.front().handle==tank.handle&&hits.front().approximate);q.hitHandle.reset();tank.parent=0xffffffffu;tank.parentAttachment=0;
 // Native collision identity also handles models whose dormant getter/attachment pose is unavailable.
 auto sparse=witch;sparse.dormant.reset();sparse.networkAngles.reset();sparse.parent.reset();f.records={local,sparse};q.hitHandle=sparse.handle;check(!identifyAimCandidates(f,c,1100,q).empty());q.hitHandle.reset();check(identifyAimCandidates(f,c,1100,q).empty());
 // Invisible and dead actors cannot beat visible live actors.
 claw.effects=32;f.records={local,tank,claw};check(identifyAimCandidates(f,c,1100,q).size()==1);f.records[1].health=0;check(identifyAimCandidates(f,c,1100,q).empty());claw.effects=0;
 // Local hands are explicit, never mistaken for a scene target; attachment pose can be unavailable.
 auto gun=record(6,280,"CPistol",{40,0,64});gun.weaponOwner=local.handle;gun.parent=local.handle;gun.parentAttachment=1;auto view=record(7,281,"CTerrorViewModel",{});view.owner=local.handle;view.dormant.reset();local.activeWeapon=gun.handle;local.viewModel=view.handle;f.records={local,gun,view,tank};hits=identifyAimCandidates(f,c,1100,q);check(hits.front().handle==tank.handle);q.handsOnly=true;hits=identifyAimCandidates(f,c,1100,q);check(hits.size()==2&&hits.front().handle==view.handle);q.handsOnly=false;
 auto world=record(10,284,"CBaseWeaponWorldModel",{40,0,64});world.weaponParent=gun.handle;f.records.push_back(world);hits=identifyAimCandidates(f,c,1100,q);check(hits.front().handle==tank.handle);q.handsOnly=true;check(identifyAimCandidates(f,c,1100,q).size()==3);q.handsOnly=false;
 auto ownerless=claw;ownerless.owner.reset();ownerless.weaponOwner.reset();f.records={local,tank,ownerless};check(aimActorOwner(f,ownerless)==tank.handle&&identifyAimCandidates(f,c,1100,q).front().handle==tank.handle);
 auto duplicate=tank;duplicate.index=11;duplicate.handle=4107;f.records.push_back(duplicate);check(aimActorOwner(f,ownerless)==AmbiguousAimOwner);q.hitHandle=ownerless.handle;check(identifyAimCandidates(f,c,1100,q).empty());q.hitHandle.reset();
 auto gnome=record(8,282,"CGnome",{40,0,64});gnome.owner=local.handle;gnome.weaponOwner=local.handle;f.records={local,gnome};check(aimActorOwner(f,gnome)==0xffffffffu&&identifyAimCandidates(f,c,1100,q).front().handle==gnome.handle);
 // Unknown roles never become v:-1 or a broad CTerrorPlayer override.
 auto unknown=tank;unknown.className="CTerrorPlayer";unknown.team=2;unknown.character.reset();f.records={local,unknown};check(identifyAimCandidates(f,c,1100,q).empty());deform::AimTarget target;target.remember(unknown,7);check(!target.valid&&deform::keys(unknown).empty());
 // The old and new menus address the same explicit role/type setting.
 AimSelection selection;selection.classId=tank.classId;selection.name=tank.className;selection.handle=tank.handle;selection.record=tank;EntitySelection old;old.specials[6]=true;check(old.setAim(selection,false)&&!old.selectedRecord(tank));old.setAim(selection,true);check(old.selectedRecord(tank));
 selection.classId=gun.classId;selection.name=gun.className;selection.record=gun;old.classes[gun.classId]=false;old.setAim(selection,true);check(old.selectedRecord(gun)&&old.types.at("CPistol")&&!old.classes.count(gun.classId));target.remember(gun,7);check(target.key=="t:CPistol");
 nlohmann::json config={{"entityTransform",{{"rules",{{"t:CPistol",{{"enabled",true},{"size",2}}},{"c:280:CPistol",{{"enabled",false},{"size",.5}}},{"v:-1",{{"enabled",true}}}}}}}};
 auto migrated=deform::readSettings(config);check(!migrated.rules.count("c:280:CPistol")&&!migrated.rules.count("v:-1")&&!migrated.rules.at("t:CPistol").enabled&&migrated.rules.at("t:CPistol").size==.5f);migrated.rules["t:CPistol"].enabled=true;check(deform::selected(migrated,gun).enabled);deform::writeSettings(config,migrated);check(!config["entityTransform"]["rules"].contains("c:280:CPistol"));
 auto dynamic=record(9,283,"ModdedProp",{60,0,64});target.remember(dynamic,7);check(target.key=="c:283:ModdedProp");
 // Basic visibility/health metadata stays available with Tank prediction disabled.
 Memory mem;entitywire::Table table;table.props=256;table.count=3;table.name=1024;mem.put(128,table);mem.text(1024,"DT_TestEntity");const char* fields[]={"m_fEffects","m_iHealth","m_lifeState"};for(unsigned i=0;i<3;++i){entitywire::Prop prop;prop.name=1100+i*40;prop.type=0;prop.offset=16+i*4;mem.put(256+i*sizeof(prop),prop);mem.text(prop.name,fields[i]);}entitywire::Fields parsed;check(entitywire::fields(mem,128,parsed,false)&&parsed.size()==3&&parsed.count("m_fEffects")&&parsed.count("m_lifeState"));
 // Late request results, map changes, candidate switching and failure clearing.
 AimSession session;session.begin(1,7,1000);AimResult result;result.id=1;result.epoch=7;result.candidates={selection};check(session.accept(result)&&session.aimed.classId==selection.classId);session.validateEpoch(8);check(session.aimed.classId<0&&!session.pending&&session.candidates.empty());session.begin(2,8,1100);check(!session.accept(result));result.id=2;result.epoch=8;result.candidates.clear();check(session.accept(result)&&session.aimed.classId<0);
 EntityMenuState menu;menu.aimEpoch=7;menu.aimed=selection;menu.observe(f,old);f.epoch=8;menu.observe(f,old);check(menu.aimed.classId<0);
 // Filter every fallback/hand candidate through the current raw serial, not only the native hit.
 struct Live{unsigned entity=0,serial=0;};std::array<Live,32> live{};f.epoch=7;f.records={local,tank,claw};for(const auto& r:f.records)live[r.index]={100+r.index,r.serial};check(currentAimFrame(f,live).records.size()==3);++live[claw.index].serial;check(currentAimFrame(f,live).records.size()==2);live[tank.index].entity=0;check(currentAimFrame(f,live).records.size()==1);
 // Recognition of a shared-Class-ID survivor uses the role as an experiment source too.
 auto zoey=record(12,260,"CTerrorPlayer",{500,0,0});zoey.team=2;zoey.character=5;auto nearZoey=record(13,300,"CGnome",{520,0,0});auto nearNick=record(14,301,"CPhysicsProp",{20,0,0});f.epoch=7;f.records={local,zoey,nearZoey,nearNick};ProximitySettings proximity;proximity.enabled=true;proximity.triggerName="Zoey";proximity.radius=100;auto neighborhood=evaluateProximity(f,c,proximity,1100);check(neighborhood.sources==1&&neighborhood.inRange.count(nearZoey.handle)&&!neighborhood.inRange.count(nearNick.handle));
 AimSelection body;body.classId=tank.classId;body.name=tank.className;body.handle=tank.handle;body.record=tank;session.begin(3,8,1200);result.id=3;result.epoch=8;result.candidates={selection,body};check(session.accept(result));session.next();check(session.aimed.handle==tank.handle);session.next();check(session.aimed.handle==selection.handle);session.clear();check(!session.accept(result));
 check(!hud::opensChild(127)&&!hud::opensChild(128)&&!hud::opensChild(0x700033)&&!hud::opensChild(0x700034));
 std::cout<<"PASS: torso/crouching/near/nearest geometry, eight SI/survivor roles, native hit and occlusion gates, owner/raw/hands choices, invisible/dead/local exclusions, released carryables, canonical settings migration and async/map identity\n";
}
