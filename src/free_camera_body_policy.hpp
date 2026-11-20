#pragma once
// FreeCamera only changes the view; the player remains the real CTerrorPlayer.
// This policy does not create models, entities or any server-side state.
#include <optional>

namespace visual::firstperson::freecamerabody {
enum class Mode { NormalView, WorldAnchored, NativeThirdPerson };

// The free camera can be thousands of units away: its distance to the player
// must never classify the actual game view as third person. A verified active
// native shoulder camera still owns its own player rendering.
inline Mode choose(bool freeCamera, bool thirdPersonCompatibility,
                   std::optional<bool> nativeShoulder) noexcept {
 if(!freeCamera)return Mode::NormalView;
 if(thirdPersonCompatibility&&nativeShoulder&&*nativeShoulder)
  return Mode::NativeThirdPerson;
 return Mode::WorldAnchored;
}

// FreeCamera's body stays in its original Studio model position and pose;
// no camera-relative offset, dynamic anchor, head/arm shrink or yaw tracking.
inline bool passOriginalStudio(bool freeCamera) noexcept {
 return freeCamera;
}

// Called only in the post-render game-frame callback, not Studio draws.
struct Transition {
 bool previous=false;
 bool update(bool freeCamera) noexcept {
  const bool changed=previous!=freeCamera;
  previous=freeCamera;
  return changed;
 }
 void reset() noexcept {previous=false;}
};
} // namespace visual::firstperson::freecamerabody
