#include "first_person_native_gate_policy.hpp"
#include "first_person_body_patch.hpp"
#include <stdexcept>
#include <iostream>
using namespace visual::firstperson;
static void check(bool ok){if(!ok)throw std::runtime_error("Native gate transition contract");}
int main(){
 using gatepolicy::Phase;using gatepolicy::Byte;using gatepolicy::Decision;
 using gatepolicy::decide;using gatepolicy::classify;
 check(classify(branch::Original,branch::Original,branch::PassThrough)==Byte::Original);
 check(classify(branch::PassThrough,branch::Original,branch::PassThrough)==Byte::PassThrough);
 check(classify(0x42,branch::Original,branch::PassThrough)==Byte::Foreign);
 check(decide(false,Phase::Clean,Byte::Original)==Decision::AlreadyClean);
 check(decide(false,Phase::Patched,Byte::PassThrough)==Decision::Restore);
 check(decide(false,Phase::Patched,Byte::Original)==Decision::AlreadyClean);
 check(decide(false,Phase::Unknown,Byte::Original)==Decision::AlreadyClean);
 check(decide(false,Phase::Unknown,Byte::PassThrough)==Decision::Restore);
 check(decide(false,Phase::Unknown,Byte::Foreign)==Decision::Conflict);
 check(decide(false,Phase::Clean,Byte::PassThrough)==Decision::Conflict);
 check(decide(true,Phase::Clean,Byte::Original)==Decision::Enable);
 check(decide(true,Phase::Clean,Byte::PassThrough)==Decision::Conflict);
 check(decide(true,Phase::Patched,Byte::PassThrough)==Decision::AlreadyPatched);
 check(decide(true,Phase::Unknown,Byte::Original)==Decision::Conflict);
 check(decide(true,Phase::Unknown,Byte::PassThrough)==Decision::Conflict);
 check(std::string(gatepolicy::label(Phase::Unknown))=="Unknown");
 std::cout<<"PASS: native DrawModel ownership and failure recovery transitions\n";
}
