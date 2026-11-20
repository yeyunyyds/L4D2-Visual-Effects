#pragma once
// Source ClientMode viewmodel visibility overlay: never modify actual
// camera mode, world body, weapon entities or the native decision when
// FreeCamera is inactive or the menu option is disabled.
namespace visual::freecamera::viewmodel::policy {
inline bool visible(bool freeCameraActive,bool hideEnabled,bool original) noexcept {
 return (freeCameraActive&&hideEnabled)?false:original;
}
} // namespace visual::freecamera::viewmodel::policy
