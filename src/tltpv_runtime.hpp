#pragma once
// Verified, read-only 2026 L4D2 client.dll ConVar probe for TLTPV's native
// c_thirdpersonshoulder state. This never calls an unverified engine virtual
// and never writes/redirects game memory. No TLTPV hooks or file dependencies.
#include "tltpv_camera_compat.hpp"
#include "module_verification.hpp"
#include "dango/source_entities.hpp"
#include <atomic>
#include <optional>
#include <cstdint>
#include <cstring>
namespace visual::tltpv {
inline std::atomic<std::uintptr_t> acceptedImage{0};
inline std::atomic<Mode> mode{Mode::Unknown};
inline std::atomic<unsigned> generation{1};
inline std::atomic<const char*> status{"TLTPV compatibility disabled"};
inline std::atomic<int> shoulderValue{-1};
inline void observe(Mode next) noexcept {
 if(mode.exchange(next,std::memory_order_acq_rel)!=next)
  generation.fetch_add(1,std::memory_order_acq_rel);
}
inline void reset() noexcept {
 observe(Mode::Unknown);
 status="TLTPV mode unverified";
 shoulderValue=-1;
}
inline std::optional<bool> readShoulder() noexcept {
 auto module=source_camera::clientModule;
 if(!module)return std::nullopt;
 auto base=std::uintptr_t(module);
 if(acceptedImage.load(std::memory_order_acquire)!=base){
  if(!source_camera::matchesImage(module,source_camera::ClientTimestamp,
        source_camera::ClientImageSize,"TLTPV cvar client")||
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
inline Mode detect(const dango::GameCameraFrame& camera,
                   const std::optional<dango::Vec3>& origin) noexcept {
 dango::CameraPose pose{};
 auto shoulder=readShoulder();
 shoulderValue=shoulder?(*shoulder?1:0):-1;
 if(!camera.active||!dango::cameraPose(camera,pose))return Mode::Unknown;
 return classify(pose,origin,shoulder);
}
} // namespace visual::tltpv
