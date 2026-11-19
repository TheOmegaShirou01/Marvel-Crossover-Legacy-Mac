# v0.3.12 — ROM audit preflight (source only)

Adds a **VERIFY SELECTED ROM (MAME)** button to the arcade selection window. It builds a strictly allowlisted command for the currently selected title and runs the configured MAME executable with `-verifyroms <game_id> -rompath <rom_directory>`.

## What it does
- Supports the six Marvel games and the two unlocked EX titles; **Street Fighter EX3 is never a playable target**.
- Checks expected per-game archive and necessary NAOMI/ZN BIOS ZIP presence before invoking MAME.
- Runs MAME's own ROM verification in a child process without a command shell. A 15-second timeout applies.
- Reports pass/fail/timeout/start failure; reports a pass **only** on MAME exit code 0.
- Does not touch the originals; external or local `Runtime/ROMs` stays outside all source checkpoints.

## Limitations
- This is **not** emulation, a Windows build test, a completed boot test, a verified achievement hook, or an implementation of the custom pause/cheat inputs.
- MAME's exit code is used for audit success. Detailed chip-level audit output remains available when running MAME directly and is not displayed by the app in this checkpoint.
- The verification button runs synchronously and may pause UI responsiveness for up to 15 seconds. Making audit execution asynchronous is future work.
- CPS-2 parent/clone archive sets may need additional compatible archives/security keys beyond file-presence checks.
- The launcher is a separate configured MAME executable, not an embedded emulator.

## Local configuration
Place `mame_executable.txt` beside the app with your MAME executable path on the first line. Use the local `Runtime/ROMs` folder or `rom_directory.txt` to select an external path. ROMs, BIOS, and MAME binaries are **not** shipped in this source archive.

## Tests
11 portable C++ test binaries built using `g++ -std=c++17 -O0 -Wall -Wextra -Werror` and passed. Windows GUI compilation, actual MAME CLI audit, and ROM boots have not been tested in this container.
