#pragma once
// Only the verified local-player viewmodel can be suppressed, never
// the native world player/weapon or a model matched by a filename.
#include <cstdint>
#include <string_view>
namespace visual::freecamera::viewmodel::policy {
inline bool classIsViewmodel(std::string_view name) noexcept {
 return name=="CBaseViewModel"||name=="CPredictedViewModel"||
        name=="C_ViewmodelAttachmentModel"||name=="CViewModel";
}
inline bool ownedByLocal(std::uint32_t player,std::uint32_t viewmodel,
                         std::uint32_t candidate,std::uint32_t owner,
                         std::string_view cls) noexcept {
 if(!player||player==0xffffffffu||!candidate||candidate==0xffffffffu||
    !classIsViewmodel(cls))return false;
 return (viewmodel&&viewmodel!=0xffffffffu&&viewmodel==candidate)||
        owner==player;
}
inline bool suppress(bool active,bool option,std::uint32_t accepted,
                     std::uint32_t renderable,bool epoch,bool fresh,bool live) noexcept {
 return active&&option&&accepted!=0&&accepted==renderable&&epoch&&fresh&&live;
}
}
