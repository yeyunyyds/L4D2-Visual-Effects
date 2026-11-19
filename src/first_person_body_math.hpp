#pragma once
#include "dango/world_camera.hpp"
#include <array>
#include <algorithm>
#include <cmath>
namespace visual::firstperson::math {
inline bool finite(dango::Vec3 v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool offset(const dango::GameCameraFrame& camera,float forward,float right,float up,dango::Vec3& result) {
 dango::CameraPose pose;
 if(!dango::cameraPose(camera,pose))return false;
 auto f=dango::normalize(dango::Vec3{pose.forward.x,pose.forward.y,0});
 auto r=dango::normalize(dango::Vec3{pose.right.x,pose.right.y,0});
 if(dango::length(f)<.99f||dango::length(r)<.99f)return false;
 result=f*forward+r*right+dango::Vec3{0,0,up};return finite(result);
}
inline bool clipPlane(const dango::Matrix4& worldToClip,float maxZ,bool programmable,std::array<float,4>& result) {
 if(!std::isfinite(maxZ))return false;
 const std::array<float,4> world={0,0,-1,maxZ};
 if(!programmable){result=world;return true;}
 dango::Matrix4 inv{};
 if(!dango::inverse(worldToClip,inv))return false;
 for(unsigned row=0;row<4;++row){double sum=0;for(unsigned k=0;k<4;++k)sum+=double(inv[k*4+row])*world[k];
  if(!std::isfinite(sum)||std::abs(sum)>1e7)return false;
  result[row]=float(sum);}
 float scale=0;for(float f:result)scale=std::max(scale,std::abs(f));
 if(!(scale>1e-9f))return false;
 for(auto& f:result)f/=scale;
 return true;
}
}
