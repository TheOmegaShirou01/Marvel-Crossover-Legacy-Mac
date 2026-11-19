# Marvel: Crossover Legacy v0.3.15 QA

## New code
- Added guarded MAME F5 pause/resume hotkey request using Windows `SendInput` targeted to the process window associated with the launched MAME PID.
- Added a **SEND MAME PAUSE KEY (F5)** button available during a running emulator session.
- The application reports only that the hotkey was sent, **not** that the game is paused; the emulator bridge's `pause()` continues to refuse unverified pause state changes.
- Includes pause-request gating tests for no session, no process window, bad foreground, blocked input, and accepted injection.
- No global input hook, ROM modification, or third-party assets added.

## Testing performed
- g++ C++17 `-Wall -Wextra -Werror`: **13/13** portable test suites PASS.
- Full native Windows GUI compilation: **NOT TESTED** (MSVC unavailable in this environment).
- MAME live pause/resume: **NOT TESTED**; Windows focus restrictions, MAME user key remaps and raw input settings may prevent input delivery.
- Actual in-game dimmed pause overlay, controller hotkey interception, verified pause-state detection and three-second quick exit in gameplay: **NOT IMPLEMENTED**.
- Source ZIP archive checked for excluded ROMs, BIOS and binary executables.

## Regression and operational limitations
- Real-game controls are still passed to MAME directly. The new F5 button is an experimental request, **not** the finished START+SELECT command.
- The current MAME default pause key is documented as F5; older versions and user configurations can differ.
- EX3 remains music-only. No ROM/BIOS files included in this checkpoint.
