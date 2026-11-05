#pragma once
#include <cstdint>
namespace visual::pictures {
// No system-cursor visibility dependency: DXVK/relative input can leave the OS
// cursor flag set while the game's actual cursor is hidden.
struct SprayInput {
 enum Key{T,Chat,TeamChat,Console,Enter,Escape,Count};
 bool held[Count]={},chat=false,console=false;
 std::uint64_t last=0;bool sent=false;
 void reset(){for(auto& key:held)key=false;chat=console=false;}
 bool tick(const bool (&keys)[Count],bool foreground,bool menu,bool enabled,std::uint64_t now){
  if(!foreground||!enabled){reset();return false;}
  bool edge[Count]={};for(unsigned i=0;i<Count;++i){edge[i]=keys[i]&&!held[i];held[i]=keys[i];}
  if(edge[Console])console=!console;
  if(edge[Escape])chat=console=false;
  else if(edge[Enter])chat=false;
  else if(!menu&&(edge[Chat]||edge[TeamChat]))chat=true;
  return edge[T]&&!menu&&!chat&&!console&&(!sent||(now>=last&&now-last>=500));
 }
 void placed(std::uint64_t now){last=now;sent=true;}
};
}
