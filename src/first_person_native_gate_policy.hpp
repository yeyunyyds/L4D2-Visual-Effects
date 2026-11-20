#pragma once
// Platform-independent contract for a reversible one-byte DrawModel gate.
// An uncertain write must never be interpreted as proof of a patched byte.
namespace visual::firstperson::gatepolicy {
enum class Phase { Clean, Patched, Unknown };
enum class Byte { Original, PassThrough, Foreign };
enum class Decision { AlreadyClean, AlreadyPatched, Restore, Enable, Conflict };
inline Byte classify(unsigned char value,unsigned char original,unsigned char patched) noexcept {
 return value==original?Byte::Original:
        value==patched?Byte::PassThrough:Byte::Foreign;
}
inline Decision decide(bool enable,Phase phase,Byte observed) noexcept {
 if(enable){
  if(phase==Phase::Unknown||observed==Byte::Foreign)return Decision::Conflict;
  if(phase==Phase::Patched)return observed==Byte::PassThrough?
    Decision::AlreadyPatched:Decision::Conflict;
  return observed==Byte::Original?Decision::Enable:Decision::Conflict;
 }
 if(phase==Phase::Clean)return observed==Byte::Original?
    Decision::AlreadyClean:Decision::Conflict;
 if(observed==Byte::Original)return Decision::AlreadyClean;
 if(observed==Byte::PassThrough)return Decision::Restore;
 return Decision::Conflict;
}
inline const char* label(Phase phase) noexcept {
 switch(phase){
 case Phase::Clean:return "Clean";
 case Phase::Patched:return "Patched";
 case Phase::Unknown:return "Unknown";
 }
 return "Invalid";
}
}
