#pragma once
#include <array>
#include <string_view>
namespace visual {
enum class EntityGroup:unsigned {Special,Survivors,Pickups,Hands,Other};
inline constexpr unsigned EntityGroupCount=5;
inline const char* groupName(EntityGroup group){
 constexpr const char* names[]={u8"特感",u8"求生者",u8"场景可拾取武器与物品",u8"手上道具",u8"其他"};
 auto i=unsigned(group);return i<EntityGroupCount?names[i]:names[4];
}
inline bool handModelClass(std::string_view name){
 return name.find("ViewModel")!=std::string_view::npos||name.find("Viewmodel")!=std::string_view::npos||
        name=="CBaseWeaponWorldModel"||name=="CWeaponWorldModel";
}
// Fixed type catalog, matching DangoMusic's declared filters rather than live entities.
inline constexpr const char* BuiltinEntityClasses[]={
 "Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Tank","Witch",
 "CTerrorPlayer","SurvivorBot",
 "CPistol","CMagnumPistol","CSubMachinegun","CSMG_Silenced","CSMG_MP5",
 "CPumpShotgun","CShotgun_Chrome","CAutoShotgun","CShotgun_SPAS","CAssaultRifle","CRifle_Desert","CRifle_AK47","CRifle_SG552","CRifle_M60",
 "CSniperRifle","CSniper_Military","CSniper_AWP","CSniper_Scout","CGrenadeLauncher","CChainsaw","CTerrorMeleeWeapon",
 "CFirstAidKit","CPainPills","CItem_Adrenaline","CItemDefibrillator","CItemUpgradePackExplosive","CItemUpgradePackIncendiary",
 "CItem_VomitJar","CPipeBomb","CMolotov","CGasCan","CColaBottles","CGnome","COxygenTank","CPropaneTank","CFireworkCrate",
 "CWeaponSpawn","CWeaponAmmoSpawn","CWeaponScavengeItemSpawn","CBaseUpgradeItem",
 "CTerrorViewModel","CBaseViewModel","CBaseWeaponWorldModel","CWeaponWorldModel",
 "Infected","CPhysicsProp","CPhysicsPropMultiplayer","CDynamicProp","CBreakableProp","CFuncTrackTrain",
 "CGrenadeLauncher_Projectile","CMolotovProjectile","CPipeBombProjectile","CVomitJarProjectile",
 "CPropJeep","CPropVehicleDriveable","CPropVehicleChoreoGeneric"
};
inline constexpr unsigned BuiltinEntityClassCount=sizeof(BuiltinEntityClasses)/sizeof(*BuiltinEntityClasses);
inline int builtinClassIndex(std::string_view name){
 for(unsigned i=0;i<BuiltinEntityClassCount;++i)if(name==BuiltinEntityClasses[i])return int(i);
 return -1;
}
inline bool validTypeName(std::string_view name){
 if(name.empty()||name.size()>127)return false;
 for(char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return false;
 return true;
}
inline bool pickupClass(std::string_view name){
 // Concrete pickup classes from DangoMusic's supported client-build catalog.
 constexpr const char* names[]={"CPistol","CMagnumPistol","CSubMachinegun","CSMG_Silenced","CSMG_MP5",
  "CPumpShotgun","CShotgun_Chrome","CAutoShotgun","CShotgun_SPAS","CAssaultRifle","CRifle_Desert","CRifle_AK47","CRifle_SG552","CRifle_M60",
  "CSniperRifle","CSniper_Military","CSniper_AWP","CSniper_Scout","CGrenadeLauncher","CChainsaw","CTerrorMeleeWeapon",
  "CFirstAidKit","CPainPills","CItem_Adrenaline","CItemDefibrillator","CItemUpgradePackExplosive","CItemUpgradePackIncendiary",
  "CItem_VomitJar","CPipeBomb","CMolotov","CGasCan","CColaBottles","CGnome","COxygenTank","CPropaneTank","CFireworkCrate",
  "CWeaponSpawn","CWeaponAmmoSpawn","CWeaponScavengeItemSpawn","CBaseUpgradeItem"};
 for(auto candidate:names){if(name==candidate)return true;}
 return false;
}
inline EntityGroup groupForName(std::string_view name){
 if(handModelClass(name))return EntityGroup::Hands;
 for(auto special:{"Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Tank","Witch","NextBotCombatCharacter"})
  if(name==special)return EntityGroup::Special;
 for(auto actor:{"CTerrorPlayer","SurvivorBot","CCSPlayer","CBasePlayer"})if(name==actor)return EntityGroup::Survivors;
 if(pickupClass(name))return EntityGroup::Pickups;
 return EntityGroup::Other;
}
} // namespace visual
