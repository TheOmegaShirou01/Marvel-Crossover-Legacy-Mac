# v0.3.8 — Emulator bridge foundation

- Added `EmulatorBackend` interface, `EmulatorBridge` lifecycle and active-session event gate.
- ROMs and BIOS files remain **external** and are never packaged/copied.
- Current backend is an interface; there is **no integrated emulator**, no real boot or gameplay hooks.
- A mock backend is used solely in tests. Do not mistake mock success for emulator compatibility.
- Trusted adapters must verify ROM revisions, map gameplay events from actual emulator state, and apply cheats independently.
- EX3 remains jukebox-only and cannot be prepared as a playable game.
- The prototype UI has not yet been connected to this bridge.
