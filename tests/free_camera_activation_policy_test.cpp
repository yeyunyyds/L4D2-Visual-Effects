#include "free_camera_activation_policy.hpp"
#include <cassert>
using namespace visual::freecamera;
int main(){
 using activation::Phase;
 assert(activation::phase(false,false,false,false)==Phase::Off);
 assert(activation::phase(true,false,false,false)==Phase::Binding);
 assert(activation::phase(true,true,false,false)==Phase::Starting);
 assert(activation::phase(true,true,true,false)==Phase::Active);
 assert(activation::phase(true,true,true,true)==Phase::Paused);
 assert(!activation::shouldBind(false,false));
 assert(activation::shouldBind(true,false));
 assert(!activation::shouldBind(true,true));
 assert(!activation::shouldConsumeInput(true,false));
 assert(activation::shouldConsumeInput(true,true));
 assert(!activation::shouldConsumeInput(false,true));
}
