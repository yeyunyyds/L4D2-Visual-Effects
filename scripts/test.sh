#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
for suite in rock_prediction rock_settings defaults stretch studio_pose native_menu animation animation_config render_options aim_selection entity_policy survivors specials image_scene spray_protocol spray_roster spray_input tank_model tank_sequences tank_animation_layers tank_layer_attack tank_visual tank_cooldown tank_stopwatch; do
  "${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined -Isrc -isystem vendor -isystem vendor/dango "tests/${suite}_test.cpp" -o "build/${suite}_test"
  "./build/${suite}_test"
done

"${CXX:-g++}" -std=c++17 -O1 -g -fsanitize=address,undefined -Isrc -isystem vendor -isystem vendor/dango -Ivendor/imgui tests/menu_font_test.cpp vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_tables.cpp vendor/imgui/imgui_widgets.cpp -o build/menu_font_test
./build/menu_font_test
