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

0.5.1 adds category rules, an explicit identification menu and a default-off
proximity experiment. Actor health/life-state network fields are captured
for trigger validation. HUD pagination uses minus/equal again; arrows remain
reserved for the existing menu-position editor.

0.5.2 uses a predefined type catalog based on DangoMusic's declared entity
filters, with name-based settings available before entities spawn. The proximity
experiment now creates a per-entity distance mask around target entities;
viewer position is not used.

0.5.3 survivor roles use the m_survivorCharacter ordering from LDBDangoMusic
experiment/npc-dialogue, commit 5d9869cf13e7eec9e82c8ff78f26ad31d03ab839,
src/dialogue.hpp and src/source_entities.hpp. The local entity collector and
RecvTable field allowlist include this role property.

Special-infected matching also reads m_zombieClass for infected players.
The 1..8 zombie-type mapping was checked against the public L4D2ZombieClass
enum in SilvDev/Left4DHooks, sourcemod/scripting/include/left4dhooks_stocks.inc.
These are zombie-type values, not network Class IDs.

The wall-picture module reuses wall_panel.hpp and source_trace_contract.hpp
from LDBDangoMusic main commit 54ed4046e3a5dc9ecc762b133714fea8090d432c
(GPL-3.0). image_trace.hpp adapts the same commit's source_walls.hpp static
TraceRay binding, world-only filter, and result checks.
Image decoding uses the Windows Imaging Component system API; no third-party
image decoder or game assets are bundled.

Steam P2P transport in src/spray_steam.hpp is adapted from LDBDangoMusic
experiment/steam-progress-sync commit ca9f3bcc70a2f0ea982526bf22de31d42c871100,
src/sync/steam_transport.hpp (GPL-3.0). It preserves the borrowed game Steam
context and explicit x86 CSteamID return adapter. Visual Effects uses channel
45632 and its own versioned spray protocol, not the music synchronization wire
format. PNG encoding and SHA-256 use Windows WIC and CryptoAPI.

0.7.1 same-game spray discovery adapts src/source_roster.hpp and
src/sync/roster.hpp from LDBDangoMusic experiment/steam-progress-sync commit
6d327fcabe322771e5737165862a9db455998860 (GPL-3.0). The statically verified
144-byte player-info contract and four-human roster bound are preserved.
Sprays use cohort-scoped reciprocal handshakes, without music host election.

Entity scaling in src/entity_transform_runtime.hpp and the matrix/pivot design
in src/entity_transform.hpp adapt Skeeto (Copyright (C) 2026 regoists), GPL-3.0,
commit 3598bd3afc32bccb98ac4715b2fa53cd6fe883b7:
src/skeeto/core/skeeto_proxy.cpp, ViewModelScaleBones / Hooked_ViewModelBuild /
WModelScale* / Hooked_WModelBuild / HookedModelScaleProxy.
https://github.com/regoists/Skeeto
This is an independent L4N integration. It extends categories, XYZ deformation
and 128/256 bones, adds handle/model identity, owned-value restoration,
explicit proxy teardown, and a Studio-render fallback; no Skeeto binaries,
marks or game assets are bundled. The existing project GPL-3.0 LICENSE applies.

Tank range-warning native outline code in src/tank_outline.hpp adapts the
CampaignOutline* glow ownership and native color-proxy route from the same
Skeeto commit above (Copyright (C) 2026 regoists), under GPL-3.0. It validates
the exact client image/native entry points and embedded glow layout, preserves
existing glows and restores only values still owned by this feature.
