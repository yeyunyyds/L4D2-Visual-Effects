#include "tank_event_contract.hpp"
#include <stdexcept>
#include <iostream>
void check(bool b){if(!b)throw std::runtime_error("Tank event regression");}
struct Memory{std::vector<unsigned char> data=std::vector<unsigned char>(8192);bool read(uint32_t p,void* out,size_t n){if(!p||uint64_t(p)+n>data.size())return false;std::memcpy(out,data.data()+p,n);return true;}template<class T>void put(uint32_t p,T v){std::memcpy(data.data()+p,&v,sizeof(v));}void text(uint32_t p,const char* s){std::memcpy(data.data()+p,s,std::strlen(s)+1);}void getter(uint32_t p,uint32_t value){data[p]=0xb8;put(p+1,value);data[p+5]=0xc3;}};
int main(){using namespace visual::tank::events;Memory m;wire::ClientClass c;c.name=200;c.table=300;c.event=400;m.text(200,"CTEPlayerAnimEvent");m.put(100,c);m.getter(400,500);m.put(500,600u);m.put(604,700u);m.getter(700,100);m.put(640,720u);m.getter(720,500);wire::Table t;t.props=1000;t.count=3;t.name=800;m.text(800,"DT_TEPlayerAnimEvent");m.put(300,t);
 const char* names[]={"m_hPlayer","m_iEvent","m_nData"};for(unsigned i=0;i<3;++i){wire::Prop p;p.name=2000+i*32;m.text(p.name,names[i]);p.type=0;p.offset=16+int(i)*4;p.proxy=3000+i*32;m.put(1000+i*sizeof(p),p);}
 auto owned=[](uint32_t a){return a>0&&a<8192;};auto layout=inspect(m,100,owned,owned);check(layout.valid&&layout.base==500&&layout.offsets[1]==20);m.data[400]=0xe9;check(!inspect(m,100,owned,owned).valid);m.getter(400,500);wire::Prop p;m.read(1060,&p,sizeof(p));p.offset=24;m.put(1060,p);check(!inspect(m,100,owned,owned).valid);
 p.offset=20;m.put(1060,p);check(inspect(m,100,owned,owned).valid);p.type=1;m.put(1060,p);check(!inspect(m,100,owned,owned).valid);p.type=0;m.put(1060,p);m.getter(700,104);check(!inspect(m,100,owned,owned).valid);m.getter(700,100);c.create=800;m.put(100,c);check(!inspect(m,100,owned,owned).valid);c.create=0;m.put(100,c);check(!inspect(m,100,owned,[](uint32_t a){return a>0&&a<3000;}).valid);
 Recorder r;r.begin(1,100,500);r.field(2,8,8,1,100,500);r.field(0,17,4097,1,100,500);r.field(1,33,33,1,100,500);auto out=r.take();check(out.size()==1&&out[0].complete&&out[0].decoded[0]==4097&&out[0].decoded[1]==33);
 r.begin(1,101,500);r.field(1,33,33,1,101,500);r.begin(1,102,500);r.field(1,33,33,1,102,500);r.flush();out=r.take();check(out.size()==2&&!out[0].complete&&!out[1].complete&&out[0].id!=out[1].id);
 r.field(0,0,0,1,104,500);r.field(1,34,34,1,104,500);r.field(2,0,0,1,104,500);out=r.take();check(out.size()==1&&!out[0].complete&&!out[0].factory);
 r.begin(1,105,500);r.field(0,0,0,2,106,500);r.flush();out=r.take();check(out.size()==2&&!out[1].complete);
 for(int i=0;i<600;++i){r.begin(2,200+i,500);r.flush();}check(r.take().size()==512&&r.dropped()==88);
 std::cout<<"PASS: event descriptor/getter validation, three-field transactions, repeated events, orphan updates, epochs and bounded overflow\n";
}
