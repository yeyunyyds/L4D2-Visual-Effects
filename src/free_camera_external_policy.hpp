#pragma once
#include <algorithm>
#include <cstdint>

namespace visual::freecamera::external {
enum class Owner : std::uint32_t { Native=0, User=1, External=2 };

// Pure decision layer, independent of Win32 hooks and easy to regression test.
// Before first acquisition the game remains native. Once external view has
// appeared, user input relinquishes it to MANUAL FreeCamera at the same pose.
struct Policy {
 bool allowed=false;
 bool desired=false;
 bool manualPinned=false;
 bool hasPose=false;
 bool everEngaged=false;
 unsigned idleSeconds=3;
 std::uint64_t lastInputMs=0;
 std::uint64_t lastPoseMs=0;
 static constexpr std::uint64_t poseLeaseMs=15000;

 void allow(bool value,std::uint64_t now) noexcept {
  allowed=value;
  if(!value){desired=false;hasPose=false;everEngaged=false;}
  lastInputMs=now;
 }
 void setIdle(unsigned value) noexcept {idleSeconds=std::clamp(value,1u,30u);}
 bool request(bool value) noexcept {
  if(value&&!allowed)return false;
  desired=value;
  if(!value){hasPose=false;everEngaged=false;}
  return true;
 }
 void setPose(std::uint64_t now) noexcept {hasPose=true;lastPoseMs=now;}
 void noteInput(std::uint64_t now) noexcept {lastInputMs=now;}
 void setManual(bool enabled) noexcept {manualPinned=enabled;}
 void cancelExternal() noexcept {desired=false;hasPose=false;everEngaged=false;}
 void stop() noexcept {cancelExternal();manualPinned=false;}

 bool leaseAlive(std::uint64_t now) const noexcept {
  return hasPose&&now>=lastPoseMs&&now-lastPoseMs<=poseLeaseMs;
 }
 std::uint32_t remainingMs(std::uint64_t now) const noexcept {
  const std::uint64_t threshold=std::uint64_t(idleSeconds)*1000;
  if(now<lastInputMs)return static_cast<std::uint32_t>(threshold);
  const auto elapsed=now-lastInputMs;
  return elapsed>=threshold?0:static_cast<std::uint32_t>(threshold-elapsed);
 }
 Owner select(std::uint64_t now) const noexcept {
  if(manualPinned)return Owner::User;
  if(!allowed||!desired||!leaseAlive(now))return Owner::Native;
  if(remainingMs(now))return everEngaged?Owner::User:Owner::Native;
  return Owner::External;
 }
 void onExternalFrame() noexcept {everEngaged=true;}
};
} // namespace visual::freecamera::external
