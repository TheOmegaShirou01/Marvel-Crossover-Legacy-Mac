# v0.3.15 — Guarded MAME pause hotkey (source only)

Adds a `SEND MAME PAUSE KEY (F5)` button to the Arcade Select view, enabled during a running MAME session. It finds a top-level visible window owned by the launched MAME process, checks/attempts foreground focus, and sends a Windows F5 down/up event using `SendInput`.

MAME's current documented default pause key is F5 (not the legacy P mapping). This is **best-effort**, requires a compatible keyboard/UI mapping, and is not a verified pause-state protocol. The frontend does **not** mark the emulator paused or display the real in-game overlay. Pressing the button a second time requests another F5 toggle. If the user remaps pause in MAME, the key will not be correct.

There is NO global keyboard hook and NO interception of controller inputs. The user's START+SELECT and three-second quick-exit remain logic-only, and original MAME gameplay input is untouched. A proper pause-state acknowledgement and controller bridge remain prerequisites to implementing the real overlay.

ROMs and BIOS are not included in source checkpoints. EX3 remains jukebox-only.
