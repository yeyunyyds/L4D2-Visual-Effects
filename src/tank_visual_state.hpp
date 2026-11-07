#pragma once
#include "tank_visual.hpp"
#include <atomic>
#include <memory>
namespace visual::tank {
inline std::shared_ptr<const VisualSettings> visualSettings=std::make_shared<const VisualSettings>();
namespace display {
inline std::atomic<bool> paused{false},foreground{true},resumeRequested{false};
inline std::atomic<const char*> gateReason{"waiting for Present"},stage{"not drawn yet"};
inline std::atomic<std::uint64_t> calls{0},submitted{0},barProjected{0},barRejected{0},tick{0},renderAnchorHits{0},networkAnchorFallbacks{0};
inline std::atomic<unsigned> visibleBars{0};inline std::atomic<float> viewportWidth{0},viewportHeight{0};inline std::atomic<long> result{0};
}
}
