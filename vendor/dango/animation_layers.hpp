#pragma once
#include "source_entity_contract.hpp"
#include <array>
#include <cmath>
namespace dango::animation {
inline constexpr const char* SampledReason="sampled read-only; network values, not an event callback";
// SDK l4d2: dt_utlvector_recv.cpp CRecvPropExtra_UtlVector. Metadata is
// inspected only. No receive proxies, resize functions or entity methods run.
struct LayerLayout {
 std::uint32_t offset=0,stride=0,maximum=0;
 dango::entitywire::Fields fields;
 std::string reason="overlay metadata unavailable";
 bool valid=false;
};
struct AnimationLayer {int index=0,sequence=0,order=0;float cycle=0,previousCycle=0,weight=0;};
template<class Reader> LayerLayout animationLayerLayout(Reader& reader,std::uint32_t root){
 using namespace dango::entitywire;LayerLayout result;std::set<uint32_t> seen;
 struct Node{uint32_t table,base;};std::vector<Node> pending{{root,0}};
 for(size_t i=0;i<pending.size()&&i<20;++i){auto node=pending[i];if(!seen.insert(node.table).second)continue;Table t;
  if(!load(reader,node.table,t)||t.count<0||t.count>2048||(t.count&&!t.props)||uint64_t(t.props)+uint64_t(t.count)*sizeof(Prop)>0x100000000ull)continue;
  for(int k=0;k<t.count;++k){Prop p;if(!load(reader,t.props+uint32_t(k)*sizeof(p),p))break;auto name=text(reader,p.name);
   if(p.type!=6||p.offset<0||uint64_t(node.base)+p.offset>65536)continue;
   if(name=="baseclass"||name=="overlay_vars"){if(pending.size()<20)pending.push_back({p.table,node.base+uint32_t(p.offset)});continue;}
   if(name!="m_AnimOverlay")continue;
   if(p.offset!=0){result.reason="non-SDK overlay vector property offset rejected";return result;}
   result.reason="overlay vector metadata shape rejected";
   Table a;if(!load(reader,p.table,a)||a.count<2||a.count>33||!a.props||uint64_t(a.props)+uint64_t(a.count)*sizeof(Prop)>0x100000000ull)return result;
   Prop length;if(!load(reader,a.props,length)||text(reader,length.name)!="lengthproxy"||length.type!=6||length.offset!=0||!length.extra)return result;
   std::array<uint32_t,7> extra{};if(!reader.read(length.extra,extra.data(),sizeof(extra)))return result;
   int stride=int32_t(extra[4]),offset=int32_t(extra[5]),maximum=int32_t(extra[6]);
   if(!extra[0]||!extra[2]||!extra[3]||stride<20||stride>512||offset<0||uint64_t(node.base)+p.offset+offset+20>65536||maximum<1||maximum>32||a.count!=maximum+1)return result;
   Prop first;for(int j=0;j<maximum;++j){Prop element;
    if(!load(reader,a.props+uint32_t(j+1)*sizeof(Prop),element)||element.type!=6||element.offset!=0||element.extra!=length.extra||element.stride!=j||!element.tableProxy)return result;
    if(j==0)first=element;else if(element.table!=first.table||element.tableProxy!=first.tableProxy)return result;
   }
   Table layer;if(!load(reader,first.table,layer)||text(reader,layer.name)!="DT_Animationlayer"||layer.count<5||layer.count>32||!layer.props||uint64_t(layer.props)+uint64_t(layer.count)*sizeof(Prop)>0x100000000ull)return result;
   Fields fields;for(int j=0;j<layer.count;++j){Prop value;if(!load(reader,layer.props+uint32_t(j)*sizeof(Prop),value))return result;auto n=text(reader,value.name);
    if(n!="m_nSequence"&&n!="m_nOrder"&&n!="m_flCycle"&&n!="m_flPrevCycle"&&n!="m_flWeight")continue;
    int expected=(n=="m_nSequence"||n=="m_nOrder")?0:1;
    if(value.type!=expected||value.offset<0||uint64_t(value.offset)+4>uint64_t(stride)||!fields.emplace(n,Field{uint32_t(value.offset),expected}).second)return result;
   }
   if(fields.size()!=5)return result;
   std::set<uint32_t> occupied;for(const auto& f:fields)for(unsigned b=0;b<4;++b)if(!occupied.insert(f.second.offset+b).second)return result;
   result.offset=node.base+uint32_t(p.offset)+uint32_t(offset);result.stride=uint32_t(stride);result.maximum=uint32_t(maximum);result.fields=std::move(fields);result.valid=true;result.reason="SDK UtlVector metadata validated; read-only";return result;
  }
 }return result;
}
template<class Reader> std::vector<AnimationLayer> animationLayers(Reader& reader,std::uint32_t entity,const LayerLayout& layout,std::string& reason){
 using namespace dango::entitywire;std::vector<AnimationLayer> result;reason=layout.reason;if(!layout.valid)return result;
 // x86 CUtlVector: memory, allocation count, grow size, count, debug elements.
 std::array<uint32_t,5> before{},after{};uint64_t address=uint64_t(entity)+layout.offset;
 if(address+sizeof(before)>0x100000000ull||!reader.read(uint32_t(address),before.data(),sizeof(before))){reason="overlay vector unreadable";return {};}
 auto fail=[&](const char* s){reason=s;return std::vector<AnimationLayer>{};};
 if(before[3]>layout.maximum||before[1]<before[3]||before[1]>4096||int32_t(before[2])<0||before[2]>4096||before[0]!=before[4])return fail("overlay vector invariants rejected");
 if(before[3]&&(!before[0]||uint64_t(before[0])+uint64_t(before[3])*layout.stride>0x100000000ull))return fail("overlay storage bounds rejected");
 for(uint32_t i=0;i<before[3];++i){AnimationLayer layer;layer.index=int(i);auto base=before[0]+i*layout.stride;
  auto read=[&](const char* name,auto& value){auto f=layout.fields.find(name);return f!=layout.fields.end()&&load(reader,base+f->second.offset,value);};
  if(!read("m_nSequence",layer.sequence)||!read("m_nOrder",layer.order)||!read("m_flCycle",layer.cycle)||!read("m_flPrevCycle",layer.previousCycle)||!read("m_flWeight",layer.weight))return fail("overlay element unreadable");
  if(layer.sequence<0||layer.sequence>4095||layer.order<0||layer.order>32||!std::isfinite(layer.cycle)||layer.cycle<0||layer.cycle>1||!std::isfinite(layer.previousCycle)||layer.previousCycle<0||layer.previousCycle>1||!std::isfinite(layer.weight)||layer.weight<0||layer.weight>1)return fail("overlay element values rejected");
  result.push_back(layer);
 }
 if(!reader.read(uint32_t(address),after.data(),sizeof(after))||before!=after)return fail("overlay vector changed during sample");
 reason=SampledReason;return result;
}
}
