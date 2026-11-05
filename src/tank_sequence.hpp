#pragma once
#include "dango/source_entity_contract.hpp"
namespace visual::tank {
struct SequenceName{std::string label,activity;};
template<class Reader> std::optional<SequenceName> sequenceName(Reader& reader,std::uint32_t header,int sequence){
 using namespace dango::entitywire;uint32_t magic=0;int version=0,length=0,count=0,offset=0;
 if(!load(reader,header,magic)||magic!=0x54534449||!load(reader,header+4,version)||version<44||version>49||!load(reader,header+76,length)||length<212||length>64*1024*1024||uint64_t(header)+length>0x100000000ull||!load(reader,header+188,count)||count<1||count>4096||sequence<0||sequence>=count||!load(reader,header+192,offset)||offset<212||offset>=length)return {};
 std::optional<SequenceName> found;
 for(auto stride:{212,216,224,232}){int64_t relative=int64_t(offset)+int64_t(sequence)*stride;if(relative<0||relative+212>length)continue;auto address=header+uint32_t(relative);int back=0,label=0,activity=0;
  if(!load(reader,address,back)||int64_t(relative)+back!=0||!load(reader,address+4,label)||!load(reader,address+8,activity))continue;
  auto name=[&](int rel){auto at=int64_t(relative)+rel;return rel>0&&at>=0&&at+128<=length?text(reader,header+uint32_t(at),128):std::string();};
  auto l=name(label),a=name(activity);if(l.empty()||l.size()>100||(!a.empty()&&a.rfind("ACT_",0)!=0))continue;
  if(found&&(found->label!=l||found->activity!=a))return {};
  found=SequenceName{l,a};
 }return found;
}
inline std::string classify(SequenceName name){auto s=name.label+" "+name.activity;for(char& c:s)if(c>='A'&&c<='Z')c=char(c+32);
 if(s.find("throw")!=std::string::npos||s.find("overhead")!=std::string::npos)return "throw-animation";
 if(s.find("punch")!=std::string::npos||s.find("melee")!=std::string::npos||s.find("attack")!=std::string::npos)return "attack-animation";
 if(s.find("run")!=std::string::npos||s.find("walk")!=std::string::npos)return "movement-animation";
 return "other-animation";
}
}
