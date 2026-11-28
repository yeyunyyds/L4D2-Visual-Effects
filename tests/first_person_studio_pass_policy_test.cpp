#include "first_person_studio_pass_policy.hpp"
#include <cassert>
using visual::firstperson::studiopass::preserveOriginal;
int main(){
 assert(!preserveOriginal(0));
 assert(!preserveOriginal(1));
 assert(!preserveOriginal(0x20));
 assert(preserveOriginal(0x200));
 assert(preserveOriginal(0x201));
 assert(preserveOriginal(0x600));
}
