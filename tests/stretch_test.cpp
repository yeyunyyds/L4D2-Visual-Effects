#include "render_contract.hpp"
#include "target_identity.hpp"
#include "animation.hpp"
#include "render_override.hpp"
#include "entity_inventory.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

void check(bool value) { if (!value) throw std::runtime_error("stretch regression"); }
bool near(float a, float b) { return std::abs(a-b) < 0.0002f; }
int main() {
    using namespace visual;
    check(near(scaleAt(0), .5f) && near(scaleAt(500), 1.f));
    check(near(scaleAt(1000), 1.5f) && near(scaleAt(1500), 1.f));
    check(near(scaleAt(2000), .5f));
    for (std::uint64_t t=0; t<10000; ++t) {
        check(scaleAt(t)>=.5f && scaleAt(t)<=1.5f);
        check(near(scaleAt(t), scaleAt(t+2000)));
        if (t%2000<1000) check(scaleAt(t+1)>=scaleAt(t));
        else check(scaleAt(t+1)<=scaleAt(t));
    }
    // Replay the reported game state: model-info is hooked and model names are
    // absent, but the verified raw inventory has a valid infected/model index.
    dango::inventory::Record tank;tank.className="Tank";tank.modelIndex=12;
    check(tank.modelName.empty() && eligibleRecord(tank));
    tank.modelIndex.reset();check(!eligibleRecord(tank));
    tank.modelIndex=0;check(!eligibleRecord(tank));
    tank.modelIndex=12;tank.className="CTerrorPlayer";tank.team=3;check(eligibleRecord(tank));
    tank.team=2;check(!eligibleRecord(tank));
    // Direct Studio draws lack an engine context. All special-infected classes
    // can match their renderable; stale serials and other entities cannot.
    for(auto name:{"Tank","Boomer","Hunter","Smoker","Charger","Spitter","Jockey","Witch"}) {
        dango::inventory::Record r;r.className=name;r.modelIndex=17;r.serial=9;
        check(eligibleRecord(r) && sameRenderableTarget(r,0x1234,9,0x1234));
        check(!sameRenderableTarget(r,0x1234,10,0x1234));
        check(!sameRenderableTarget(r,0x1234,9,0x5678));
        check(!sameRenderableTarget(r,0,9,0));
    }
    RenderOverride setting;int value=1,calls=0;
    auto setter=[&](int v){value=v;++calls;return true;};
    check(setting.update(true,value,setter)&&value==0&&calls==1);
    check(setting.update(true,value,setter)&&calls==1);
    check(setting.update(false,value,setter)&&value==1&&calls==2);
    value=0;check(setting.update(true,value,setter)&&setting.original==0);
    check(setting.update(false,value,setter)&&value==0&&calls==2);
    value=1;check(setting.update(true,value,setter));value=2;
    check(setting.update(false,value,setter)&&value==2); // preserve user's later change
    check(!setting.update(true,value,[](int){return false;})&&!setting.owned);
    Matrix3x4 pose[2]{{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}},
                      {{{0,-1,0,12},{1,0,0,25},{0,0,1,100}}}};
    Matrix3x4 original[2]; std::memcpy(original,pose,sizeof(pose));
    PoseCopy output;
    for(float scale : {.5f,1.f,1.5f}) {
        check(stretchCopy(pose,2,30,scale,output));
        check(near(output.bones[0].m[2][3],30)); // foot-plane stays fixed
        check(near(output.bones[1].m[2][3],30+70*scale));
        check(near(output.bones[1].m[2][2],scale)); // vertex offsets stretch too
        for(unsigned i=0;i<2;++i) for(unsigned r=0;r<2;++r)
            check(std::memcmp(pose[i].m[r],output.bones[i].m[r],4*sizeof(float))==0);
        check(std::memcmp(original,pose,sizeof(pose))==0);
    }
    // Non-axis-aligned bones: arbitrary skinned point obeys the same world Z transform.
    pose[1].m[2][0]=.6f; pose[1].m[2][1]=-.8f;
    check(stretchCopy(pose,2,30,1.5f,output));
    float vertex[]{3,4,5,1}; float z=0,stretched=0;
    for(unsigned c=0;c<4;++c) {z+=pose[1].m[2][c]*vertex[c];stretched+=output.bones[1].m[2][c]*vertex[c];}
    check(near(stretched,30+(z-30)*1.5f));
    auto before=output;
    check(!stretchCopy(pose,257,30,1,output));
    check(!stretchCopy(nullptr,2,30,1,output));
    check(!stretchCopy(pose,0,30,1,output));
    check(!stretchCopy(pose,2,30,4,output));
    check(!stretchCopy(output.bones.data(),2,30,1,output));
    pose[1].m[0][0]=std::numeric_limits<float>::quiet_NaN();
    check(!stretchCopy(pose,2,30,1,output));
    check(std::memcmp(&before,&output,sizeof(output))==0);
    pose[1].m[0][0]=0;
    RenderInput input{{1,0x1001,10},{1,0x1001,10},pose,2,30,1000,0,true,true,false,true,true};
    check(prepareDraw(input,output));
    input.draw.handle+=0x1000; check(!prepareDraw(input,output)); // reused index
    input.draw=input.observed; input.draw.epoch++; check(!prepareDraw(input,output));
    input.draw=input.observed; input.draw.modelIndex++; check(!prepareDraw(input,output));
    input.draw=input.observed; input.verifiedRenderAbi=false;check(!prepareDraw(input,output));
    input.verifiedRenderAbi=true;input.verifiedAnchor=false;check(!prepareDraw(input,output));
    input.verifiedAnchor=true;input.snapshotAgeMs=251;check(!prepareDraw(input,output));
    input.snapshotAgeMs=0;input.dormant=true;check(!prepareDraw(input,output));
    input.dormant=false;input.enabled=false;check(!prepareDraw(input,output));
    for(auto name : {"Tank","Smoker","Boomer","Hunter","Spitter","Jockey","Charger","Witch"}) check(specialInfected(name));
    check(specialInfected("CTerrorPlayer",3));check(!specialInfected("CTerrorPlayer",2));
    check(!specialInfected("Infected"));check(!specialInfected("CPhysicsProp"));
    std::cout << "PASS: 2s cycle, bounds, foot anchor, matrix deformation, immutable input, draw identity, fail-closed gates, infected category\n";
}
