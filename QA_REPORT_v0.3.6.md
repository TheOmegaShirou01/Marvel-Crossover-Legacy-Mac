# v0.3.6 QA — CHEAT EX preferences + bundled user-supplied ROMs

## Verified
- 17/17 ROM/BIOS ZIPs copied from user-supplied uploads, CRC verified, SHA-256 hashes recorded and verified.
- Archive manifest covers all eight playable games, regional/parent/key variants, and NAOMI/ZN-1/ZN-2 BIOS archives.
- No Street Fighter EX3 playable ROM is included.
- Windows build script now checks missing original ROM/BIOS ZIP filenames before copying them to `bin/ROMs` on a successful compile.
- CHEAT EX Plus and EX2 Plus configuration settings (Health, Power, Time, Unlock Hidden Characters) persist separately, default OFF, reject EX3.
- Five portable C++ tests compiled with -std=c++17 -Wall -Wextra -Werror and PASSED: core, achievements, gallery, CHEAT EX code, CHEAT EX settings.

## Not yet verified or implemented
- Windows Visual Studio compilation or GUI behavior (no Windows runtime available here).
- Emulator core integration or real ROM boots; ZIP integrity isn't arcade-emulation compatibility.
- Activation of cheat memory patches or playable boss unlocks; these are configuration-only controls.
- Permanent controller-mapped CHEAT EX code entry on the final title screen.
- The existing selection prototype may still need GUI-level corrections after Windows build.

## Rights / distribution
- User-provided arcade game and BIOS dumps are included only in this private checkpoint. Do not redistribute them without permission.
