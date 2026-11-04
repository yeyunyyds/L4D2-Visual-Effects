#pragma once
#include "stretch.hpp"
namespace visual {
// Identity comes from the entity handle + current model index and the engine's
// draw state/renderable. A model-info name query is optional diagnostic data.
template<class Record> bool eligibleRecord(const Record& record) {
 return record.modelIndex && *record.modelIndex>0 &&
        specialInfected(record.className,record.team.value_or(-1));
}
template<class Record> bool sameRenderableTarget(const Record& record,std::uint32_t entity,
                                                std::uint32_t serial,std::uint32_t renderableBase) {
 return entity!=0 && entity==renderableBase && serial==record.serial;
}
}
