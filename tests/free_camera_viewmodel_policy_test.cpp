#include "free_camera_viewmodel_policy.hpp"
#include <cassert>
using namespace visual::freecamera::viewmodel::policy;
int main(){
 assert(classIsViewmodel("CBaseViewModel"));
 assert(classIsViewmodel("CPredictedViewModel"));
 assert(!classIsViewmodel("CTerrorPlayer"));
 assert(!classIsViewmodel("CWeaponRifle"));
 assert(!classIsViewmodel("CBaseAnimating"));
 constexpr std::uint32_t player=0x2001,vm=0x3002,render=0x12345678u;
 assert(ownedByLocal(player,vm,vm,0,"CBaseViewModel"));
 assert(ownedByLocal(player,0xffffffffu,vm,player,"CBaseViewModel"));
 assert(!ownedByLocal(player,vm,vm,0,"CTerrorPlayer"));
 assert(!ownedByLocal(player,vm,0x4003,0x5004,"CBaseViewModel"));
 assert(!ownedByLocal(0xffffffffu,vm,vm,player,"CBaseViewModel"));
 assert(suppress(true,true,render,render,true,true,true));
 assert(!suppress(false,true,render,render,true,true,true));
 assert(!suppress(true,false,render,render,true,true,true));
 assert(!suppress(true,true,render,render,false,true,true));
 assert(!suppress(true,true,render,render,true,false,true));
 assert(!suppress(true,true,render,render,true,true,false));
 assert(!suppress(true,true,render,0x1234567cu,true,true,true));
 assert(!suppress(true,true,0,0,true,true,true));
}
