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
