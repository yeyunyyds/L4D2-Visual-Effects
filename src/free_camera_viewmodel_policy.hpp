#pragma once
// Source ClientMode viewmodel visibility overlay: never modify actual
// camera mode, world body, weapon entities or the native decision when
// F6 is off or the menu option is disabled.
namespace visual::freecamera::viewmodel::policy {
inline bool visible(bool freeCameraRequested,bool hideEnabled,bool original) noexcept {
 return (freeCameraRequested&&hideEnabled)?false:original;
}
} // namespace visual::freecamera::viewmodel::policy
