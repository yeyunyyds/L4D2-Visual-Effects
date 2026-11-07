#include "entity_transform.hpp"
#include "dango/entity_inventory.hpp"
#include "json.hpp"
#include "dango/hud_menu_policy.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace visual;using namespace visual::deform;
void check(bool b){if(!b)throw std::runtime_error("Entity transform regression");}
int main(){Settings s;check(!any(s));check(dango::hud::opensChild(800)&&dango::hud::opensChild(0x710000)&&!dango::hud::opensChild(0x700020));Rule r;r.enabled=true;r.size=2;float pivot[]={10,20,30};std::array<Matrix3x4,256> input{},out;
 for(unsigned i=0;i<256;++i){for(unsigned a=0;a<3;++a)input[i].m[a][a]=1;input[i].m[0][3]=float(i)+10;input[i].m[1][3]=22;input[i].m[2][3]=34;}
 auto original=input;check(transform(input.data(),out.data(),256,pivot,r));check(out[255].m[0][3]==520&&out[0].m[2][3]==38&&out[0].m[0][0]==2&&input[255].m[0][3]==original[255].m[0][3]);
 r.size=1;r.z=.5;check(!uniform(r));check(transform(input.data(),out.data(),128,pivot,r));check(out[0].m[0][3]==10&&out[0].m[1][3]==22&&out[0].m[2][3]==32&&out[0].m[2][2]==.5f);
 unsigned flags[]={1,2};check(transform(input.data(),out.data(),2,pivot,r,flags,1));check(out[0].m[2][3]==32&&out[1].m[2][3]==34);
 check(boneFlagVector({4096,256,0,256,4096},256));check(!boneFlagVector({4096,256,0,128,4096},256));check(!boneFlagVector({4096,256,0,256,8192},256));
 auto before=out;input[1].m[0][0]=std::numeric_limits<float>::quiet_NaN();check(!transform(input.data(),out.data(),2,pivot,r));check(!std::memcmp(out.data(),before.data(),sizeof(out)));input=original;
 check(!transform(input.data(),out.data(),257,pivot,r));check(!transform(input.data(),input.data()+1,2,pivot,r));float center[3];check(centroid(input.data(),2,center)&&center[0]==10.5f&&center[2]==34);check(!centroid(input.data(),0,center));
 using dango::inventory::Record;Record tank;tank.className="CTerrorPlayer";tank.classId=1;tank.team=3;tank.zombieClass=8;Record zoey;zoey.className="CTerrorPlayer";zoey.classId=1;zoey.team=2;zoey.character=5;
 s.enabled=true;s.rules["g:0"]=r;check(any(s)&&selected(s,tank).z==.5f&&!selected(s,zoey).enabled);Rule off;s.rules["s:6"]=off;check(!selected(s,tank).enabled);s.rules.erase("s:6");check(inherited(s,"s:6").z==.5f);
 Record prop;prop.classId=77;prop.className="CustomProp";s.rules["c:77:CustomProp"]=r;check(selected(s,prop).enabled);prop.className="OtherProp";check(!selected(s,prop).enabled);
 nlohmann::json j;writeSettings(j,s);auto back=readSettings(j);check(back.enabled&&back.rules.size()==s.rules.size()&&back.rules.at("g:0").z==.5f);j["entityTransform"]["rules"]["g:0"]["z"]=-100;check(readSettings(j).rules.at("g:0").z==.1f);check(!readSettings(nlohmann::json::object()).enabled);
 std::cout<<"PASS: 128/256 XYZ transforms, native mask, pivot, immutable/transactional buffers, invalid/overlap rejection, role/category identity and settings\n";
}
