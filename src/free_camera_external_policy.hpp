#pragma once
#include <algorithm>
#include <cstdint>

namespace visual::freecamera::external {
enum class Owner : std::uint32_t { Native=0, User=1, External=2 };

// One camera switch. Owner only decides who supplies the pose, never which
// FreeCamera is running. Explicit OFF cancels the external session.
struct Policy {
 bool allowed=false;
 bool desired=false;
 bool on=false;
 bool hasPose=false;
 unsigned idleSeconds=3;
 std::uint64_t lastInputMs=0,lastPoseMs=0;
 static constexpr std::uint64_t poseLeaseMs=15000;

 void stop() noexcept {on=false;desired=false;hasPose=false;}
 void allow(bool value,std::uint64_t now) noexcept {
  if(!value&&desired)stop();
  allowed=value;
  lastInputMs=now;
 }
 void setIdle(unsigned value) noexcept {idleSeconds=std::clamp(value,1u,30u);}
 bool request(bool value,std::uint64_t now) noexcept {
  if(!value){if(desired)stop();return true;}
  if(!allowed)return false;
  if(!desired){desired=true;on=true;hasPose=false;lastPoseMs=now;lastInputMs=now;}
  return true;
 }
 void setPose(std::uint64_t now) noexcept {if(desired){hasPose=true;lastPoseMs=now;}}
 void noteInput(std::uint64_t now) noexcept {lastInputMs=now;}
 void toggle(std::uint64_t now) noexcept {if(on||desired)stop();else{on=true;lastInputMs=now;}}
 bool leaseAlive(std::uint64_t now) const noexcept {
  return now>=lastPoseMs&&now-lastPoseMs<=poseLeaseMs;
 }
 void expire(std::uint64_t now) noexcept {if(desired&&!leaseAlive(now))stop();}
 std::uint32_t remainingMs(std::uint64_t now) const noexcept {
  const std::uint64_t threshold=std::uint64_t(idleSeconds)*1000;
  if(now<lastInputMs)return static_cast<std::uint32_t>(threshold);
  const auto elapsed=now-lastInputMs;
  return elapsed>=threshold?0:static_cast<std::uint32_t>(threshold-elapsed);
 }
 Owner select(std::uint64_t now) const noexcept {
  if(!on)return Owner::Native;
  if(allowed&&desired&&hasPose&&leaseAlive(now)&&remainingMs(now)==0)return Owner::External;
  return Owner::User;
 }
};
} // namespace visual::freecamera::external
