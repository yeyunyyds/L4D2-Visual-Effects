# Third-party sources

This project contains code from yeyunyyds/LDBDangoMusic, branch
`experiment/entity-attachments`, commit
`81acce6af86c8faf345de20ca4074b2af88f1bfe`, under GPL-3.0 (see LICENSE).

The baseline camera and entity headers in `vendor/dango/` were imported
from that commit's `src/`. Later imports and local adaptations are listed below.
They implement the verified Source camera and read-only entity inventory.
Their game-build profile is not a generic Source ABI.

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

HUD glyphs, keyboard pagination and entity selection menus are adapted for this plugin.

Crosshair-nearby identification uses entity_follow.hpp and scene_attachment.hpp
from LDBDangoMusic experiment/entity-follow-v094 commit
ce15a74704b943db33356eaba5153152f712daa1 (GPL-3.0).
entity_inventory.hpp is updated from that commit; aim_selection.hpp adapts
the candidate ranking for Class ID selection without the follow-mode category whitelist.
This ranking uses network poses and does not test wall occlusion.

source_entities.hpp additionally captures m_fEffects for hidden-target rejection in nearby identification.

Entity inventory also publishes a bounded Class ID/name catalog by following verified module-owned ClientClass descriptors.
