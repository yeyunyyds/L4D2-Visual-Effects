#pragma once
#include "tank_model.hpp"
#include <array>
#include "dango/source_entity_contract.hpp"
namespace visual::tank {
struct GlowLayout{std::uint32_t property=0,type=0;explicit operator bool()const{return property&&type==property+4;}};
template<class Reader> GlowLayout glowLayout(Reader& r,std::uint32_t root){
 using namespace dango::entitywire;std::set<std::uint32_t> seen;
 for(unsigned depth=0;root&&depth<16;++depth){Table t;if(!seen.insert(root).second||!load(r,root,t)||t.count<0||t.count>2048||text(r,t.name).rfind("DT_",0)!=0)return {};std::uint32_t parent=0;
 for(int i=0;i<t.count;++i){Prop p;if(!load(r,t.props+unsigned(i)*sizeof(Prop),p))return {};auto name=text(r,p.name);
 if(name=="baseclass"&&p.type==6&&p.offset==0)parent=p.table;
 if(name!="m_Glow")continue;
 if(p.type!=6||p.offset<=0||p.offset>65500)return {};
 Table child;if(!load(r,p.table,child)||text(r,child.name)!="DT_GlowProperty"||child.count<4||child.count>64)return {};
 const char* names[]={"m_iGlowType","m_nGlowRange","m_nGlowRangeMin","m_glowColorOverride"};bool found[4]{};
 for(int j=0;j<child.count;++j){Prop c;if(!load(r,child.props+unsigned(j)*sizeof(Prop),c))return {};auto n=text(r,c.name);for(unsigned k=0;k<4;++k)if(n==names[k]){if(c.type!=0||c.offset!=int((k+1)*4)||found[k])return {};found[k]=true;}}
 if(!std::all_of(found,found+4,[](bool b){return b;}))return {};
 return {unsigned(p.offset),unsigned(p.offset+4)};
 }root=parent;
 }return {};
}
struct GlowLease{
 std::array<int,4> before{},written{2,8192,0,255}; // COLORREF red; native type 2 uses LOS.
 bool acquire(const std::array<int,4>& current){if(current[0]!=0)return false;before=current;return true;}
 std::array<bool,4> owned(const std::array<int,4>& current)const{std::array<bool,4> result{};for(unsigned i=0;i<4;++i)result[i]=current[i]==written[i];return result;}
};
}
