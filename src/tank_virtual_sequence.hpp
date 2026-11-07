#pragma once
#include "tank_sequence.hpp"
namespace visual::tank {
// Passive x86 SDK layouts; studiohdr_t::virtualModel is an opaque cache handle.
struct SequenceVector{std::uint32_t data=0,allocated=0,grow=0,count=0,elements=0;};
inline bool validVector(const SequenceVector& v,unsigned maximum,unsigned stride){return v.count<=maximum&&v.allocated>=v.count&&v.allocated<=16384&&v.grow<=4096&&v.data==v.elements&&(!v.count||v.data)&&uint64_t(v.data)+uint64_t(v.count)*stride<=0x100000000ull;}
struct VirtualSequence{int flags=0,activity=0,group=0,index=0;};
struct SharedName{SequenceName names;std::uint32_t header=0;int local=-1,group=-1;};
struct WrapperView{std::uint32_t root=0,model=0;SequenceVector cache,seqs,groups;};
struct WrapperProbe{unsigned scanned=0,candidates=0,rootMatches=0,offset=0;std::string reason="not probed";};
template<class Reader> std::optional<WrapperView> studioWrapper(Reader& reader,std::uint32_t wrapper,std::uint32_t root,const char** reason=nullptr){using namespace dango::entitywire;
 auto fail=[&](const char* why)->std::optional<WrapperView>{if(reason)*reason=why;return {};};
 if(!wrapper||uint64_t(wrapper)+28>0x100000000ull)return fail("wrapper address rejected");
 WrapperView v;if(!load(reader,wrapper,v.root)||v.root!=root)return fail("root header mismatch");
 if(!load(reader,wrapper+4,v.model)||!v.model||uint64_t(v.model)+108>0x100000000ull)return fail("virtual model pointer unavailable");
 if(!load(reader,wrapper+8,v.cache)||!validVector(v.cache,32,4)||!v.cache.count)return fail("header cache vector rejected");
 if(!load(reader,v.model+8,v.seqs)||!validVector(v.seqs,4096,16)||!v.seqs.count)return fail("virtual sequence vector rejected");
 if(!load(reader,v.model+88,v.groups)||!validVector(v.groups,32,144)||v.groups.count!=v.cache.count)return fail("virtual group vector rejected");
 // ResetVModel initializes ALL cache slots to NULL. Group zero need not have
 // been visited. The wrapper's own root pointer establishes root identity.
 uint32_t first=0;if(!load(reader,v.cache.data,first)||(first&&first!=root))return fail("root cache identity rejected");
 if(reason)*reason="wrapper located; lazy header cache accepted";
 return v;
}
template<class Reader> std::optional<SharedName> virtualSequence(Reader& reader,std::uint32_t wrapper,std::uint32_t root,int sequence,const char** reason=nullptr){using namespace dango::entitywire;
 auto fail=[&](const char* why)->std::optional<SharedName>{if(reason)*reason=why;return {};};
 auto view=studioWrapper(reader,wrapper,root,reason);if(!view)return {};auto& v=*view;
 if(sequence<0||unsigned(sequence)>=v.seqs.count)return fail("global sequence outside virtual table");
 VirtualSequence entry;if(!load(reader,v.seqs.data+uint32_t(sequence)*16,entry)||entry.group<0||unsigned(entry.group)>=v.cache.count||entry.index<0||entry.index>=4096)return fail("global sequence group/index rejected");
 uint32_t cached=0;if(!load(reader,v.cache.data+uint32_t(entry.group)*4,cached))return fail("group header cache unreadable");
 uint32_t header=cached;if(!header&&entry.group==0)header=root;
 if(!header)return fail("selected shared header not yet cached by game");
 auto key=modelKey(reader,header);if(!key||entry.index>=key->count)return fail("selected local model/index rejected");
 auto name=sequenceName(reader,header,entry.index);if(!name)return fail("selected local descriptor rejected");
 std::array<unsigned char,144> groupBytes{};if(!reader.read(v.groups.data+uint32_t(entry.group)*144,groupBytes.data(),groupBytes.size()))return fail("selected group unreadable");
 auto after=studioWrapper(reader,wrapper,root);VirtualSequence checkEntry;uint32_t checkHeader=0;
 if(!after||after->model!=v.model||std::memcmp(&v.cache,&after->cache,sizeof(v.cache))||std::memcmp(&v.seqs,&after->seqs,sizeof(v.seqs))||std::memcmp(&v.groups,&after->groups,sizeof(v.groups))||!load(reader,v.seqs.data+uint32_t(sequence)*16,checkEntry)||std::memcmp(&entry,&checkEntry,sizeof(entry))||!load(reader,v.cache.data+uint32_t(entry.group)*4,checkHeader)||checkHeader!=cached)return fail("mapping changed during read");
 if(reason)*reason="resolved via validated virtual-model group cache";
 return SharedName{*name,header,entry.index,entry.group};
}
// Locate the container independently from whether a particular lazy group is
// cached. A pending sequence must not hide a valid entity field location.
template<class Reader> std::uint32_t findStudioWrapper(Reader& reader,std::uint32_t entity,std::uint32_t root,int sequence,WrapperProbe* probe=nullptr){using namespace dango::entitywire;(void)sequence;WrapperProbe result;
 auto finish=[&](uint32_t p){if(probe)*probe=result;return p;};
 if(!entity||uint64_t(entity)+16384>0x100000000ull){result.reason="entity scan bounds rejected";return finish(0);}
 std::set<uint32_t> seen;unsigned budget=768;result.reason="no root-matching container in bounded entity scan";
 for(unsigned offset=0;offset<16384&&budget;offset+=256){std::array<uint32_t,64> words{};if(!reader.read(entity+offset,words.data(),sizeof(words)))continue;result.scanned+=256;
  for(unsigned i=0;i<words.size();++i){auto pointer=words[i];if(!budget)break;if(pointer<65536||pointer%4||uint64_t(pointer)+28>0x100000000ull||!seen.insert(pointer).second)continue;--budget;++result.candidates;uint32_t first=0;
   if(!load(reader,pointer,first)||first!=root)continue;
   ++result.rootMatches;const char* why=nullptr;auto view=studioWrapper(reader,pointer,root,&why);result.reason=why;
   if(view){result.offset=offset+i*4;return finish(pointer);}
  }
 }return finish(0);
}
}
