#pragma once
#include "dango/source_entity_contract.hpp"
#include <cstdlib>
#include <optional>
namespace visual::tank {
// Read-only Source x86 ConVar candidate. Never dispatch ICvar/ConVar virtuals.
// Reject candidates unless names, parent, replicated flag and string/float agree.
struct CvarValue {std::uint32_t object=0,parent=0;float value=0;};
template<class Reader> std::optional<CvarValue> readCvar(Reader& reader,std::uint32_t object,const std::string& expected,float minimum,float maximum){
 using namespace dango::entitywire;std::uint32_t words[13]{};
 if(!reader.read(object,words,sizeof(words))||!words[0]||!words[6]||text(reader,words[3])!=expected||(words[5]&(1u<<13))==0)return {};
 auto parent=words[7];if(!parent||!reader.read(parent,words,sizeof(words))||words[7]!=parent||text(reader,words[3])!=expected||(words[5]&(1u<<13))==0)return {};
 auto current=text(reader,words[9],64),defaults=text(reader,words[8],64);if(current.empty()||defaults.empty()||words[10]<current.size()+1||words[10]>65536)return {};
 char* end=nullptr;float number=std::strtof(current.c_str(),&end);if(end!=current.c_str()+current.size()||!std::isfinite(number))return {};
 float raw;std::memcpy(&raw,words+11,4);if(!std::isfinite(raw)||std::abs(raw-number)>.001f*std::max(1.f,std::abs(number))||raw<minimum||raw>maximum)return {};
 return CvarValue{object,parent,raw};
}
}
