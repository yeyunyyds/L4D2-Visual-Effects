#include "free_camera_body_policy.hpp"
#include <cassert>

using namespace visual::firstperson::freecamerabody;
int main(){
 // Normal camera path retains the existing FPB view-compat policy.
 assert(choose(false,false,false)==Mode::NormalView);
 assert(choose(false,true,true)==Mode::NormalView);
 assert(choose(false,true,std::nullopt)==Mode::NormalView);

 // Camera distance is intentionally NOT part of the detached policy.
 // With compatibility enabled and native shoulder explicitly OFF, the
 // local player remains visible at the true game-world origin.
 assert(choose(true,true,false)==Mode::WorldAnchored);
 assert(choose(true,true,std::nullopt)==Mode::WorldAnchored);
 assert(choose(true,false,false)==Mode::WorldAnchored);
 assert(choose(true,false,true)==Mode::WorldAnchored);

 // Explicit native third-person shoulder retains native drawing behavior.
 assert(choose(true,true,true)==Mode::NativeThirdPerson);

 // Entirely bypass body relocation, head/arm shrink and camera yaw steering
 // while the FreeCamera is requested. Native bones are forwarded unchanged.
 assert(passOriginalStudio(true));
 assert(!passOriginalStudio(false));

 // Both F6 edges invalidate the normal camera classifier's old latch.
 Transition state;
 assert(!state.update(false));
 assert(state.update(true));
 assert(!state.update(true));
 assert(state.update(false));
 assert(!state.update(false));
 state.reset();
 assert(state.update(true));
}
