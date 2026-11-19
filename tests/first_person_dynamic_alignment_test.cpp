#include "first_person_dynamic_alignment.hpp"
#include "dango/json.hpp"
#include <cassert>
#include <cmath>
#include <limits>
using namespace visual;
using namespace visual::firstperson;
using namespace visual::firstperson::dynamic;
static Matrix3x4 matrix(float x,float y,float z){
 Matrix3x4 m{};
 for(unsigned i=0;i<3;++i)m.m[i][i]=1;
 m.m[0][3]=x;m.m[1][3]=y;m.m[2][3]=z;return m;
}
int main(){
 float inverse[12]={1,0,0,0,0,1,0,0,0,0,1,-62};
 std::array<float,3> at{};
 assert(inverseBindPosition(inverse,at)&&at[2]==62);
 inverse[0]=0;inverse[5]=0;inverse[10]=0;
 assert(!inverseBindPosition(inverse,at));
 StudioPose rig{};
 rig.count=7;rig.name="models/survivors/survivor_coach.mdl";rig.checksum=678;
 rig.parents={-1,0,1,2,3,2,2};
 rig.boneNames={"root","bip_spine","bip_spine4","bip_neck1","bip_head1",
                "bip_l_clavicle","bip_r_clavicle"};
 rig.bindValid=std::vector<unsigned char>(7,1);
 rig.bindLocal=std::vector<std::array<float,3>>(7);
 rig.bindLocal[2]={0,0,50};
 rig.bindLocal[3]={0,0,58};rig.bindLocal[4]={0,0,64};
 rig.bindLocal[5]={0,9,53};rig.bindLocal[6]={0,-9,53};
 rig.eyeLocal={0,0,65};rig.eyeValid=true;
 auto anchor=discover(rig,5);
 assert(anchor.anchor==2&&anchor.bindEye&&anchor.knownEye);
 assert(anchor.eyeFromAnchor.z==15);
 Matrix3x4 bones[7]{};
 for(int i=0;i<7;++i)bones[i]=matrix(0,0,float(i*8));
 bones[2]=matrix(0,0,50);bones[5]=matrix(0,9,53);bones[6]=matrix(0,-9,53);
 dango::CameraPose camera{};
 camera.eye={0,0,65};camera.forward={1,0,0};
 camera.right={0,-1,0};camera.up={0,0,1};
 Settings options{};options.dynamicAnchor=true;
 // The alignment math tests use a neutral calibration, not the live visual preset.
 options.forward=options.right=options.up=options.yawTrim=0.f;
 Track track;
 auto r=compute(rig,anchor,bones,camera,options,nullptr,1,123,1000,track);
 assert(r.ok&&std::abs(r.translation.z)<.001f);
 // Jump/landing: camera rises 10u but animated chest rises 24u.
 camera.eye.z=75;bones[2].m[2][3]=74;
 auto jump=compute(rig,anchor,bones,camera,options,nullptr,1,123,1016,track);
 assert(jump.ok&&std::abs(jump.translation.z+14)<.001f);
 auto next=compute(rig,anchor,bones,camera,options,nullptr,1,123,1016,track);
 assert(next.ok&&std::abs(next.translation.z+14)<.001f);
 // Transforms affect ONLY render copies, do not mutate original bones.
 auto original=bones[2];Matrix3x4 copy[7]{};
 for(unsigned i=0;i<7;++i)copy[i]=bones[i];
 assert(transform(copy,7,location(bones[2]),jump.translation,0.f));
 assert(std::abs(copy[2].m[2][3]-60.f)<.001f);
 assert(bones[2].m[2][3]==original.m[2][3]);
 // Reject dangerous/implausible tracking deltas without altering bones.
 camera.eye.z=300;
 auto invalid=compute(rig,anchor,bones,camera,options,nullptr,1,123,1032,track);
 assert(!invalid.ok);
 camera.eye.z=75;
 auto restored=compute(rig,anchor,bones,camera,options,nullptr,2,123,1048,track);
 assert(restored.ok);assert(track.epoch==2);
 // Missing/invalid compiled eye -> inverse bind fallback to head.
 rig.eyeLocal={0,0,200};
 auto fallback=discover(rig,5);
 assert(fallback.knownEye&&!fallback.bindEye);
 assert(fallback.eyeFromAnchor.z==19);
 // Full inverse bind missing: bootstrap once, then track independent motion.
 rig.bindValid.assign(7,0);
 auto boot=discover(rig,5);
 assert(boot.anchor==2&&!boot.knownEye);
 Track bootstrap;
 camera.eye.z=65;bones[2].m[2][3]=50;
 auto first=compute(rig,boot,bones,camera,options,nullptr,1,42,1000,bootstrap);
 assert(first.ok&&first.usedBootstrap&&std::abs(first.translation.z)<.01f);
 camera.eye.z=75;bones[2].m[2][3]=74;
 auto again=compute(rig,boot,bones,camera,options,nullptr,1,42,1016,bootstrap);
 assert(again.ok&&std::abs(again.translation.z+14)<.01f);
 // Model-specific calibration survives save/load and is checksum-isolated.
 Settings saved{};saved.dynamicAnchor=true;saved.forward=3;saved.right=-2;saved.up=4;saved.yawTrim=5;
 const auto key=canonicalModel(rig.name);
 assert(storeForModel(saved,key,rig.checksum));
 assert(saved.forward==0&&saved.up==0&&saved.yawTrim==0);
 auto profileA=profile(saved,key,rig.checksum);
 assert(profileA&&profileA->forward==3&&profileA->up==4&&profileA->yaw==5);
 assert(!profile(saved,key,rig.checksum+1));
 nlohmann::json j=nlohmann::json::object();writeSettings(j,saved);
 auto reloaded=readSettings(j);
 assert(reloaded.modelProfiles.size()==1&&reloaded.dynamicAnchor);
 assert(profile(reloaded,key,rig.checksum)->right==-2);
 assert(eraseForModel(reloaded,key,rig.checksum)&&!eraseForModel(reloaded,key,rig.checksum));
 // Recover from pathological non-finite poses and clamp rotation correction.
 Matrix3x4 bad[7]{};
 for(int i=0;i<7;++i)bad[i]=bones[i];
 bad[2].m[0][3]=std::numeric_limits<float>::quiet_NaN();
 assert(!transform(bad,7,{0,0,0},{0,0,0},0));
}
