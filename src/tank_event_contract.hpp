#pragma once
#include "dango/source_entity_contract.hpp"
#include <array>
#include <deque>
namespace visual::tank::events {
namespace wire=dango::entitywire;
// The registered callbacks use the public SDK cdecl ABI. No entity virtual
// methods are invoked. Factory must be a decoded static singleton getter.
struct Layout {uint32_t descriptor=0,factory=0,singleton=0,base=0;std::array<uint32_t,3> properties{},proxies{},offsets{};std::string reason="event descriptor unavailable";bool valid=false;};
struct ProxyData {uint32_t property=0;int32_t value=0;uint32_t padding[2]{};int32_t type=0,element=0,objectId=0;};
static_assert(sizeof(ProxyData)==28&&offsetof(ProxyData,type)==16);
template<class Reader,class Owned,class Code> Layout inspect(Reader& reader,uint32_t address,Owned owned,Code code){
 Layout out;out.descriptor=address;auto fail=[&](const char* why){out.reason=why;return out;};wire::ClientClass c;
 if(!owned(address)||!wire::load(reader,address,c)||wire::text(reader,c.name)!="CTEPlayerAnimEvent"||c.create||!code(c.event))return fail("event descriptor/factory ownership rejected");
 out.factory=c.event;auto singleton=wire::addressGetter(reader,c.event,0);if(!singleton||!owned(*singleton))return fail("event factory is not a verified static singleton getter");out.singleton=*singleton;
 uint32_t vt=0,cls=0,baseGetter=0;if(!wire::load(reader,*singleton,vt)||!owned(vt)||!wire::load(reader,vt+4,cls)||!code(cls)||wire::addressGetter(reader,cls,*singleton)!=std::optional<uint32_t>(address)||!wire::load(reader,vt+40,baseGetter)||!code(baseGetter))return fail("event singleton profiled class/base getters rejected");
 auto base=wire::addressGetter(reader,baseGetter,*singleton);if(!base||!owned(*base))return fail("event data base getter rejected");out.base=*base;
 wire::Table t;if(!wire::load(reader,c.table,t)||wire::text(reader,t.name)!="DT_TEPlayerAnimEvent"||t.count!=3||!t.props||uint64_t(t.props)+3*sizeof(wire::Prop)>0x100000000ull)return fail("event receive table shape rejected");
 std::array<bool,3> seen{};for(int i=0;i<3;++i){uint32_t a=t.props+uint32_t(i)*sizeof(wire::Prop);wire::Prop p;if(!wire::load(reader,a,p))return fail("event property unreadable");auto n=wire::text(reader,p.name);int k=n=="m_hPlayer"?0:n=="m_iEvent"?1:n=="m_nData"?2:-1;
  if(k<0||seen[size_t(k)]||p.type!=0||p.offset<0||p.offset>128||!code(p.proxy)||p.table||p.array||p.extra||p.offset%4)return fail("event property metadata rejected");
  seen[size_t(k)]=true;out.properties[size_t(k)]=a;out.proxies[size_t(k)]=p.proxy;out.offsets[size_t(k)]=uint32_t(p.offset);
 }
 if(out.offsets[0]!=16||out.offsets[1]!=20||out.offsets[2]!=24)return fail("event field layout differs from observed build");
 out.valid=true;out.reason="static factory and three scalar receive proxies verified; no virtual dispatch";return out;
}
struct Event {uint64_t id=0,tick=0;uint32_t epoch=0,base=0;std::array<int32_t,3> raw{},decoded{};unsigned mask=0;bool factory=false,complete=false;std::string boundary;};
class Recorder {
 Event current;uint64_t nextId=0;std::deque<Event> pending;uint64_t lost=0;
 void finish(const char* why){if(!current.id)return;current.boundary=why;current.complete=current.factory&&current.mask==7;if(pending.size()==512){pending.pop_front();++lost;}pending.push_back(current);current={};}
public:
 void begin(uint32_t epoch,uint64_t tick,uint32_t base){finish("next factory");current={};current.id=++nextId;current.epoch=epoch;current.tick=tick;current.base=base;current.factory=true;}
 void field(unsigned k,int32_t raw,int32_t decoded,uint32_t epoch,uint64_t tick,uint32_t base){
  if(k>=3)return;
  if(current.id&&(current.epoch!=epoch||current.base!=base||tick<current.tick||tick-current.tick>250||(current.mask&(1u<<k))))finish("field boundary or stale transaction");
  if(!current.id){current.id=++nextId;current.epoch=epoch;current.tick=tick;current.base=base;}
  current.raw[k]=raw;current.decoded[k]=decoded;current.mask|=1u<<k;if(current.mask==7)finish("all three fields received");
 }
 void flush(){finish("frame boundary");}
 void reset(){finish("capture disabled or map changed");}
 std::vector<Event> take(){std::vector<Event> out(pending.begin(),pending.end());pending.clear();return out;}
 uint64_t dropped()const{return lost;}
};
}
