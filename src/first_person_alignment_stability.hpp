#pragma once
// A failed dynamic solve in one Studio callback must not switch the visible
// first-person body to a different manual pose for a single draw.
#include <cstdint>
namespace visual::firstperson::viewpolicy {
struct AlignmentStability {
 enum class State : unsigned char { Cold, Dynamic, Manual };
 enum class Choice : unsigned char { Dynamic, Hold, Manual };
 static constexpr std::uint64_t HoldMs=48, RecoveryMs=80;
 State state=State::Cold;
 std::uint64_t lastDynamicAt=0,lastObservedAt=0,recoverySince=0;
 bool recovering=false,everDynamic=false;
 void reset() noexcept {*this=AlignmentStability{};}
 Choice decide(bool validDynamic,std::uint64_t now) noexcept {
  if(lastObservedAt&&now<lastObservedAt)reset();
  lastObservedAt=now;
  if(state==State::Cold){
   state=validDynamic?State::Dynamic:State::Manual;
   if(validDynamic){lastDynamicAt=now;everDynamic=true;return Choice::Dynamic;}
   return Choice::Manual;
  }
  if(state==State::Dynamic){
   if(validDynamic){lastDynamicAt=now;return Choice::Dynamic;}
   if(now>=lastDynamicAt&&now-lastDynamicAt<=HoldMs)return Choice::Hold;
   state=State::Manual;recovering=false;return Choice::Manual;
  }
  if(!validDynamic){recovering=false;return Choice::Manual;}
  // Bootstrap from the first valid dynamic result; later recoveries must settle.
  if(!everDynamic){state=State::Dynamic;lastDynamicAt=now;everDynamic=true;return Choice::Dynamic;}
  if(!recovering){recovering=true;recoverySince=now;return Choice::Manual;}
  if(now>=recoverySince&&now-recoverySince>=RecoveryMs){
   state=State::Dynamic;lastDynamicAt=now;recovering=false;
   return Choice::Dynamic;
  }
  return Choice::Manual;
 }
};
}
