#include "dango/animation_layers.hpp"
#include <iostream>
#include <stdexcept>
struct Memory{std::vector<unsigned char> bytes=std::vector<unsigned char>(16384);bool read(uint32_t a,void* out,size_t n){if(!a||uint64_t(a)+n>bytes.size())return false;std::memcpy(out,bytes.data()+a,n);return true;}template<class T>void put(unsigned a,T v){std::memcpy(bytes.data()+a,&v,sizeof(v));}void text(unsigned a,const char* s){std::memcpy(bytes.data()+a,s,std::strlen(s)+1);}};
void check(bool ok){if(!ok)throw std::runtime_error("animation layer regression");}
int main(){using namespace dango::entitywire;using namespace dango::animation;Memory m;
 Table root;root.props=1000;root.count=1;root.name=700;m.text(700,"DT_Test");m.put(100,root);
 Prop vector;vector.name=730;m.text(730,"m_AnimOverlay");vector.type=6;vector.table=200;m.put(1000,vector);
 Table array;array.name=760;m.text(760,"_ST_m_AnimOverlay_2");array.props=1100;array.count=3;m.put(200,array);
 Prop length;length.name=800;m.text(800,"lengthproxy");length.type=6;length.extra=2000;m.put(1100,length);
 std::array<uint32_t,7> extra{50,0,60,70,32,256,2};m.put(2000,extra);
 Prop element;element.type=6;element.extra=2000;element.tableProxy=80;element.table=300;m.put(1160,element);element.stride=1;m.put(1220,element);
 Table layer;layer.name=830;m.text(830,"DT_Animationlayer");layer.props=1300;layer.count=5;m.put(300,layer);
 const char* names[]={"m_nSequence","m_nOrder","m_flCycle","m_flPrevCycle","m_flWeight"};for(int i=0;i<5;++i){Prop field;field.name=2500+unsigned(i)*32;m.text(field.name,names[i]);field.type=i<2?0:1;field.offset=i*4;m.put(1300+unsigned(i)*60,field);}
 auto layout=animationLayerLayout(m,100);check(layout.valid&&layout.offset==256&&layout.stride==32&&layout.maximum==2);
 std::array<uint32_t,5> vec{5000,2,0,2,5000};m.put(4096+256,vec);for(int i=0;i<2;++i){unsigned a=5000+unsigned(i)*32;m.put(a,40+i);m.put(a+4,i);m.put(a+8,.3f);m.put(a+12,.1f);m.put(a+16,i==0?1.f:0.f);}
 std::string reason;auto layers=animationLayers(m,4096,layout,reason);check(layers.size()==2&&layers[0].sequence==40&&layers[0].cycle==.3f&&layers[1].weight==0);
 vec[3]=3;m.put(4352,vec);check(animationLayers(m,4096,layout,reason).empty());vec[3]=2;vec[4]=6000;m.put(4352,vec);check(animationLayers(m,4096,layout,reason).empty());vec[4]=5000;m.put(4352,vec);m.put(5016,2.f);check(animationLayers(m,4096,layout,reason).empty());m.put(5016,1.f);
 extra[4]=8;m.put(2000,extra);check(!animationLayerLayout(m,100).valid);extra[4]=32;m.put(2000,extra);element.extra=2100;m.put(1220,element);check(!animationLayerLayout(m,100).valid);
 check(animationLayers(m,0xfffffff0u,layout,reason).empty());
 element.extra=2000;m.put(1220,element);Prop bad;m.read(1300,&bad,sizeof(bad));bad.offset=0x7fffffff;m.put(1300,bad);check(!animationLayerLayout(m,100).valid);
 std::cout<<"PASS: SDK overlay metadata, dynamic stride, all layers, invalid vector/weight/storage/metadata rejection\n";
}
