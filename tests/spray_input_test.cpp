#include "spray_input.hpp"
#include "spray_display.hpp"
#include <stdexcept>
#include <iostream>
void check(bool ok){if(!ok)throw std::runtime_error("spray input regression");}
int main(){using namespace visual::pictures;SprayInput state;bool keys[SprayInput::Count]={};
 auto tap=[&](unsigned key,std::uint64_t now,bool foreground=true,bool menu=false,bool enabled=true){for(auto& k:keys)k=false;state.tick(keys,foreground,menu,enabled,now);keys[key]=true;return state.tick(keys,foreground,menu,enabled,now);};
 check(tap(SprayInput::T,0));state.placed(0);check(!state.tick(keys,true,false,true,600));check(!tap(SprayInput::T,499));check(tap(SprayInput::T,500));state.placed(500);
 check(!tap(SprayInput::Chat,1000));check(!tap(SprayInput::T,1001));check(!tap(SprayInput::Enter,1002));check(tap(SprayInput::T,1003));
 check(!tap(SprayInput::Console,2000));check(!tap(SprayInput::T,2001));check(!tap(SprayInput::Console,2002));check(tap(SprayInput::T,2003));
 check(!tap(SprayInput::T,3000,false));check(!tap(SprayInput::T,3001,true,true));check(!tap(SprayInput::Chat,3002,true,true));check(tap(SprayInput::T,3003));check(!tap(SprayInput::T,4000,true,false,false));
 DisplayGate display;displayPaused=false;display.poll(true,true,true,true);check(!displayPaused);display.poll(false,false,true,true);display.poll(true,false,true,true);check(displayPaused);display.poll(false,false,true,true);display.poll(true,false,true,true);check(displayPaused);display.poll(false,false,true,false);check(!displayPaused);
 std::cout<<"PASS: T edges, debounce, focus/menu gates, chat exit, console toggle recovery and conservative Esc display suspension\n";
}
