#include "first_person_native_probe_policy.hpp"
#include <cassert>
int main(){
 using namespace visual::firstperson::nativeprobe;
 const std::uint8_t imm[]={0xb8,0,0x10,0x40,0,0xc3};
 const std::uint8_t slot[]={0xa1,0,0x20,0x40,0,0xc3};
 const std::uint8_t slot2[]={0x8b,0x05,0,0x20,0x40,0,0xc3};
 const std::uint8_t arg[]={0x8b,0x44,0x24,0x04,0xc3,0x90};
 const std::uint8_t thunk[]={0xe9,0,0,0,0,0xc3};
 const std::uint8_t noReturn[]={0xb8,0,0x10,0x40,0,0x90};
 assert(decode(imm,sizeof(imm)).kind==GetterKind::Immediate);
 assert(decode(imm,sizeof(imm)).operand==0x401000);
 assert(decode(slot,sizeof(slot)).kind==GetterKind::AbsoluteSlot);
 assert(decode(slot2,sizeof(slot2)).operand==0x402000);
 assert(decode(slot2,6).kind==GetterKind::Unknown);
 assert(decode(arg,sizeof(arg)).kind==GetterKind::Unknown);
 assert(decode(thunk,sizeof(thunk)).kind==GetterKind::Unknown);
 assert(decode(noReturn,sizeof(noReturn)).kind==GetterKind::Unknown);
 assert(decode(nullptr,0).kind==GetterKind::Unknown);
 assert(decode(CheckedGetter,sizeof(CheckedGetter)).kind==GetterKind::CheckedArgument);
 assert(decode(CheckedGetter,sizeof(CheckedGetter)-1).kind==GetterKind::Unknown);
 std::uint8_t changed[sizeof(CheckedGetter)];std::memcpy(changed,CheckedGetter,sizeof(changed));changed[15]^=1;
 assert(decode(changed,sizeof(changed)).kind==GetterKind::Unknown);
 assert(callerMatches(GateCallerPrefix,sizeof(GateCallerPrefix)));
 assert(!callerMatches(GateCallerPrefix,sizeof(GateCallerPrefix)-1));
 std::uint8_t altered[sizeof(GateCallerPrefix)];std::memcpy(altered,GateCallerPrefix,sizeof(altered));altered[12]^=1;
 assert(!callerMatches(altered,sizeof(altered)));
 std::uint32_t out=0;assert(fieldAddress(0x401000,out)&&out==0x4035ec);
 assert(!fieldAddress(0,out));assert(!fieldAddress(0xffffff00,out));
}
