#include "../src/free_camera_hotkey.hpp"
#include <cassert>
#include <string>

int main(){
 using visual::freecamera::hotkey::parse;
 using visual::freecamera::hotkey::label;
 using visual::freecamera::hotkey::normalize;
 assert(parse("f6")==0x75);
 assert(parse("  F7  ")==0x76);
 assert(parse("F12")==0x7B);
 assert(parse("f24")==0x87);
 assert(!parse("f10"));
 assert(!parse("f25"));
 assert(!parse("f0"));
 assert(!parse("escape"));
 assert(!parse("unknown-key"));
 assert(!parse(""));
 assert(parse("p")=='P');
 assert(parse("0")=='0');
 assert(parse("mouse4")==0x05);
 assert(parse("numpad7")==0x67);
 assert(parse("left")==0x25);
 assert(parse("pageup")==0x21);
 assert(parse("ctrl")==0x11);
 assert(parse("lshift")==0xA0&&parse("rshift")==0xA1);
 assert(parse("caps")==0x14&&parse("capslock")==0x14);
 assert(label(0)=="none"&&label(0xA0)=="lshift");
 assert(label(*parse("f6"))=="F6");
 assert(label(*parse("mouse5"))=="mouse5");
 assert(label(*parse("numpad1"))=="Numpad1");
 assert(normalize("  Mouse4 ")=="mouse4");
}
