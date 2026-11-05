# L4N-only Tank prediction: initial client experiment

The feature lives on experiment/tank-threat; v0.7.2 is unchanged. No SourceMod,
Left4DHooks, new Source engine virtual dispatch or server plugin is required.

Data comes from the existing verified raw entity reader/RecvTables (only baseclass
and the explicitly embedded DT_CollisionProperty), and CPU-only metadata observation
inside the existing verified model hook. The observer never issues D3D calls.
ConVar probing uses bounded read-only PE section scans, recognized x86 layout and
strict name/parent/replicated-flag/string-float consistency validation. Unknown or
ambiguous objects are rejected. Live values refresh independently; missing runtime
range/arc means geometry is not drawn. The in-process server module, when present (module presence is not proof of hosting),
is only read for ConVar data. It is never hooked or called.

Names/field availability are hypotheses to validate at runtime. m_lookatPlayer is a
network look-at hint, not proof of the server NextBot's intention. Enemy fields are
reported separately when actually exposed. Predicted facing targets require a clear
score winner; ties, stale handles, dead/dormant players produce unknown. Body yaw
fallback must be labeled. Stationary heading is an observation, never an automatic
post-rock reach reduction. Animation metadata uses bounded relative offsets and
header backlinks to reject unsupported layouts; gesture-only attacks may remain
unknown. Weapon cooldown differences use entity simulation time as an estimate.

All graphics stay inside the stable Present BeginScene/EndScene transaction, with
state restoration and the existing backbuffer backup. No new depth-buffer borrowing.
The arc interpretation and hull expansion are an initial visualization model, not a
reverse-engineered full L4D2 hit solver. Prediction assumes current velocity only;
AI/player future input, vertical/hull/occlusion checks, server plugins and lag
compensation remain unmodeled. This is particularly important around Tank throws.

The JSONL worker output is bounded and contains the missing/available data and
survivor position/health required for later local single-player calibration. Do not
claim high hit accuracy until actual Tank punch and post-rock trials are available.

Public research references (not runtime dependencies):
- Valve cvar list, tank_swing_range/arc, replicated flag:
  https://github.com/dgibbs64/Valve-Server-cvarlist/blob/a9ed52d9fc5d6daebdea843bd18f482d122b750d/l4d2-cvarlist.txt
- SendProp m_lookatPlayer usage:
  https://github.com/dvander/sourcepawn-corpus/blob/f95422d7f5337c9e433847f84e5735fd91d94012/forums/x/z/k/xZk/l4d_random_tank_rock_vZ_2661054.sp

No SourcePawn code is linked or installed by this experiment. Looking up server
hooks during research does not expose those hooks to the client. No VAC guarantee.
