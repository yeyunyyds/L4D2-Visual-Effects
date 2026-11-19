#pragma once
// Original render-only bone-branch handling for Evo-style first-person presentation.
// Operates ONLY on caller-owned per-draw Studio matrix copies.
#include "studio_pose.hpp"
#include <array>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace visual::firstperson::parts {
struct Selection {
 int head=-1, neck=-1, leftArm=-1, rightArm=-1;
 std::vector<unsigned> headBones, leftBones, rightBones;
};
inline bool namedEnd(const std::string& name,const char* stem) {
 const std::size_t n=std::char_traits<char>::length(stem);
 auto pos=name.rfind(stem);
 if(pos==std::string::npos)return false;
 // Skip words such as forehead, headset, ahead or forehead_attachment.
 if(pos && name[pos-1]!='_' && name[pos-1]!='.' && name[pos-1]!=':' &&
    name[pos-1]!='/' && name[pos-1]!='-')return false;
 auto suffix=pos+n;
 while(suffix<name.size() && (name[suffix]>='0'&&name[suffix]<='9'))++suffix;
 return suffix==name.size();
}
inline bool headName(const std::string& name){
 return namedEnd(name,"head") || namedEnd(name,"head1") || namedEnd(name,"head_01");
}
inline bool neckName(const std::string& name){return namedEnd(name,"neck")||namedEnd(name,"neck1");}
inline bool upperArmName(const std::string& name){
 return namedEnd(name,"upperarm") || namedEnd(name,"upper_arm") || namedEnd(name,"upperarm1");
}
inline int side(const std::string& name){
 if(name.find("_l_")!=std::string::npos||name.find("left")!=std::string::npos||
    name.find(".l_")!=std::string::npos)return 1;
 if(name.find("_r_")!=std::string::npos||name.find("right")!=std::string::npos||
    name.find(".r_")!=std::string::npos)return 2;
 return 0;
}
inline bool descendant(const std::vector<int>& parents,unsigned index,int root) {
 if(root<0||index>=parents.size())return false;
 int current=int(index);
 for(std::size_t n=0;n<=parents.size();++n){
  if(current==root)return true;
  if(current<0||unsigned(current)>=parents.size())return false;
  current=parents[unsigned(current)];
 }
 return false; // malformed or cyclic hierarchy
}
inline Selection discover(const StudioPose& studio){
 Selection s;
 const auto count=studio.count;
 if(!count||count>MaxBones||studio.parents.size()!=count||studio.boneNames.size()!=count)return s;
 // Require head to be a child of an actual neck, not an arbitrary mesh name.
 for(unsigned i=0;i<count;++i){
  if(!headName(studio.boneNames[i]))continue;
  int parent=studio.parents[i];
  for(unsigned depth=0;parent>=0&&depth<count;++depth){
   if(unsigned(parent)>=count)break;
   if(neckName(studio.boneNames[unsigned(parent)])){
    s.head=int(i);s.neck=parent;break;
   }
   parent=studio.parents[unsigned(parent)];
  }
  if(s.head>=0)break;
 }
 for(unsigned i=0;i<count;++i){
  if(!upperArmName(studio.boneNames[i])||studio.parents[i]<0)continue;
  const int which=side(studio.boneNames[i]);
  if(which==1&&s.leftArm<0)s.leftArm=int(i);
  if(which==2&&s.rightArm<0)s.rightArm=int(i);
 }
 for(unsigned i=0;i<count;++i){
  if(s.head>=0&&descendant(studio.parents,i,s.head))s.headBones.push_back(i);
  if(s.leftArm>=0&&descendant(studio.parents,i,s.leftArm))s.leftBones.push_back(i);
  if(s.rightArm>=0&&descendant(studio.parents,i,s.rightArm))s.rightBones.push_back(i);
 }
 return s;
}
inline bool matrixValid(const Matrix3x4& m){
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
  if(!std::isfinite(m.m[r][c]))return false;
 return true;
}
// Small nonzero scale avoids singular matrices in Source's Studio backend.
inline void collapse(Matrix3x4& destination,const Matrix3x4& original,const float point[3]){
 constexpr float tiny=0.0001f;
 for(unsigned r=0;r<3;++r){
  for(unsigned c=0;c<3;++c)destination.m[r][c]=original.m[r][c]*tiny;
  destination.m[r][3]=point[r];
 }
}
struct Result {bool head=false,left=false,right=false;unsigned modified=0;};
inline Result apply(const StudioPose& pose,const Selection& plan,Matrix3x4* copy,
                   bool hideHead,bool hideArms){
 Result result;
 if(!copy||pose.count==0||pose.count>MaxBones)return result;
 for(unsigned i=0;i<pose.count;++i)if(!matrixValid(copy[i]))return result;
 auto branch=[&](const std::vector<unsigned>& indices,int pivot)->bool{
  if(pivot<0||unsigned(pivot)>=pose.count||indices.empty())return false;
  float point[3]{};
  for(unsigned axis=0;axis<3;++axis)point[axis]=copy[unsigned(pivot)].m[axis][3];
  for(auto i:indices)if(i>=pose.count||int(i)==pivot)return false;
  for(auto i:indices){
   Matrix3x4 original=copy[i];collapse(copy[i],original,point);++result.modified;
  }
  return true;
 };
 if(hideHead && plan.head>=0 && plan.neck>=0)
  result.head=branch(plan.headBones,plan.neck);
 if(hideArms){
  if(plan.leftArm>=0 && plan.leftArm<int(pose.count))
   result.left=branch(plan.leftBones,pose.parents[unsigned(plan.leftArm)]);
  if(plan.rightArm>=0 && plan.rightArm<int(pose.count))
   result.right=branch(plan.rightBones,pose.parents[unsigned(plan.rightArm)]);
 }
 return result;
}
} // namespace visual::firstperson::parts
