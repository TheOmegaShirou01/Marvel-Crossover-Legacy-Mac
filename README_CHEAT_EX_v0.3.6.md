# CHEAT EX v0.3.6 — configuration checkpoint

Each of the two playable hidden EX games now has a settings dialog with independently saved switches for Infinite Health, Infinite Power, Infinite Time, and Unlock All Hidden Characters.

The button appears **only inside the unlocked CHEAT EX selection**. Settings default OFF and are saved to `MCL_UserData/cheat_ex_settings.txt`. Street Fighter EX3 is excluded from the playable roster, cheat settings, and automatic main music rotation; its planned jukebox soundtrack remains separate.

IMPORTANT: These are requested cheat preferences only. No emulator is connected, and no health/meter/timer/roster memory patches are executed yet. Hidden fighter selection has not been verified against the supplied ROM revisions. No ROM files were modified.

Windows GUI compile and actual playback require Visual Studio 2022 runtime validation. Portable C++ settings and existing regression tests are in tests/.
