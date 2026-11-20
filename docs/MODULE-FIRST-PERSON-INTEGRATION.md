# First-person body module integration

The first-person body feature is registered through the modular host and
uses the existing native game rendering path. It is **off by default**.
This branch contains no image, spray, or Steam P2P modules.

## Module boundaries

- `feature_composition.hpp` registers feature adapters.
- `module_host.hpp` exposes an optional Studio-decoration continuation.
- `source_render.hpp` invokes the registered decorator after entity pose
  modification, with exactly one original drawing continuation.
- `first_person_body_module.hpp` owns the menu, configuration, frame checks,
  native visibility cleanup and draw decoration.
- Device ownership is not required by first-person body processing; the
  earlier GPU height-clipping subsystem has been removed.
- `logging.level` defaults to `info`; periodic status snapshots require
  Debug.
- `邦友的酒` is the first item within the first page of entity scaling;
  it remains a separate runtime module.

## Optional third-person view compatibility

The menu offers one default-off option for managing first-person body display
when switching between first-person and third-person views. It changes this
feature's own rendering behavior; no external camera plugin is distributed
or required. Earlier compatibility settings are still read during migration, while new
saves use `thirdPersonCompatibility`.

## Validation boundaries

The user has confirmed the view-switching and movement behavior on the
preceding test build. Changes in this commit are source and documentation
updates only; no new DLL or game-run verification is claimed.
