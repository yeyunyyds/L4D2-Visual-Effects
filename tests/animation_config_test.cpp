#include "animation_config.hpp"
#include "json.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("Animation config regression");}
int main(){using namespace visual;using nlohmann::json;
 json old={{"font",27.0},{"centerY",false},{"x",35.0},{"animationPeriodMs",2000}};
 auto a=readAnimationSettings(old);check(a.periodMs==500&&a.minTenths==5&&a.maxTenths==11);
 a={2300,7,18,1};writeAnimationSettings(old,a);
 auto restored=readAnimationSettings(json::parse(old.dump()));
 check(restored.periodMs==2300&&restored.minTenths==7&&restored.maxTenths==18&&restored.preset==1);
 check(old["font"]==27.0&&old["x"]==35.0&&old["centerY"]==false);
 old["animationMinTenths"]=50;old["animationMaxTenths"]=-10;old["animationPreset"]=80;
 a=readAnimationSettings(old);check(a.minTenths==29&&a.maxTenths==30&&a.preset==0);
 std::cout<<"PASS: legacy default migration, exact settings round-trip, HUD appearance preservation, invalid bounds\n";
}
