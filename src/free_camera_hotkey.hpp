#pragma once
// Config-file hotkey parser. Uses stable Win32 virtual-key values without
// depending on windows.h, so the parsing logic is testable on Linux.
#include <optional>
#include <string>
#include <string_view>
#include <cctype>

namespace visual::freecamera::hotkey {
inline constexpr int defaultVk=0x75; // VK_F6
struct NamedKey {std::string_view name;int vk;};
inline constexpr NamedKey named[]={
 {"backspace",0x08},{"tab",0x09},{"enter",0x0D},
 {"space",0x20},{"pageup",0x21},{"pagedown",0x22},
 {"end",0x23},{"home",0x24},{"left",0x25},{"up",0x26},
 {"right",0x27},{"down",0x28},{"insert",0x2D},{"delete",0x2E},
 {"shift",0x10},{"ctrl",0x11},{"control",0x11},{"alt",0x12},
 {"capslock",0x14},{"caps",0x14},{"pause",0x13},
 {"lshift",0xA0},{"rshift",0xA1},{"lctrl",0xA2},{"rctrl",0xA3},{"lalt",0xA4},{"ralt",0xA5},
 {"numlock",0x90},{"scrolllock",0x91},
 {"mouse3",0x04},{"mouse4",0x05},{"mouse5",0x06}
};
inline std::string normalize(std::string_view raw){
 const auto first=raw.find_first_not_of(" \t\r\n");
 if(first==std::string_view::npos)return {};
 const auto last=raw.find_last_not_of(" \t\r\n");
 std::string result(raw.substr(first,last-first+1));
 for(char& c:result)c=char(std::tolower(static_cast<unsigned char>(c)));
 return result;
}
inline std::optional<int> parse(std::string_view raw){
 const std::string s=normalize(raw);
 if(s.size()==1){
  const char c=s[0];
  if(c>='a'&&c<='z')return int(c-'a'+'A');
  if(c>='0'&&c<='9')return int(c);
 }
 if(s.size()>=2&&s[0]=='f'){
  int number=0;
  for(size_t i=1;i<s.size();++i){
   if(s[i]<'0'||s[i]>'9')return std::nullopt;
   number=number*10+(s[i]-'0');
   if(number>24)return std::nullopt;
  }
  // F10 is reserved for the native menu; never bind to its toggle.
  if(number>=1&&number<=24&&number!=10)return 0x70+number-1;
 }
 if(s.size()==7&&s.substr(0,6)=="numpad"&&s[6]>='0'&&s[6]<='9')
  return 0x60+(s[6]-'0');
 for(const auto& item:named)if(s==item.name)return item.vk;
 // Esc is menu close and cannot safely toggle the camera.
 return std::nullopt;
}
inline std::string label(int vk){
 if(vk==0)return "none";
 if(vk>=0x70&&vk<=0x87)return "F"+std::to_string(vk-0x70+1);
 if(vk>='A'&&vk<='Z')return std::string(1,char(vk));
 if(vk>='0'&&vk<='9')return std::string(1,char(vk));
 if(vk>=0x60&&vk<=0x69)return "Numpad"+std::to_string(vk-0x60);
 for(const auto& item:named)if(vk==item.vk)return std::string(item.name);
 return "F6";
}
}
