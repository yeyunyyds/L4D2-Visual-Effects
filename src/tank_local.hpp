#pragma once
#include "tank_model.hpp"
#include "local_player.hpp"
namespace visual::tank {
inline std::uint32_t localPlayerHandle(const Frame& frame){auto h=localplayer::handle(frame);auto r=resolve(frame,h);return r&&survivor(*r)?h:0xffffffffu;}
}
