#include "first_person_native_visibility.hpp"
#include <cassert>
using namespace visual::firstperson;
int main(){
 using freecamerabody::Mode;
 assert(visibility::allowNativeGate(Mode::WorldAnchored,true,visual::viewcompat::Mode::Unknown,-1));
 assert(visibility::allowNativeGate(Mode::WorldAnchored,true,visual::viewcompat::Mode::Third,1));
 assert(!visibility::allowNativeGate(Mode::NativeThirdPerson,true,visual::viewcompat::Mode::First,0));
 assert(visibility::allowNativeGate(Mode::NormalView,true,visual::viewcompat::Mode::First,0));
 assert(!visibility::allowNativeGate(Mode::NormalView,true,visual::viewcompat::Mode::Third,0));
 assert(!visibility::allowNativeGate(Mode::NormalView,true,visual::viewcompat::Mode::Unknown,0));
 assert(!visibility::allowNativeGate(Mode::NormalView,true,visual::viewcompat::Mode::First,1));
 assert(!visibility::allowNativeGate(Mode::NormalView,false,visual::viewcompat::Mode::Unknown,1));
 assert(visibility::allowNativeGate(Mode::NormalView,false,visual::viewcompat::Mode::Unknown,0));
 assert(visibility::allowNativeGate(Mode::NormalView,false,visual::viewcompat::Mode::Unknown,-1));
}
