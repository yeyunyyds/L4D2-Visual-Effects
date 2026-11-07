#pragma once
#include "tank_sequence.hpp"
namespace visual::tank {
// Passive x86 SDK layouts. Never treat studiohdr_t::virtualModel as a pointer:
// the engine can use that field as an opaque cache handle.
struct SequenceVector{std::uint32_t data=0,allocated=0,grow=0,count=0,elements=0;};
inline bool validVector(const SequenceVector& v,unsigned maximum,unsigned stride){return v.count<=maximum&&v.allocated>=v.count&&v.allocated<=16384&&v.grow<=4096&&v.data==v.elements&&(!v.count||v.data)&&uint64_t(v.data)+uint64_t(v.count)*stride<=0x100000000ull;}
struct VirtualSequence{int flags=0,activity=0,group=0,index=0;};
struct SharedName{SequenceName names;std::uint32_t header=0;int local=-1,group=-1;};
template<class Reader> std::optional<SharedName> virtualSequence(Reader& reader,std::uint32_t wrapper,std::uint32_t root,int sequence){using namespace dango::entitywire;
 if(!wrapper||sequence<0||sequence>=4096||uint64_t(wrapper)+28>0x100000000ull)return {};
 std::array<uint32_t,2> pointers{},after{};SequenceVector cache,seqs,groups;
 if(!reader.read(wrapper,pointers.data(),sizeof(pointers))||pointers[0]!=root||!pointers[1]||uint64_t(pointers[1])+108>0x100000000ull||!load(reader,wrapper+8,cache)||!validVector(cache,32,4)||!cache.count||!load(reader,pointers[1]+8,seqs)||!validVector(seqs,4096,16)||unsigned(sequence)>=seqs.count||!load(reader,pointers[1]+88,groups)||!validVector(groups,32,144)||groups.count!=cache.count)return {};
 uint32_t first=0;if(!load(reader,cache.data,first)||first!=root)return {};
 VirtualSequence entry;if(!load(reader,seqs.data+uint32_t(sequence)*16,entry)||entry.group<0||unsigned(entry.group)>=cache.count||entry.index<0||entry.index>=4096)return {};
 uint32_t header=0;if(!load(reader,cache.data+uint32_t(entry.group)*4,header))return {};auto key=modelKey(reader,header);if(!key||entry.index>=key->count)return {};
 auto name=sequenceName(reader,header,entry.index);if(!name)return {};
 std::array<unsigned char,144> groupBytes{};if(!reader.read(groups.data+uint32_t(entry.group)*144,groupBytes.data(),groupBytes.size()))return {};
 SequenceVector checkCache,checkSeq,checkGroups;VirtualSequence checkEntry;uint32_t checkHeader=0;
 if(!reader.read(wrapper,after.data(),sizeof(after))||after!=pointers||!load(reader,wrapper+8,checkCache)||std::memcmp(&cache,&checkCache,sizeof(cache))||!load(reader,pointers[1]+8,checkSeq)||std::memcmp(&seqs,&checkSeq,sizeof(seqs))||!load(reader,pointers[1]+88,checkGroups)||std::memcmp(&groups,&checkGroups,sizeof(groups))||!load(reader,seqs.data+uint32_t(sequence)*16,checkEntry)||std::memcmp(&entry,&checkEntry,sizeof(entry))||!load(reader,cache.data+uint32_t(entry.group)*4,checkHeader)||checkHeader!=header)return {};
 return SharedName{*name,header,entry.index,entry.group};
}
// Bounded, read-only discovery of a CStudioHdr pointer stored by the matched
// entity. Accept only a complete mapping cross-validated against the root MDL,
// virtual sequence/group vectors, group header cache and local descriptors.
// A failed candidate is never executed. The caller rate-limits retries.
template<class Reader> std::uint32_t findStudioWrapper(Reader& reader,std::uint32_t entity,std::uint32_t root,int sequence){using namespace dango::entitywire;if(!entity||uint64_t(entity)+16384>0x100000000ull)return 0;
 std::set<uint32_t> seen;unsigned budget=768;
 for(unsigned offset=0;offset<16384&&budget;offset+=256){std::array<uint32_t,64> words{};if(!reader.read(entity+offset,words.data(),sizeof(words)))continue;
  for(auto pointer:words){if(!budget)break;if(pointer<65536||pointer%4||uint64_t(pointer)+28>0x100000000ull||!seen.insert(pointer).second)continue;--budget;uint32_t first=0;
   if(load(reader,pointer,first)&&first==root&&virtualSequence(reader,pointer,root,sequence))return pointer;
  }
 }return 0;
}
}
