#pragma once
#include "tank_visual.hpp"
namespace visual::tank {
// Homogeneous clipping keeps the warning fill stable when standing inside a
// sector or when its center leaves the viewport (including near-plane crossings).
inline std::vector<Point> warningTriangle(const dango::GameCameraFrame& camera,float width,float height,const std::array<dango::Vec3,3>& triangle){
 if(!std::isfinite(width)||!std::isfinite(height)||width<1||height<1)return {};
 using Clip=std::array<double,4>;std::vector<Clip> polygon;auto& m=camera.worldToClip;
 for(const auto& p:triangle){Clip v{};for(unsigned row=0;row<4;++row){v[row]=double(m[row*4])*p.x+double(m[row*4+1])*p.y+double(m[row*4+2])*p.z+m[row*4+3];if(!std::isfinite(v[row]))return {};}polygon.push_back(v);}
 auto distance=[](const Clip& v,unsigned plane){switch(plane){case 0:return v[3]-.001;case 1:return v[2];case 2:return v[3]-v[2];case 3:return v[3]+v[0];case 4:return v[3]-v[0];case 5:return v[3]+v[1];default:return v[3]-v[1];}};
 for(unsigned plane=0;plane<7&&!polygon.empty();++plane){std::vector<Clip> next;auto previous=polygon.back();double dp=distance(previous,plane);
 for(const auto& current:polygon){double dc=distance(current,plane);if((dp>=0)!=(dc>=0)){double t=dp/(dp-dc);Clip v{};for(unsigned j=0;j<4;++j)v[j]=previous[j]+(current[j]-previous[j])*t;next.push_back(v);}if(dc>=0)next.push_back(current);previous=current;dp=dc;}
 if(next.size()>16)return {};
 polygon=std::move(next);}
 std::vector<Point> out;for(const auto& v:polygon){if(v[3]<.000999)return {};out.push_back({float((v[0]/v[3]+1)*width*.5),float((1-v[1]/v[3])*height*.5)});}return out;
}
}
