#pragma once
#include "dango/world_camera.hpp"
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
} // namespace visual::firstperson::math
