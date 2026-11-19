# Marvel: Crossover Legacy — Achievement milestone v0.3.1

## Implemented
- All **108** planned achievement definitions (72 main-game, 24 CHEAT EX, 12 collection-wide).
- `Collection::processVerifiedEvent` for emulator-originated launch, coin, match win/loss, two-player completion, arcade clears.
- First match wins for eight games, first credit, first win, 100/500 victories, six game launches, six clears, two-player match, first-clear Gallery notification, no-continue clears (where defined), and select team-survival achievements.
- Cheat and continue guards for no-continue clear trophies; attract-mode/demo rejection.
- Persistent achievement unlocks across restarts; one-shot popup notice queue.
- No ROM files have been edited.

## Outstanding (do not claim implemented)
- **No emulator integrated yet**: the event engine cannot receive live events until verified core adapters are written.
- Game-specific combo, gem, boss, perfect, hidden-character, and special-move triggers have **definitions only**, not automatic detection.
- All-Marvel / all-Capcom ten-win counters, no-continue-across-all-games tracking, secret code and cheat-use progress need extra persistable event data.
- EX2 achievements needing exact gameplay instrumentation have definitions only.
- Achievement notification animations and audio clip playback are not connected to the Win32 UI.
- Windows VS2022 GUI build requires validation on a Windows machine; portable C++ tests run on Linux.
- Existing `unlockAchievement` method remains trusted-integration API; do not expose it to player inputs.

## Test on Linux
```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic src/core.cpp tests/core_test.cpp -o /tmp/mcl-core-test && /tmp/mcl-core-test
g++ -std=c++17 -Wall -Wextra -Werror -pedantic src/core.cpp tests/gallery_test.cpp -o /tmp/mcl-gallery-test && /tmp/mcl-gallery-test
g++ -std=c++17 -Wall -Wextra -Werror -pedantic src/core.cpp tests/achievement_test.cpp -o /tmp/mcl-achievement-test && /tmp/mcl-achievement-test
```
