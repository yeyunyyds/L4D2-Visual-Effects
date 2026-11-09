#pragma once
#include "dango/entity_inventory.hpp"
#include "special_infected.hpp"
namespace visual::rocks {
enum class Identity {None,DedicatedClass,TankOwnedSharedProjectile};
inline bool rockClass(const std::string& name){return name=="CTankRock"||name=="TankRock";}
inline bool validIdentity(const dango::inventory::Record& r){return r.index<2048&&r.handle!=0xffffffffu&&r.serial==(r.handle>>12)&&(r.handle&4095u)==r.index;}
struct Identification {Identity kind=Identity::None;};
inline Identification identifyRock(const dango::inventory::Record& r,const dango::inventory::Frame& frame){
 if(!validIdentity(r))return {};
 if(rockClass(r.className))return {Identity::DedicatedClass};
 if(r.className!="CBaseCSGrenadeProjectile")return {};
 // A known thrower outranks owner. Ordinary player grenades are never admitted by class alone.
 auto actor=r.thrower&&*r.thrower!=0xffffffffu?r.thrower:r.owner;if(!actor||*actor==0xffffffffu)return {};
 for(const auto& candidate:frame.records)if(candidate.handle==*actor&&validIdentity(candidate))return specialCharacter(candidate)==specialIndex("Tank")?Identification{Identity::TankOwnedSharedProjectile}:Identification{};
 return {};
}
}
