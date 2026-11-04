#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
for suite in stretch studio_pose native_menu animation animation_config render_options aim_selection entity_policy; do
  "${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined -Isrc -isystem vendor -isystem vendor/dango "tests/${suite}_test.cpp" -o "build/${suite}_test"
  "./build/${suite}_test"
done
