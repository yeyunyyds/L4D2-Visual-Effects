#pragma once
#include "tank_sequence.hpp"
namespace visual::tank {
// Explicit opt-in experiment based on the user's vanilla trace. These are
// observed GLOBAL overlay indices, never claimed to be local MDL descriptors.
inline bool originalBackupCompatible(const ModelKey& key){return key.checksum==1500020281&&key.version==49&&key.count==1&&key.includes==1&&lower(key.name)=="infected/hulk.mdl";}
inline std::optional<SequenceName> originalLayerBackup(const ModelKey& key,int sequence){
 if(!originalBackupCompatible(key))return {};
 if(sequence!=40&&sequence!=43&&sequence!=45)return {};
 return SequenceName{"experimental_punch_backup_"+std::to_string(sequence),""};
}
}
