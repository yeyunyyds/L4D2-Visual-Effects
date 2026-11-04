#include "aim_selection.hpp"
#include <iostream>
#include <stdexcept>
using namespace dango;
void check(bool value){if(!value)throw std::runtime_error("aim selection regression");}
inventory::Record object(std::uint32_t handle,int classId,Vec3 origin,const char* name){
 inventory::Record r;r.handle=handle;r.index=handle&4095;r.serial=handle>>12;r.classId=classId;r.className=name;
 r.modelIndex=10;r.networkOrigin=origin;r.networkAngles=Vec3{};r.parent=0xffffffffu;r.parentAttachment=0;r.dormant=false;return r;
}
int main(){
 GameCameraFrame camera;camera.active=1;camera.epoch=7;camera.width=1920;camera.height=1080;camera.tick=1000;
 camera.worldToView={0,-1,0,0,0,0,1,0,1,0,0,0,0,0,0,1};
 camera.worldToClip={0,-1,0,0,0,0,1,0,1.0001f,0,0,-1.0001f,1,0,0,0};
 inventory::Frame frame;frame.epoch=7;frame.tick=1000;
 for(auto name:{"Tank","Witch","Boomer","CTerrorPlayer","SurvivorBot","CPhysicsProp","CGnome","CustomModeledEntity"}){
  frame.records={object(0x9001,260,{100,0,0},name)};
  auto chosen=visual::identifyAim(frame,camera,1100);check(chosen.classId==260&&chosen.handle==0x9001&&chosen.name==name);
 }
 frame.records={object(0x9001,260,{100,0,0},"Tank"),object(0x9002,261,{100,20,0},"Witch")};
 check(visual::identifyAim(frame,camera,1100).classId==260);
 frame.records[0].dormant=true;check(visual::identifyAim(frame,camera,1100).classId==261);
 frame.records[1].networkOrigin=Vec3{-100,0,0};check(visual::identifyAim(frame,camera,1100).classId==-1);
 frame.records[1].networkOrigin=Vec3{100,0,0};
 check(visual::identifyAim(frame,camera,1251).classId==-1&&visual::identifyAim(frame,camera,999).classId==-1);
 frame.epoch=8;check(visual::identifyAim(frame,camera,1100).classId==-1);frame.epoch=7;
 frame.records[1].classId=-1;check(visual::identifyAim(frame,camera,1100).classId==-1);
 std::cout<<"PASS: DangoMusic nearby-candidate ranking, Class ID resolution, props/gnome/custom classes, dormancy, stale/map mismatch and behind-camera rejection\n";
}
