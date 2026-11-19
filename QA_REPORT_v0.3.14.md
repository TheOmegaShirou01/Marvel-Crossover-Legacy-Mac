# Marvel: Crossover Legacy — v0.3.14 QA

## Implemented
- Windows pause-menu **preview** window, opened from Arcade Select when no emulator session is running.
- Continue / Exit Game / Are You Sure? / Yes / No decisions backed by the v0.3.13 `PauseControls` state machine.
- `No` immediately closes the preview and returns to the caller (does not reopen the previous menu).
- `Yes` returns an exit-request outcome from the preview, but DOES NOT terminate a real emulated game.
- Explicit warning and disabled preview when an emulator session is running, to avoid misleading claims about pausing MAME.
- Visual styling is a prototype, not the final MVC2 victory typography or a dimmed in-game overlay.

## Regression verification
- 12/12 portable C++ regression tests passed using g++ C++17 -Wall -Wextra -Werror. The first five passed individually; the remaining seven passed when linked to precompiled core objects.
- Source package audited to exclude ROMs/BIOS/emulator executables.
- Actual Windows compilation, user input interception, and MAME pause/resume are NOT verified. Windows pause-preview GUI source cannot be runtime tested in this Linux environment.

## Known limitation
- This checkpoint is a safe **preview-only UI**. MAME is launched as a separate process; full-screen input capture and pausing require a cooperative emulator integration, still pending.
