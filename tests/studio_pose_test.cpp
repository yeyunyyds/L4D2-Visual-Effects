#include "studio_pose.hpp"
#include "studio_cache.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace visual;
void check(bool condition){if(!condition)throw std::runtime_error("studio pose regression");}
struct Reader {
    std::vector<unsigned char> data=std::vector<unsigned char>(4096);
    unsigned reads=0;
    bool read(std::uint32_t address,void* out,std::size_t count) {
        ++reads;
        if(address<0x1000 || std::uint64_t(address-0x1000)+count>data.size())return false;
        std::memcpy(out,data.data()+address-0x1000,count);return true;
    }
    void integer(unsigned offset,int value){std::memcpy(data.data()+offset,&value,4);}
    Reader() {
        integer(0,0x54534449);integer(4,49);integer(76,4096);integer(156,5);integer(160,168);
        std::memcpy(data.data()+12,"infected/test.mdl",18);
        const char* names[]={"root","ValveBiped.Bip01_L_Foot","ValveBiped.Bip01_R_Foot","toe_L","body"};
        const int parents[]={-1,0,0,1,0};unsigned cursor=1400;
        for(unsigned i=0;i<5;++i){unsigned bone=168+i*216;integer(bone,int(cursor-bone));integer(bone+4,parents[i]);std::strcpy(reinterpret_cast<char*>(data.data()+cursor),names[i]);cursor+=unsigned(std::strlen(names[i]))+1;}
    }
};
int main(){
    Reader reader;StudioPose studio;
    check(studioPose(reader,0x1000,studio));
    check(studio.count==5 && studio.feet==std::vector<unsigned>({1,2}));
    check(studio.fixed==std::vector<unsigned>({1,2,3}));
    StudioCache cache;const char* reason="";
    check(cache.find(reader,0x1000,1,&reason)!=nullptr);unsigned before=reader.reads;
    check(cache.find(reader,0x1000,1,&reason)!=nullptr);check(reader.reads==before+1);
    before=reader.reads;reader.integer(8,42);check(cache.find(reader,0x1000,1,&reason)!=nullptr);check(reader.reads>before+1);
    before=reader.reads;check(cache.find(reader,0x1000,2,&reason)!=nullptr);check(reader.reads>before+1);
    reader.integer(4,99);check(cache.find(reader,0x1000,2,&reason)==nullptr);reader.integer(4,49);
    Matrix3x4 pose[5]{};
    for(unsigned i=0;i<5;++i){pose[i].m[0][0]=pose[i].m[1][1]=pose[i].m[2][2]=1;pose[i].m[2][3]=float(i*10);}
    pose[2].m[2][3]=25; // feet at different heights must both retain full poses
    float anchor{};check(footAnchor(studio,pose,anchor)&&anchor==10);
    for(unsigned tick=0;tick<=2000;++tick){
        PoseCopy copy;check(stretchCopy(pose,5,anchor,scaleAt(tick),copy));
        preserveFeet(studio,pose,copy);
        for(unsigned i:{1u,2u,3u})check(std::memcmp(&copy.bones[i],&pose[i],sizeof(Matrix3x4))==0);
        check(std::abs(copy.bones[4].m[2][3]-(anchor+(40-anchor)*scaleAt(tick)))<0.0001f);
    }
    check(canonicalModel("Models\\Infected/Test.mdl")==canonicalModel(studio.name));
    Reader bad;bad.integer(168+4,1);check(!studioPose(bad,0x1000,studio)); // cyclic tree
    bad=Reader{};bad.integer(156,257);check(!studioPose(bad,0x1000,studio));
    bad=Reader{};bad.integer(160,4090);check(!studioPose(bad,0x1000,studio));
    bad=Reader{};bad.integer(168,-1);check(!studioPose(bad,0x1000,studio));
    bad=Reader{};bad.integer(4,99);check(!studioPose(bad,0x1000,studio));
    bad=Reader{};std::strcpy(reinterpret_cast<char*>(bad.data.data()+1405),"left_joint");check(!studioPose(bad,0x1000,studio));
    std::cout<<"PASS: bounded studio metadata, model identity, per-model foot lookup, both feet/toes fixed across full cycle, malformed hierarchy rejection\n";
}
