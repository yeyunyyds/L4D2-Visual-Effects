#include "first_person_alignment_stability.hpp"
#include <cassert>
using visual::firstperson::viewpolicy::AlignmentStability;
int main(){
 using C=AlignmentStability::Choice;
 AlignmentStability s;
 assert(s.decide(true,1000)==C::Dynamic);
 assert(s.decide(false,1015)==C::Hold);
 assert(s.decide(false,1048)==C::Hold);
 assert(s.decide(false,1050)==C::Manual);
 assert(s.decide(true,1060)==C::Manual);
 assert(s.decide(false,1070)==C::Manual);
 assert(s.decide(true,1080)==C::Manual);
 assert(s.decide(true,1159)==C::Manual);
 assert(s.decide(true,1160)==C::Dynamic);
 assert(s.decide(false,1170)==C::Hold);
 s.reset();
 assert(s.decide(false,1200)==C::Manual);
 assert(s.decide(true,1210)==C::Dynamic);
 assert(s.decide(false,1215)==C::Hold);
 assert(s.decide(true,1290)==C::Dynamic);
 s.reset();
 assert(s.decide(true,2000)==C::Dynamic);
 assert(s.decide(true,2010)==C::Dynamic);
 assert(s.decide(false,2055)==C::Hold);
 assert(s.decide(false,2059)==C::Manual);
 // A secondary/manual callback before each main-view callback cannot
 // repeatedly force a manual/dynamic output flip.
 s.reset();
 assert(s.decide(false,3000)==C::Manual);
 assert(s.decide(true,3000)==C::Dynamic);
 for(unsigned f=1;f<25;++f){
  const auto now=3000u+f*16u;
  assert(s.decide(false,now)==C::Hold);
  assert(s.decide(true,now)==C::Dynamic);
 }
 s.reset();
 assert(s.decide(true,5000)==C::Dynamic);
 assert(s.decide(false,4990)==C::Manual); // clock backwards invalidates history
}
