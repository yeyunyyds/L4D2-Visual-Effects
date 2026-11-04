#pragma once
#include "stretch.hpp"
#include "animation.hpp"

namespace visual {
// This is a process-local adapter contract, NOT a guessed Source virtual ABI.
// No engine hook is installed until its signature/layout is verified.
struct RenderInput {
    DrawIdentity observed, draw;
    const Matrix3x4* bones{};
    std::size_t boneCount{};
    float anchorZ{};
    std::uint64_t elapsedMs{}, snapshotAgeMs{};
    bool enabled{}, special{}, dormant{}, verifiedRenderAbi{}, verifiedAnchor{};
    unsigned periodMs=500;
    int minTenths=5,maxTenths=11;
    unsigned preset=0;
};
inline bool prepareDraw(const RenderInput& input, PoseCopy& output) noexcept {
    if (!input.enabled || !input.special || input.dormant ||
        !input.verifiedRenderAbi || !input.verifiedAnchor ||
        input.snapshotAgeMs > 250 || !sameDraw(input.observed, input.draw)) return false;
    return stretchCopy(input.bones, input.boneCount, input.anchorZ,
                       animationScale(input.elapsedMs,{input.periodMs,input.minTenths,input.maxTenths,input.preset}), output);
}
} // namespace visual
