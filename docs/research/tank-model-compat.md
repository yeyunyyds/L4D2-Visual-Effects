# Original Studio and Tank animation compatibility

The supplied original studiorender.dll is never executed by the offline tooling and is not redistributed. SHA256: 3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85. Its PE image is 0x453000 bytes versus 0x653000 for the previously verified Studio build. Factory, VStudioRender026 object, vtable slot 25, method prologue, stack cleanup and bone-stride samples are identical. Runtime binding still requires the exact selected SHA and instruction samples; client/engine verification remains unchanged. Reproduce with scripts/verify_render_profile.py --studio PATH.

Model identity is the validated Tank entity and full handle, independent of the MDL filename. Metadata keys include header address, checksum, version, name, length and sequence/include table metadata. A key change resets layer and timer history. A periodic refresh also catches changed label/activity semantics at reused addresses. Semantic ACT_* names take priority over labels. Numeric activity IDs are diagnostic only; no universal sequence number is assumed.

Included models use global sequences, so local table access is insufficient even when a global index happens to be numerically less than the root's local count. Passive mapping reads CStudioHdr -> virtualmodel_t::m_seq -> group/index -> CStudioHdr's cached group header -> local sequence descriptor. No opaque studiohdr virtualModel handle is dereferenced. CStudioHdr discovery is bounded to 16 KiB of the matched entity, 768 unique pointer probes, at most once per second per Tank while unresolved. Cache/vector pointers, counts, capacities, group bounds, root identity, descriptor backlinks and transaction rereads must agree. Any failure returns unresolved. No unknown virtual function, SetupBones, engine cache query, or GPU work is added.

Layout provenance: AlliedModders hl2sdk l4d2 public/studio.h and public/tier0/threadtools.h. Assumed x86 SDK layouts are structurally validated, not proven for every Mod or client binary. A renamed model using standard activity names should remain recognizable; arbitrary custom activity semantics, animation timing changes or incompatible layouts cannot be inferred reliably. Cooldown remains a live-ConVar-based estimate after observed onset, not a guarantee of AI readiness.

Validation: exact supplied DLL fingerprint and ABI samples, native synthetic shared-model mappings and malformed rejection, prior Mod-log replay. Those logs are not vanilla Tank evidence. Actual original Tank and alternate VPK runs remain required. Diagnostics and all active features remain off by default; no new SourceMod runtime dependency.


## preview2 investigation

The vanilla trace has checksum 1500020281, MDL version 49 and one local sequence; the earlier Mod has checksum 795675474 and 80 local sequences. Both include anim_hulk.mdl. The latest raw vanilla trace has 246 samples / 30.58s with weighted overlay indices 40/43/45, no resolved names and zero onsets. Old runs coexist in rotated logs and must be split by fingerprint.

SDK public/studio.cpp ResetVModel clears every m_pStudioHdrCache entry; GroupStudioHdr fills only the visited entry. Requiring cache[0] == root was incorrect: NULL is legal. Discovery now accepts NULL at zero, still rejects a non-null conflicting root, and validates wrapper root and virtual/cache/group vectors independently of the currently requested descriptor. An unresolved group does not conceal a valid wrapper location. Its offset, candidate/root-match counts and exact read stage are exposed in schema 10 and the in-game status page. This is a concrete code defect found via source inspection, not proof that the supplied game's field was located: the next runtime probe will establish the actual field offset.

Explicit session-only vanilla index backup is guarded by that fingerprint and only used for unresolved overlay entries 40/43/45. Synthesized names are marked experimental; real descriptors always win. It never interprets these indices as local sequences and never applies to the earlier Mod fingerprint. The option is false at startup and is never serialized. Toggling resets model generation / onset history. No new game hooks, unknown calls, GPU work or P2P initialization are involved.

preview2 validation: 24 native suites including font coverage; enhanced ASan/UBSan shared mapping tests cover NULL root cache, uncached selected group, discovery independent of descriptor readiness, original-fingerprint rejection and deduplicated fallback onset. Offline replay of the latest 401-row mixed log yields 0 onsets without backup, 13 backup onsets/13 stopwatch starts with no duplicate restarts; only the 246-row vanilla segment matches the backup fingerprint. The earlier 782-row resolved Mod replay remains 29/29. Offline matching is not hit/miss ground truth.


## preview3: automatic broad sequence table matching

User requested removal of the backup test switch and fingerprint restrictions. All 80 label/activity/flags entries in uploaded hulk.mdl, hulk_dlc3.mdl and hulk_l4d1.mdl were parsed with validated descriptor backlinks and found identical. Checksums differ: 795675474, 1674290501, -209254859; the observed current runtime root checksum is 1500020281 with one local sequence. This does not prove which files are modified versus original, and those three files do not provide the missing shared runtime mapping.

The copied metadata table is used automatically for matched Tank entities when a real descriptor is unresolved, for both primary and all sampled overlay sequences. There is no separate test switch, session option, fingerprint requirement or persisted flag. Tank feature defaults remain off. Actual model descriptors win. The status page distinguishes table matching from descriptor resolution; schema 11 uses sequenceSource=known-index-table/model-descriptor. Indices outside 0..79 remain unresolved. This is a deliberately broad heuristic; global index reordering by models/Mods may misclassify.

40/43/45 retain their known attack labels; 46/47 use ACT_HULK_ATTACK_LOW and are recognized as attack animations. 48..51 have throw activities and must enter throw observation without creating a punch onset or invented throw cooldown. 58 local entries are STUDIO_OVERRIDE forward declarations; static labels do not prove their animation data or global runtime ordering. The three raw MDLs are never bundled.

Source metadata SHA256:
- hulk.mdl: 1e30381ca37aeb66cc26b7c08f35cb37545ca4a8eeb1fcb284afd9a639a2c347
- hulk_dlc3.mdl: 5c8d1fc3102c5fddd7e8c90be8788f405dda6c1a4c26defcb17cc400556448be
- hulk_l4d1.mdl: ec392f900f4ef5510df86d33210f0bfe8f6569ddb238eac7e7c5729e2f24359a

Offline replay --known-table of the newest 419-row raw original trace produces 9 layer onsets and 9 stopwatch starts (40=2,43=3,45=4), zero duplicate restarts. The 401-row mixed trace produces 22/22. These are automated candidate observations, not independent ground-truth hit counts. Live shared-container discovery still rejects the header-cache vector and has not been fixed in this preview.


## 0.7.4 formal minimal core

User confirmed original Tank, the tested Mod and original uploaded Studio DLL work and authorized a formal release using index matching only. The production plugin removes all direct/local MDL and virtual model mappings, entity-memory container discovery, animation event factory/receive-proxy taps and diagnostic file writers. Network main/overlay indices resolve directly via knownTankSequence, independent of model draw visibility. Render callbacks now only maintain validated entity-following positions. The test animation status page is removed; layout preview and normal current-state menus remain. Old logging/markKey fields are ignored and removed when settings are saved. Historical research descriptions above are no longer production code.

Default Tank enabled=false, draw=true and all four visual display preferences=true; style values unchanged. Other active features default off. Windows x86 build, 23 native suites and original/Mod offline traces validate the retained core. The installer root contains only bin and README.md, and binary checks exclude Tank diagnostic writer paths and event descriptor strings.
