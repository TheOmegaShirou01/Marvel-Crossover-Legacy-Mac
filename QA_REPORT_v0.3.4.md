# v0.3.4 Logo Integration QA

- Original user files: byte-identical copies, verified for all eight logos.
- Normalized PNG and Windows BMP: 320x175 each, native aspect ratio preserved with contain-fitting (no distortion).
- Manifest: eight game IDs resolve.
- Windows `BS_BITMAP` card renderer source updated; Visual Studio / GUI runtime not tested in this Linux environment.
- 8/8 15-second previews and Earth video carried over unchanged.
- No AI graphics.

## Portable C++ regressions
- core_test: PASS: 8 games, 108 achievement definitions, persistence, validation, state slots, media scan
- achievement_test: PASS: 108 achievements, verified events, guards, unlocks, notifications, persistence\n
- gallery_test: Gallery package: 8 games, 192 character records; first clear unlocks once and persists. PASS

## Limitations
- Native Win32 GUI compile and visual verification pending.
- Actual flipped card animation/video playback still requires Windows testing.
- CHEAT EX unlock remains unimplemented in title-screen UI; EX view stays gated.
