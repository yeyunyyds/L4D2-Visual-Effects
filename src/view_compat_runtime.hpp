#pragma once
// Verified, read-only 2026 L4D2 client.dll ConVar probe for a verified native
// camera-state indicator. This never calls an unverified engine virtual
// and never writes/redirects game memory. No external camera add-on hooks or dependencies.
#include "view_compat.hpp"
#include "module_verification.hpp"
#include "dango/source_entities.hpp"
#include <atomic>
#include <optional>
#include <cstdint>
#include <cstring>
#include <cstdio>
namespace visual::viewcompat {
inline std::atomic<std::uintptr_t> acceptedImage{0};
inline std::atomic<Mode> mode{Mode::Unknown};
inline std::atomic<unsigned> generation{1};
inline std::atomic<const char*> status{"View compatibility disabled"};
inline std::atomic<int> shoulderValue{-1};
inline std::atomic<int> horizontalUnits{-1},eyeHeightUnits{-1};
inline CameraLatch latch{}; // Game-thread only: never modified from Studio draw.
inline void reset() noexcept {
 latch.reset();
 if(mode.exchange(Mode::Unknown,std::memory_order_acq_rel)!=Mode::Unknown)
  generation.fetch_add(1,std::memory_order_acq_rel);
 status="View mode unverified";
 shoulderValue=-1;horizontalUnits=-1;eyeHeightUnits=-1;
}
inline std::optional<bool> readShoulder() noexcept {
 auto module=source_camera::clientModule;
 if(!module)return std::nullopt;
 auto base=std::uintptr_t(module);
 if(acceptedImage.load(std::memory_order_acquire)!=base){
  if(!source_camera::matchesImage(module,source_camera::ClientTimestamp,
        source_camera::ClientImageSize,"view-mode client")||
     !verifiedModuleFile(module,
        "c0a8e1e88f7312db11bcfd4b0ec574d224a34b2d9a615d6088b28335eeb4af1f"))
   return std::nullopt;
  acceptedImage.store(base,std::memory_order_release);
 }
 // Confirmed against the provided, SHA-verified 2026 client.dll:
 // client+0x500650 registers ConVar("c_thirdpersonshoulder") at +0x7589c8;
 // the ConVar's name, parent and cached integer occupy +0x0c,+0x1c,+0x30.
 constexpr std::uintptr_t kConVarRva=0x7589c8;
 const auto location=std::uint32_t(base+kConVarRva);
 std::uint32_t name=0,parent=0;
 if(!source_entities::reader.read(location+0x0c,&name,4)||
    !source_entities::reader.read(location+0x1c,&parent,4)||
    !source_camera::owns(reinterpret_cast<void*>(std::uintptr_t(name)),module)||
    !source_camera::owns(reinterpret_cast<void*>(std::uintptr_t(parent)),module))return std::nullopt;
 constexpr char expected[]="c_thirdpersonshoulder";
 char text[sizeof(expected)]{};
 if(!source_entities::reader.read(name,text,sizeof(text))||
    std::memcmp(text,expected,sizeof(expected)))return std::nullopt;
 std::int32_t value=-1;
 if(!source_entities::reader.read(parent+0x30,&value,4)||
    (value!=0&&value!=1))return std::nullopt;
 return value==1;
}
// Refresh the player's verified m_vecOrigin directly from the live entity.
// The inventory snapshot is captured only once per 200ms and is too old for
// a movement-sensitive camera comparison. This reads a previously verified
// recvprop; it does not dispatch new engine virtual methods.
inline std::optional<dango::Vec3> freshLocalOrigin(std::uint32_t entity) noexcept {
 if(!entity)return std::nullopt;
 for(const auto& pair:source_entities::classes){
  if(pair.second.name=="CTerrorPlayer"){
   return source_entities::vectorField(entity,pair.second,"m_vecOrigin");
  }
 }
 return std::nullopt;
}
// Only called from firstperson::onFrame after the local player is verified.
// Draw-time code reads the committed mode but does not update it.
inline Mode detectAndCommit(const dango::GameCameraFrame& camera,
                            const std::optional<dango::Vec3>& origin) noexcept {
 dango::CameraPose pose{};
 auto shoulder=readShoulder();
 shoulderValue=shoulder?(*shoulder?1:0):-1;
 Observation evidence{};
 if(camera.active&&dango::cameraPose(camera,pose))
  evidence=classify(pose,origin,shoulder);
 // The actual shoulder camera should remain Third even if the entity origin
 // cannot be read for one frame (but camera validity is still required).
 if(camera.active&&shoulder&&*shoulder)evidence.evidence=Mode::Third;
 horizontalUnits=std::isfinite(evidence.horizontal)?int(evidence.horizontal):-1;
 eyeHeightUnits=std::isfinite(evidence.eyeHeight)?int(evidence.eyeHeight):-1;
 auto previous=mode.load(std::memory_order_acquire);
 auto resolved=latch.update(evidence.evidence,camera.tick,camera.epoch);
 if(resolved!=previous){
  mode.store(resolved,std::memory_order_release);
  generation.fetch_add(1,std::memory_order_acq_rel);
  char info[256]{};
  std::snprintf(info,sizeof(info),"FPB view committed: %s (sample=%s shoulder=%d xy=%d eyeZ=%d)",
   label(resolved),label(evidence.evidence),shoulderValue.load(),
   horizontalUnits.load(),eyeHeightUnits.load());
  source_camera::log(info);
 }
 return resolved;
}
} // namespace visual::viewcompat
