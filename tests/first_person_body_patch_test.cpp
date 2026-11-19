#include "first_person_body_patch.hpp"
#include <array>
#include <algorithm>
#include <cassert>
int main(){
 using namespace visual::firstperson::branch;
 std::array<unsigned char,sizeof(GateBytes)> original{};
 std::copy(GateBytes,GateBytes+sizeof(GateBytes),original.begin());
 assert(matches(original.data(),original.size(),false));
 auto patched=original;patched[DisplacementOffset]=PassThrough;
 assert(matches(patched.data(),patched.size(),true));
 assert(!matches(patched.data(),patched.size(),false));
 assert(GateBytes[DisplacementOffset-1]==0x74);
 assert(DisplacementRva+1+PassThrough==DisplacementRva+1);
 for(std::size_t i=0;i<original.size();++i){
  if(i==DisplacementOffset)continue;
  auto modified=original;modified[i]^=1;
  assert(!matches(modified.data(),modified.size(),true));
 }
 auto bad=original;bad[DisplacementOffset]=0x12;
 assert(!matches(bad.data(),bad.size(),true));
}
