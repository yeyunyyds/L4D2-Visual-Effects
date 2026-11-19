#pragma once
#include <cstdint>
#include <sstream>
#include <string>
namespace visual {
inline std::string menuRow(const std::string& label){return "\""+label+"\" { \"user_data\" \"0\" }\n";}
inline std::string menuItem(const std::string& label,std::uintptr_t id){std::ostringstream s;s<<"0x"<<std::hex<<id;return "\""+label+"\" { \"user_data\" \""+s.str()+"\" }\n";}
inline std::string menuPage(const std::string& title,const std::string& body){return "\""+title+"\" {"+body+"}";}
}
