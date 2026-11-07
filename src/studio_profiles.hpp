#pragma once
#include "dango/source_entity_contract.hpp"
namespace visual::studioabi {
struct Profile{const char* name;std::uint32_t timestamp,imageSize;const char* sha256;std::uint32_t object=0x437018,vtable=0x7f03c,draw=0x6a150;unsigned slot=25;};
inline constexpr Profile supported[]={
 {"original studio upload",1782844886,0x453000,"3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85"},
 {"legacy studio upload",1782844886,0x653000,"03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6"}
};
template<class Reader> const Profile* select(Reader& reader,std::uint32_t base){using dango::entitywire::load;std::uint16_t mz=0,machine=0,magic=0;std::uint32_t offset=0,signature=0,time=0,size=0;
 if(!load(reader,base,mz)||mz!=0x5a4d||!load(reader,base+0x3c,offset)||offset<0x40||offset>1024*1024||uint64_t(base)+offset+256>0x100000000ull)return nullptr;
 auto nt=base+offset;
 if(!load(reader,nt,signature)||signature!=0x4550||!load(reader,nt+4,machine)||machine!=0x14c||!load(reader,nt+24,magic)||magic!=0x10b||!load(reader,nt+8,time)||!load(reader,nt+24+56,size))return nullptr;
 for(const auto& profile:supported)if(profile.timestamp==time&&profile.imageSize==size)return &profile;
 return nullptr;
}
}
