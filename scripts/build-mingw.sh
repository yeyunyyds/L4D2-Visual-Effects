#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
compiler="${CXX_X86:-i686-w64-mingw32-g++}"
if ! command -v "$compiler" >/dev/null && [[ -x /workspace/tools/mingw/usr/bin/i686-w64-mingw32-g++-posix ]]; then
  compiler=/workspace/tools/mingw/usr/bin/i686-w64-mingw32-g++-posix
fi
"$compiler" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-misleading-indentation -Wno-cast-function-type -shared -static -Isrc -Ivendor -Ivendor/imgui src/plugin.cpp vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp vendor/imgui/backends/imgui_impl_dx9.cpp src/L4D2Visual.def -o build/L4D2Visual.dll -luser32 -ld3d9 -lcomctl32 -lgdi32 -luuid -ladvapi32 -lole32 -lwindowscodecs
