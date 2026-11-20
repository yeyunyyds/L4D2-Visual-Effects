#include "free_camera_viewmodel_policy.hpp"
#include <cassert>
using namespace visual::freecamera::viewmodel::policy;
int main(){
 // F6 off: preserve both native outcomes exactly.
 assert(visible(false,true,true));
 assert(!visible(false,true,false));
 // Option off: preserve both native outcomes even with F6 on.
 assert(visible(true,false,true));
 assert(!visible(true,false,false));
 // Both on: mimic third-person viewmodel visibility only.
 assert(!visible(true,true,true));
 assert(!visible(true,true,false));
 // No permanent state transition or client entity modification occurs:
 // returning to normal immediately returns the upstream decision.
 assert(visible(false,true,true));
}
