#include "first_person_body_parts.hpp"
#include <cassert>
#include <cmath>
#include <array>
using namespace visual;
using namespace visual::firstperson::parts;
int main(){
 StudioPose studio{};
 studio.count=10;
 studio.boneNames={"root","valvebiped.bip01_spine","valvebiped.bip01_neck1",
 "valvebiped.bip01_head1","hair_bone","valvebiped.bip01_l_clavicle",
 "valvebiped.bip01_l_upperarm","valvebiped.bip01_l_forearm",
 "valvebiped.bip01_r_upperarm","valvebiped.bip01_r_hand"};
 studio.parents={-1,0,1,2,3,1,5,6,1,8};
 auto plan=discover(studio);
 assert(plan.head==3&&plan.neck==2&&plan.headBones.size()==2);
 assert(plan.leftArm==6&&plan.rightArm==8&&plan.leftBones.size()==2&&plan.rightBones.size()==2);
 std::array<Matrix3x4,10> bone{};
 for(unsigned i=0;i<bone.size();++i){
  for(int k=0;k<3;++k)bone[i].m[k][k]=1;
  bone[i].m[0][3]=float(i*10);
  bone[i].m[2][3]=float(i*20);
 }
 auto initial=bone;
 auto transformed=apply(studio,plan,bone.data(),true,true);
 assert(transformed.head&&transformed.left&&transformed.right&&transformed.modified==6);
 assert(bone[3].m[0][3]==initial[2].m[0][3]);
 assert(bone[4].m[2][3]==initial[2].m[2][3]);
 assert(bone[6].m[0][3]==initial[5].m[0][3]);
 assert(bone[7].m[2][3]==initial[5].m[2][3]);
 assert(bone[8].m[0][3]==initial[1].m[0][3]);
 assert(bone[3].m[0][0]==.0001f);
 // Torso, neck and clavicle must not be edited.
 for(int idx: {0,1,2,5})for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
  assert(bone[idx].m[r][c]==initial[idx].m[r][c]);
 // Invalid model names: do nothing, do not guess head by height.
 auto missing=studio;missing.boneNames[2]="spine";
 auto noHead=discover(missing);assert(noHead.head<0);
 auto again=initial;auto r=apply(missing,noHead,again.data(),true,false);
 assert(!r.head&&r.modified==0);
 // Invalid matrices refuse all effects with no partial writes.
 auto nan=initial;nan[6].m[2][1]=NAN;
 r=apply(studio,plan,nan.data(),true,true);assert(r.modified==0);
 // Prevent cycle traversal and malformed metadata.
 assert(!descendant(std::vector<int>{1,0},0,2));
 auto bad=studio;bad.parents.pop_back();assert(discover(bad).head==-1);
}
