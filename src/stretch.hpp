#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace visual {
// Source's world-space, column-vector matrix3x4_t. Never alias a bone cache.
struct Matrix3x4 { float m[3][4]; };
constexpr std::size_t MaxBones = 256;

inline float scaleAt(std::uint64_t elapsedMs,unsigned periodMs=2000) noexcept {
    if(!periodMs)periodMs=2000;
    constexpr double pi = 3.14159265358979323846;
    return float(1.0 - 0.5 * std::cos(2.0 * pi * double(elapsedMs % periodMs) / double(periodMs)));
}

// Same special-infected category as DangoMusic's entity-follow experiment.
inline bool specialInfected(std::string_view name, int team = -1) noexcept {
    for (auto type : {"Smoker", "Boomer", "Hunter", "Spitter", "Jockey", "Charger", "Tank", "Witch"})
        if (name == type) return true;
    return team == 3 && (name == "CTerrorPlayer" || name == "NextBotCombatCharacter");
}

struct DrawIdentity {
    std::uint32_t epoch{}, handle{};
    int modelIndex{};
};
inline bool sameDraw(const DrawIdentity& a, const DrawIdentity& b) noexcept {
    return a.epoch == b.epoch && a.handle == b.handle &&
           a.modelIndex > 0 && a.modelIndex == b.modelIndex;
}

template<std::size_t Capacity> struct BasicPoseCopy {
    static_assert(Capacity == 128 || Capacity == 256);
    std::array<Matrix3x4, Capacity> bones;
    std::size_t count{};
};
using PoseCopy = BasicPoseCopy<MaxBones>;

// The adapter must supply an anchor from this draw's actual rendered pose.
// Applying T(anchor)*S(1,1,s)*T(-anchor) to ALL matrix components also scales
// skinned vertex offsets. Scaling translations alone would leave limbs rigid.
// On rejection, no source or destination bytes are changed.
template<std::size_t Capacity> inline bool stretchCopy(const Matrix3x4* source, std::size_t count,
                        float anchorZ, float scale, BasicPoseCopy<Capacity>& destination) noexcept {
    if (!source || !count || count > Capacity || !std::isfinite(anchorZ) ||
        !std::isfinite(scale) || scale < 0.1f || scale > 3.0f) return false;
    // Refuse overlap: the caller must not pass the game's pose as output.
    const auto begin = reinterpret_cast<std::uintptr_t>(source);
    const auto out = reinterpret_cast<std::uintptr_t>(&destination);
    const auto bytes = count * sizeof(Matrix3x4);
    if (begin > UINTPTR_MAX - bytes ||
        (begin < out + sizeof(destination) && out < begin + bytes)) return false;
    for (std::size_t i = 0; i < count; ++i) {
        for (auto& row : source[i].m)
            for (float value : row) if (!std::isfinite(value)) return false;
        // Validate the transformed row before touching output; rejection remains transactional.
        for (unsigned c = 0; c < 3; ++c)
            if (!std::isfinite(source[i].m[2][c] * scale)) return false;
        if (!std::isfinite(anchorZ + (source[i].m[2][3] - anchorZ) * scale)) return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        destination.bones[i] = source[i];
        for (unsigned c = 0; c < 3; ++c) destination.bones[i].m[2][c] *= scale;
        destination.bones[i].m[2][3] = anchorZ + (source[i].m[2][3] - anchorZ) * scale;
    }
    destination.count = count;
    return true;
}
} // namespace visual
