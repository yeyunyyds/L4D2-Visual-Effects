#pragma once
namespace visual {
// Own a reversible renderer setting, without overwriting a later user change.
struct RenderOverride {
 bool owned=false;int original=0;
 template<class Setter> bool update(bool active,int current,Setter set) {
  if(active&&!owned){
   if(current!=0&&!set(0))return false;
   original=current;owned=true;
  }else if(!active&&owned){
   if(current==0&&original!=0&&!set(original))return false;
   owned=false;
  }
  return true;
 }
};
}
