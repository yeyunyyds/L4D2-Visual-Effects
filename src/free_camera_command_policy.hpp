#pragma once
// Source CInput::ExtraMouseSample calls IClientMode::CreateMove with a
// temporary, zero-number CUserCmd. It contains valid per-frame mouse input
// and must not be skipped just because it is not a server-bound command.
namespace visual::freecamera::commandpolicy {
enum class Kind { Invalid, ExtraMouseSample, ServerCommand };
inline constexpr Kind classify(int number) noexcept {
 return number<0?Kind::Invalid:
        number==0?Kind::ExtraMouseSample:Kind::ServerCommand;
}
inline constexpr bool shouldSample(int number) noexcept {
 return classify(number)!=Kind::Invalid;
}
}
