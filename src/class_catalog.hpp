#pragma once
#include "dango/source_entity_contract.hpp"
namespace visual {
struct ClassCatalog {
 std::set<std::uint32_t> descriptors;
 std::map<int,std::string> names;
 void clear(){descriptors.clear();names.clear();}
 template<class Reader,class Owner> void observe(Reader& reader,std::uint32_t address,Owner owned){
  namespace wire=dango::entitywire;
  for(unsigned budget=0;address && budget<4096 && descriptors.size()<4096;++budget){
   if(descriptors.count(address))break;
   wire::ClientClass descriptor{};
   if(!owned(address)||!wire::load(reader,address,descriptor)||descriptor.id<0||descriptor.id>4096)break;
   auto name=wire::text(reader,descriptor.name);if(!wire::className(name))break;
   descriptors.insert(address);names[descriptor.id]=std::move(name);address=descriptor.next;
  }
 }
};
} // namespace visual
