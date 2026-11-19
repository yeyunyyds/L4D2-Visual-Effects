#pragma once
// Logging policy is intentionally separate from the file writer: INFO event
// messages always use diagnostics::write; DEBUG enables periodic snapshots.
#include <atomic>
#include <string>

namespace visual::logging {
enum class Level : unsigned char { Info, Debug };
inline std::atomic<Level> level{Level::Info};

inline bool periodicDebugEnabled() noexcept {
 return level.load(std::memory_order_relaxed)==Level::Debug;
}
inline const char* levelName(Level value) noexcept {
 return value==Level::Debug?"debug":"info";
}
inline Level toggle() noexcept {
 const Level next=periodicDebugEnabled()?Level::Info:Level::Debug;
 level.store(next,std::memory_order_relaxed);
 return next;
}
template<class Json>
Level readLevel(const Json& root) {
 if(!root.is_object())return Level::Info;
 auto config=root.find("logging");
 if(config==root.end()||!config->is_object())return Level::Info;
 auto setting=config->find("level");
 if(setting==config->end()||!setting->is_string())return Level::Info;
 return setting->template get<std::string>()=="debug"?Level::Debug:Level::Info;
}
template<class Json>
void writeSettings(Json& root) {
 root["logging"]=Json{{"level",levelName(level.load(std::memory_order_relaxed))}};
}
} // namespace visual::logging
