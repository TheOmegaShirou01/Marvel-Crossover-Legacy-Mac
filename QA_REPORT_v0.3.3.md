# Source Checkpoint v0.3.3 QA

## Verified in this Linux environment
- Core C++17 regression tests: PASS (108 achievement definitions, persistence, 8 game records).
- Achievement C++17 regression tests: PASS.
- Gallery C++17 regression tests: PASS (192 character records, first-clear unlock).
- All eight game preview files exist under `Previews/<game-id>.mp4` and probe as video with audio, approximately 15 seconds each.
- The uploaded original 4K rotating Earth MP4 is included unaltered as `earth_background.mp4` (~32 seconds).
- Build script updated to include `src/arcade_select.cpp`, DirectShow system libraries, globe video copy, and preview asset copies.
- Asset SHA-256 manifest included.

## New behavior coded but **not** Windows runtime-verified
- Standalone Win32 ARCADE SELECT window, launched from the existing feature browser.
- DirectShow video rendering of globe in the background and selected game's combat preview inside its card.
- Card compress/expand flip transition, left/right/up/down keyboard navigation and click selection.
- Default six Marvel titles only; EX videos are present but CHEAT EX remains locked, waiting for a genuine title-screen code state.

## Known limitations
- No Windows Visual Studio 2022 compilation or actual DirectShow playback was possible here; runtime rendering and codec availability remain UNVERIFIED.
- No actual game ROM launching, embedded emulator, authentic title-screen code state, logo-art integration, or responsive fullscreen layout. Game launch button intentionally disabled.
- Audio ducking and MVC1 menu music are not yet connected; playback sound comes from preview videos.
- The previous feature browser has its own old external playback button as a fallback.
- Globe video is user-uploaded; check redistribution rights prior to publishing.
- No AI-generated assets used.
