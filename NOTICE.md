# Third-party sources

This project contains code from yeyunyyds/LDBDangoMusic, branch
`experiment/entity-attachments`, commit
`81acce6af86c8faf345de20ca4074b2af88f1bfe`, under GPL-3.0 (see LICENSE).

The eight headers in `vendor/dango/` other than json.hpp are copied unchanged
from that commit's `src/`. They implement the verified Source camera and
read-only entity inventory. Their game-build profile is not a generic Source ABI.

`vendor/dango/json.hpp` is nlohmann/json v3.11.3 (MIT); see the embedded notices
and `vendor/JSON-LICENSE.MIT`.

`vendor/l4n/l4n_plugin.h` is copied unchanged from the same reference project.
Its upstream source is yeyunyyds/L4D2_Dxvk_32to64_Bridge, commit
`cafc0442446c502a0de0ebb52d5e73ccaae67516`, `plugins/l4n/sdk/l4n_plugin.h`.
The supplied SDK header has no license declaration and is not relicensed here.

No game DLLs, models, textures or other game assets are included.

scripts/pe_reader.py is adapted from the same DangoMusic commit's
scripts/verify_source_binaries.py (GPL-3.0).

Native HUD headers (native_hud, native_menu_model, hud_menu_policy, hud_appearance,
menu_font, menu_glyphs) are from LDBDangoMusic main 54ed4046e3a5dc9ecc762b133714fea8090d432c.
menu_font.hpp adds the visual plugin's Chinese labels to the glyph set.
tests/native_menu_test.cpp is from the entity experiment commit cited above.
vendor/imgui is copied from that reference project; Dear ImGui is MIT licensed,
see vendor/imgui/LICENSE.txt. The native HUD design in that project credits the
MIT-licensed L4D2_Dxvk_32to64_Bridge implementation; its notice is retained in
vendor/BRIDGE-LICENSE.MIT.
The HUD navigation policy adds the visual plugin's animation settings child page.
