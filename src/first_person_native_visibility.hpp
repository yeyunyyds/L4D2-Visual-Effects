#pragma once
// Restrict the reversible local DrawModel byte patch to views that need it.
// Normal third-person/unknown views return ownership to the game itself.
#include "free_camera_body_policy.hpp"
#include "view_compat.hpp"
namespace visual::firstperson::visibility {
inline bool allowNativeGate(freecamerabody::Mode cameraMode,
                            bool thirdPersonCompatibility,
                            viewcompat::Mode committedView,
                            int nativeShoulder) noexcept {
 if(cameraMode==freecamerabody::Mode::NativeThirdPerson)return false;
 // Visual-owned detached camera deliberately exposes the world-anchored body.
 if(cameraMode==freecamerabody::Mode::WorldAnchored)return true;
 // Explicit native shoulder view always belongs to the game.
 if(nativeShoulder==1)return false;
 // Optional compatibility avoids opening the branch until first person
 // has been positively committed, rather than keeping it open everywhere.
 if(thirdPersonCompatibility)return committedView==viewcompat::Mode::First;
 return true;
}
} // namespace visual::firstperson::visibility
