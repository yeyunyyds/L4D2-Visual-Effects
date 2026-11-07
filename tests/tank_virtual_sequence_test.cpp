#include "tank_virtual_sequence.hpp"
#include "studio_profiles.hpp"
#include "tank_sequence_fallback.hpp"
#include "tank_layer_attack.hpp"
#include <iostream>
#include <stdexcept>
struct Memory {
 std::vector<unsigned char> bytes=std::vector<unsigned char>(0x90000);
 bool read(uint32_t p,void* out,size_t n){if(!p||uint64_t(p)+n>bytes.size())return false;std::memcpy(out,bytes.data()+p,n);return true;}
 template<class T> void put(uint32_t p,T v){std::memcpy(bytes.data()+p,&v,sizeof(v));}
 void text(uint32_t p,const char* v){std::memcpy(bytes.data()+p,v,std::strlen(v)+1);}
};
void check(bool v){if(!v)throw std::runtime_error("virtual sequence/profile regression");}
void model(Memory& m,uint32_t h,const char* name){m.put(h,0x54534449);m.put(h+4,49);m.put(h+8,123);m.text(h+12,name);m.put(h+76,4096);m.put(h+188,2);m.put(h+192,512);
 for(int i=0;i<2;++i){auto d=h+512+212*i;m.put(d,-(512+212*i));m.put(d+4,int(h+1500+128*i-d));m.put(d+8,int(h+1800+128*i-d));m.text(h+1500+128*i,"mod_custom_name");m.text(h+1800+128*i,"ACT_MELEE_ATTACK1");}}
int main(){using namespace visual::tank;Memory m;uint32_t root=0x10000,group=0x12000,wrapper=0x20000,vm=0x21000,cache=0x22000,seq=0x23000,groups=0x25000,entity=0x30000;
 model(m,root,"original_tank.mdl");model(m,group,"shared_animation.mdl");m.put(root+336,1);
 m.put(wrapper,root);m.put(wrapper+4,vm);m.put(wrapper+8,SequenceVector{cache,2,0,2,cache});m.put(cache,root);m.put(cache+4,group);
 m.put(vm+8,SequenceVector{seq,81,0,81,seq});m.put(vm+88,SequenceVector{groups,2,0,2,groups});m.put(seq+80*16,VirtualSequence{0,0,1,1});m.put(entity+0x500,wrapper);
 auto n=virtualSequence(m,wrapper,root,80);check(n&&n->header==group&&n->local==1&&classify(n->names)=="attack-animation");check(findStudioWrapper(m,entity,root,80)==wrapper);
 // The SDK lazily fills headers: group zero can remain NULL forever.
 m.put(cache,0u);check(virtualSequence(m,wrapper,root,80).has_value());WrapperProbe probe;check(findStudioWrapper(m,entity,root,80,&probe)==wrapper&&probe.offset==0x500&&probe.rootMatches==1);
 m.put(cache+4,0u);const char* reason=nullptr;check(!virtualSequence(m,wrapper,root,80,&reason));check(std::string(reason)=="selected shared header not yet cached by game");check(findStudioWrapper(m,entity,root,80)==wrapper);
 m.put(seq,VirtualSequence{0,0,0,0});check(virtualSequence(m,wrapper,root,0).has_value());m.put(cache+4,group);
 m.put(cache,group);check(!studioWrapper(m,wrapper,root));m.put(cache,0u);
 auto key=modelKey(m,root);check(key&&key->includes==1&&key->version==49);m.put(root+8,124);check(!(modelKey(m,root).value()==*key));
 check(classify({"Attack", "ACT_IDLE"})=="other-animation");check(classify({"arbitrary", "ACT_TERROR_ATTACK_MOVING"})=="attack-animation");check(classify({"Hulk_RunAttack1", ""})=="attack-animation");check(classify({"idle_attack", ""})=="other-animation");check(classify({"Attack", "ACT_RANGE_ATTACK1"})=="throw-animation");
 m.put(seq+80*16,VirtualSequence{0,0,2,1});check(!virtualSequence(m,wrapper,root,80));m.put(seq+80*16,VirtualSequence{0,0,1,2});check(!virtualSequence(m,wrapper,root,80));m.put(seq+80*16,VirtualSequence{0,0,1,1});m.put(cache+4,root);check(!virtualSequence(m,wrapper,group,80));m.put(cache+4,group);
 m.put(wrapper+8,SequenceVector{cache,1,0,2,cache});check(!virtualSequence(m,wrapper,root,80));m.put(wrapper+8,SequenceVector{cache,2,0,2,cache+4});check(!virtualSequence(m,wrapper,root,80));m.put(wrapper+8,SequenceVector{cache,2,0,2,cache});m.put(vm+88,SequenceVector{groups,1,0,1,groups});check(!virtualSequence(m,wrapper,root,80));m.put(vm+88,SequenceVector{groups,2,0,2,groups});m.put(group+512+212,0);check(!virtualSequence(m,wrapper,root,80));check(findStudioWrapper(m,entity,root,80)==wrapper);
 ModelKey vanilla;vanilla.name="infected/hulk.mdl";vanilla.checksum=1500020281;vanilla.version=49;vanilla.count=1;vanilla.includes=1;
 for(int id:{40,43,45}){auto backup=originalLayerBackup(vanilla,id);check(backup&&classify(*backup)=="attack-animation");}check(!originalLayerBackup(vanilla,9));
 LayerAttackDetector detector;LayerSample overlay{0,43,.05f,1.f,originalLayerBackup(vanilla,43)};check(detector.update(1.f,true,{overlay}).onset);check(!detector.update(1.f,true,{overlay}).onset);overlay.cycle=.8f;check(!detector.update(1.1f,true,{overlay}).onset);overlay.cycle=.05f;check(detector.update(1.2f,true,{overlay}).onset);
 vanilla.checksum=795675474;check(!originalLayerBackup(vanilla,43));vanilla.checksum=1500020281;vanilla.count=80;check(!originalLayerBackup(vanilla,43));
 uint32_t base=0x50000,nt=base+128;m.put(base,uint16_t(0x5a4d));m.put(base+60,128u);m.put(nt,0x4550u);m.put(nt+4,uint16_t(0x14c));m.put(nt+24,uint16_t(0x10b));m.put(nt+8,1782844886u);m.put(nt+80,0x453000u);check(visual::studioabi::select(m,base)==&visual::studioabi::supported[0]);m.put(nt+80,0x653000u);check(visual::studioabi::select(m,base)==&visual::studioabi::supported[1]);m.put(nt+80,0x553000u);check(!visual::studioabi::select(m,base));m.put(nt+80,0x453000u);m.put(nt+4,uint16_t(0x8664));check(!visual::studioabi::select(m,base));
 std::cout<<"PASS: shared model mappings, mod semantic activities, invalid vector/group/descriptor rejection and both Studio PE profiles\n";
}
