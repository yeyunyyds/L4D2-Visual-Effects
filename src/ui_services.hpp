#pragma once
#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>
namespace visual::ui {
inline std::atomic<bool> saveRequested{false};
inline std::atomic<const char*> saveMessage{""};
inline std::string row(const std::string& label,std::uintptr_t id=0){std::ostringstream out;out<<"\""<<label<<"\" { \"user_data\" \"0x"<<std::hex<<id<<"\" }\n";return out.str();}
inline std::string item(const std::string& label,std::uintptr_t id){return row(label,id);}
inline std::string decimal(int tenths){return std::to_string(tenths/10)+"."+std::to_string(tenths%10);}
}
