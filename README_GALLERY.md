# Marvel: Crossover Legacy — Arcade Gallery v0.2

The **Gallery/arcade_gallery.mclgallery** file is a project-specific binary container (MCLGAL01), not a ROM or executable. It includes eight researched game histories and 192 character reference entries. The binary is referenced by the gallery screens in `src/core.cpp` and read from `Gallery/` next to the EXE.

**Unlock requirement:** On the first real arcade clear (from an emulator-confirmed `recordEvent(gameId, "clear", 1)`), `galleryUnlocked()` changes to true and `consumeGalleryUnlockNotice()` supplies one popup request to the hosting UI. Message: **GALLERY UNLOCKED!** Motto: **THE LEGENDS LIVE ON!** This frontend prototype cannot generate such events without an emulator, so the UI remains locked until genuine emulator integration.

**Artwork status:** The gallery source notes link to official and third-party research, but third-party copyrighted art and arcade ROM sprites **have not been repackaged**. Character portrait slots remain pending legitimate extraction from the user's arcade game data or separately licensed images. No fake characters or synthetic portraits are used.

**Development status:** The Windows GUI remains a prototype. The data reader is integrated and cross-platform automated tests compile under g++. The Windows executable has NOT been compiled or tested in this environment. Do not call the game or live achievement hooks complete.

**Sources:** Browse `gallery_manifest.json` in the accompanying research bundle for per-game source URLs. Read the research contents as short original editorial summaries, not as official Capcom biographies. Content specifics vary by game and ROM region; unusual playable boss behavior needs validation.
