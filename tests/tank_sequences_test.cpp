#include "studio_profiles.hpp"
#include "tank_sequence_catalog.hpp"
#include "tank_layer_attack.hpp"
#include "tank_observed_timer.hpp"
#include <iostream>
#include <stdexcept>
struct Memory {
 std::vector<unsigned char> bytes=std::vector<unsigned char>(0x90000);
 bool read(uint32_t p,void* out,size_t n){if(!p||uint64_t(p)+n>bytes.size())return false;std::memcpy(out,bytes.data()+p,n);return true;}
 template<class T> void put(uint32_t p,T v){std::memcpy(bytes.data()+p,&v,sizeof(v));}
 void text(uint32_t p,const char* v){std::memcpy(bytes.data()+p,v,std::strlen(v)+1);}
};
void check(bool v){if(!v)throw std::runtime_error("sequence catalog/profile regression");}
int main(){using namespace visual::tank;Memory m;
 for(int id:{33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}){auto name=knownTankSequence(id);check(name&&classify(*name)=="attack-animation");}
 for(int id:{48,49,50,51}){auto name=knownTankSequence(id);check(name&&classify(*name)=="throw-animation");LayerAttackDetector throwing;check(!throwing.update(1.f,true,{{0,id,.05f,1.f,name}}).onset);SwingStopwatch timer;auto reading=timer.update(1.f,1,id,.05f,name,1.5f,1.f,false,false,.5f);check(reading.throwing&&!reading.triggered&&!reading.remaining);reading=timer.update(1.1f,2,1,.1f,knownTankSequence(1),1.5f,1.f,false,false,.5f);check(!reading.throwing&&reading.sinceThrow==0.f);}
 check(!knownTankSequence(-1)&&!knownTankSequence(80));for(int id:{0,1,2,3,4,14,52,65,78,79})check(classify(*knownTankSequence(id))=="other-animation");for(int id:{6,7,8,9,10,12,13})check(classify(*knownTankSequence(id))=="movement-animation");
 LayerAttackDetector detector;LayerSample overlay{0,43,.05f,1.f,knownTankSequence(43)};check(detector.update(1.f,true,{overlay}).onset);check(!detector.update(1.f,true,{overlay}).onset);overlay.cycle=.8f;check(!detector.update(1.1f,true,{overlay}).onset);overlay.cycle=.05f;check(detector.update(1.2f,true,{overlay}).onset);
 uint32_t base=0x50000,nt=base+128;m.put(base,uint16_t(0x5a4d));m.put(base+60,128u);m.put(nt,0x4550u);m.put(nt+4,uint16_t(0x14c));m.put(nt+24,uint16_t(0x10b));
 // Check explicit identities, not their ordering in the supported profile array.
 struct Expected {uint32_t timestamp,size;const char* sha;};
 const Expected expected[]={
  {1794274100u,0x453000u,"a7efe8304d0e0bb5cfa87193d234670ea59ead1323d43a10da60b2fbd6f65d14"},
  {1782844886u,0x453000u,"3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85"},
  {1782844886u,0x653000u,"03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6"}
 };
 for(const auto& e:expected){m.put(nt+8,e.timestamp);m.put(nt+80,e.size);auto profile=visual::studioabi::select(m,base);check(profile&&std::strcmp(profile->sha256,e.sha)==0);}
 m.put(nt+8,1794274100u);m.put(nt+80,0x653000u);check(!visual::studioabi::select(m,base)); // unverified new ThinFlex
 m.put(nt+80,0x453000u);m.put(nt+8,1794274101u);check(!visual::studioabi::select(m,base));
 m.put(nt+8,1794274100u);m.put(nt+80,0x553000u);check(!visual::studioabi::select(m,base));
 m.put(nt+80,0x453000u);m.put(nt+4,uint16_t(0x8664));check(!visual::studioabi::select(m,base));
 std::cout<<"PASS: all known Tank sequences, throw/punch separation, deduplication and all three Studio PE profiles and unknown-build rejection\n";
}
