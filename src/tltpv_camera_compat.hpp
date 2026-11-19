#pragma once
// TLTPV uses Source's native firstperson/thirdpersonshoulder commands.
// Only an opt-in first-/third-person camera gate is implemented here.
// Camera separation is checked against the LOCAL networked player, rather
// than assuming a CFG alias or a camera cvar is always authoritative.
#include "dango/world_camera.hpp"
#include <cmath>
#include <optional>
#include <cstdint>
namespace visual::tltpv {
enum class Mode : unsigned char { Unknown, First, Third };
inline const char* label(Mode value) noexcept {
 switch(value){case Mode::First:return "first-person";case Mode::Third:return "third-person";default:return "unverified";}
}
// A shoulder camera at ~50 units is substantially farther from the player
// origin in XY than a normal player eye (even when crouching). Values in the
// intermediate band are intentionally UNVERIFIED, not treated as first person.
inline Mode classify(const dango::CameraPose& camera,
                     const std::optional<dango::Vec3>& playerOrigin,
                     const std::optional<bool>& shoulder) noexcept {
 if(!playerOrigin || !std::isfinite(camera.eye.x)||!std::isfinite(camera.eye.y)||
    !std::isfinite(camera.eye.z)||!std::isfinite(playerOrigin->x)||
    !std::isfinite(playerOrigin->y)||!std::isfinite(playerOrigin->z))return Mode::Unknown;
 const float dx=camera.eye.x-playerOrigin->x,dy=camera.eye.y-playerOrigin->y;
 const float distance=std::hypot(dx,dy);
 const float height=camera.eye.z-playerOrigin->z;
 if(!std::isfinite(distance)||height< -20.f||height>170.f)return Mode::Unknown;
 // Spatial separation is authoritative if a CVar is stale following a
 // direct `firstperson` command. The shoulder value only narrows the gray zone.
 if(distance<=18.f)return Mode::First;
 if(distance>=40.f)return Mode::Third;
 if(shoulder.has_value()){
  if(!*shoulder && distance<=32.f)return Mode::First;
  if(*shoulder && distance>=30.f)return Mode::Third;
 }
 return Mode::Unknown;
}
}
