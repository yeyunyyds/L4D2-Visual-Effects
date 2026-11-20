#pragma once
// TLTPV uses L4D2's native firstperson / thirdpersonshoulder commands.
// These camera observations are evidence, not an immediate permission to
// rewrite the local DrawModel branch. A state machine in tltpv_runtime.hpp
// commits changes only on the post-render game thread.
#include "dango/world_camera.hpp"
#include <cmath>
#include <optional>
#include <cstdint>
namespace visual::tltpv {
enum class Mode : unsigned char { Unknown, First, Third };
inline const char* label(Mode value) noexcept {
 switch(value){case Mode::First:return "first-person";case Mode::Third:return "third-person";default:return "unverified";}
}
struct Observation {
 Mode evidence=Mode::Unknown;
 float horizontal=-1.f, eyeHeight=-1.f;
};
// In first person the camera is at the player's eye height, not merely close
// to their XY origin. Looking steeply down from third person can make the
// horizontal separation zero while leaving a large Z offset.
// CVar reads may be stale after Source camera commands; a CVar value alone
// is therefore never used to assert First when the geometry contradicts it.
inline Observation classify(const dango::CameraPose& camera,
                     const std::optional<dango::Vec3>& playerOrigin,
                     const std::optional<bool>& shoulder) noexcept {
 Observation out{};
 if(!playerOrigin || !std::isfinite(camera.eye.x)||!std::isfinite(camera.eye.y)||
    !std::isfinite(camera.eye.z)||!std::isfinite(playerOrigin->x)||
    !std::isfinite(playerOrigin->y)||!std::isfinite(playerOrigin->z))return out;
 const float dx=camera.eye.x-playerOrigin->x,dy=camera.eye.y-playerOrigin->y;
 out.horizontal=std::hypot(dx,dy);
 out.eyeHeight=camera.eye.z-playerOrigin->z;
 if(!std::isfinite(out.horizontal)||out.eyeHeight< -40.f||out.eyeHeight>220.f)return out;
 // A positive shoulder CVar is strong third-person evidence, including
 // near-vertical top-down views and camera collision against geometry.
 if(shoulder && *shoulder){out.evidence=Mode::Third;return out;}
 const bool eyeHeight=(out.eyeHeight>=30.f&&out.eyeHeight<=88.f);
 // With shoulder explicitly off, permit enough horizontal motion to cover
 // network/interpolation lag. Live read of the local origin is preferred.
 if(shoulder && !*shoulder){
  if(eyeHeight&&out.horizontal<=58.f)out.evidence=Mode::First;
  else if(out.horizontal>=85.f||out.eyeHeight>130.f)out.evidence=Mode::Third;
  return out;
 }
 // Without an available verified CVar, prefer no new camera mode to a bad
 // guess. The 3D eye requirement prevents close XY top-down false positives.
 if(eyeHeight&&out.horizontal<=12.f)out.evidence=Mode::First;
 else if(out.horizontal>=75.f||out.eyeHeight>130.f)out.evidence=Mode::Third;
 return out;
}
// Only confirmed mode transitions should modify the native draw branch.
// One stale entity snapshot must not flip it back and forth. Unknown input
// preserves the old mode briefly; prolonged uncertainty fails closed.
struct CameraLatch {
 Mode committed=Mode::Unknown,candidate=Mode::Unknown;
 std::uint64_t since=0,lastTick=0,unknownSince=0;
 std::uint32_t epoch=0;
 void reset() noexcept {committed=candidate=Mode::Unknown;since=lastTick=unknownSince=0;epoch=0;}
 Mode update(Mode incoming,std::uint64_t tick,std::uint32_t currentEpoch) noexcept {
  if(epoch!=currentEpoch||tick<lastTick||tick-lastTick>1000){
   reset();epoch=currentEpoch;
  }
  lastTick=tick;
  if(incoming==Mode::Unknown){
   candidate=Mode::Unknown;since=0;
   if(!unknownSince)unknownSince=tick?tick:1;
   if(tick>=unknownSince && tick-unknownSince>500)committed=Mode::Unknown;
   return committed;
  }
  unknownSince=0;
  if(incoming==Mode::Third){ // safety: disable the extra local draw immediately
   committed=candidate=Mode::Third;since=tick;return committed;
  }
  if(committed==Mode::First){candidate=Mode::First;since=tick;return committed;}
  if(candidate!=Mode::First){candidate=Mode::First;since=tick;return committed;}
  // At least two observations spanning 96ms before enabling local draw.
  if(tick>=since&&tick-since>=96)committed=Mode::First;
  return committed;
 }
};
} // namespace visual::tltpv
