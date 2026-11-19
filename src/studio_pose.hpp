#pragma once
#include "stretch.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace visual {
struct StudioPose {
    unsigned count{};
    std::string name;
    std::vector<unsigned> feet;
    std::vector<unsigned> fixed;
    // Validated MDL hierarchy for render-only first-person part hiding.
    std::vector<int> parents;
    std::vector<std::string> boneNames;
    // mstudiohdr_t checksum/eye and inverse-bind bone positions (model local).
    std::uint32_t checksum=0;
    std::array<float,3> eyeLocal{};
    bool eyeValid=false;
    std::vector<std::array<float,3>> bindLocal;
    std::vector<unsigned char> bindValid;
};
// Read the inverse bind transform at mstudiobone_t::poseToBone (+96).
// The inverse matrix maps the model origin of this bone back to model space.
inline bool inverseBindPosition(const float m[12],std::array<float,3>& p){
 for(unsigned k=0;k<12;++k)if(!std::isfinite(m[k]))return false;
 const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
 const double A=e*i-f*h,B=c*h-b*i,C=b*f-c*e;
 const double D=f*g-d*i,E=a*i-c*g,F=c*d-a*f;
 const double G=d*h-e*g,H=b*g-a*h,I=a*e-b*d;
 const double det=a*A+b*D+c*G;
 if(!std::isfinite(det)||std::abs(det)<.1||std::abs(det)>10)return false;
 const double tx=m[3],ty=m[7],tz=m[11];
 const double x=-(A*tx+B*ty+C*tz)/det;
 const double y=-(D*tx+E*ty+F*tz)/det;
 const double z=-(G*tx+H*ty+I*tz)/det;
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||std::abs(x)>4096||std::abs(y)>4096||std::abs(z)>4096)return false;
 p={float(x),float(y),float(z)};return true;
}
template<class Reader> bool studioPose(Reader& reader, std::uint32_t header, StudioPose& out,const char** reason=nullptr) {
    auto fail=[&](const char* why){if(reason)*reason=why;return false;};
    std::array<std::uint32_t,42> words{};
    if (!reader.read(header,words.data(),sizeof(words))) return fail("header unreadable");
    if (words[0]!=0x54534449 || (words[1]!=48 && words[1]!=49)) return fail("studio magic/version rejected");
    const auto length=words[19],count=words[39],offset=words[40];
    if (length<sizeof(words) || length>16*1024*1024 || !count || count>MaxBones ||
        offset<sizeof(words) || std::uint64_t(offset)+count*216>length ||
        std::uint64_t(header)+length>UINT32_MAX) return fail("studio bounds/count rejected");
    StudioPose result; result.count=count;result.checksum=words[2];
    std::memcpy(result.eyeLocal.data(),words.data()+20,12);
    result.eyeValid=std::all_of(result.eyeLocal.begin(),result.eyeLocal.end(),[](float value){
      return std::isfinite(value)&&std::abs(value)<=256.f;
    });
    result.bindLocal.resize(count);result.bindValid.assign(count,0);
    const char* name=reinterpret_cast<const char*>(words.data())+12;
    const auto end=std::find(name,name+64,'\0');
    if(end==name+64 || end==name)return fail("studio model name invalid");
    result.name.assign(name,end);out.name=result.name;out.count=count;
    std::vector<int> parents(count);
    std::vector<std::string> boneNames(count);
    for(unsigned i=0;i<count;++i) {
        const auto bone=header+offset+i*216;
        std::int32_t displacement{};
        if(!reader.read(bone,&displacement,4) || displacement<=0) return fail("bone name offset invalid");
        if(!reader.read(bone+4,&parents[i],4) || parents[i]<-1 || parents[i]>=int(count) || parents[i]==int(i))return fail("bone parent index invalid");
        const auto address=std::uint64_t(bone)+displacement;
        if(address>=std::uint64_t(header)+length) return fail("bone name address out of bounds");
        std::string boneName;
        for(unsigned j=0;j<128;++j) {
            char ch{};
            if(address+j>=std::uint64_t(header)+length ||
               !reader.read(std::uint32_t(address+j),&ch,1))return fail("bone name string unreadable");
            if(!ch)break;
            if(j==127)return fail("bone name too long");
            boneName+=ch>='A'&&ch<='Z'?char(ch-'A'+'a'):ch;
        }
        boneNames[i]=boneName;
        std::array<float,12> poseToBone{};
        if(reader.read(bone+96,poseToBone.data(),48) &&
           inverseBindPosition(poseToBone.data(),result.bindLocal[i]))
           result.bindValid[i]=1;
        // Names, not fixed bone indices; work across infected model skeletons.
        if(boneName.find("foot")!=std::string::npos &&
           boneName.find("attach")==std::string::npos)result.feet.push_back(i);
    }
    // Props without a pair of feet are supported without partial foot preservation.
    if(result.feet.size()<2)result.feet.clear();
    for(unsigned i=0;i<count;++i) {
        bool fixed=false;
        int parent=int(i);unsigned depth=0;
        for(;parent!=-1 && depth<count;++depth) {
            if(std::find(result.feet.begin(),result.feet.end(),unsigned(parent))!=result.feet.end())fixed=true;
            parent=parents[unsigned(parent)];
        }
        if(parent!=-1)return fail("cyclic bone parent hierarchy"); // cyclic hierarchy
        if(fixed)result.fixed.push_back(i);
    }
    result.parents=std::move(parents);
    result.boneNames=std::move(boneNames);
    out=std::move(result);return true;
}
template<std::size_t Capacity> inline void preserveFeet(const StudioPose& studio,const Matrix3x4* original,BasicPoseCopy<Capacity>& output) {
    for(auto i:studio.fixed)if(i<output.count)output.bones[i]=original[i];
}
template<std::size_t Capacity,class Reader> bool readDrawPose(Reader& reader,std::uint32_t address,
    const StudioPose& studio,BasicPoseCopy<Capacity>& out) {
    if(!studio.count || studio.count>Capacity)return false;
    if(!reader.read(address,out.bones.data(),studio.count*sizeof(Matrix3x4)))return false;
    out.count=studio.count;return true;
}
inline bool footAnchor(const StudioPose& studio,const Matrix3x4* pose,float& anchor) {
    if(!pose || studio.feet.size()<2)return false;
    float low=std::numeric_limits<float>::infinity();
    for(auto i:studio.feet) {
        if(i>=studio.count || !std::isfinite(pose[i].m[2][3]))return false;
        low=std::min(low,pose[i].m[2][3]);
    }
    anchor=low;return std::isfinite(anchor);
}
inline bool poseAnchor(const StudioPose& studio,const Matrix3x4* pose,float& anchor) {
    if(studio.feet.size()>=2)return footAnchor(studio,pose,anchor);
    if(!pose || !studio.count)return false;
    // Rigid props scale about their rendered root bone; no entity/collision writes.
    anchor=pose[0].m[2][3];return std::isfinite(anchor);
}
inline std::string canonicalModel(std::string name) {
    for(char& c:name) {if(c=='\\')c='/';if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
    if(name.compare(0,7,"models/")==0)name.erase(0,7);
    return name;
}
}
