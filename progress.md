Deadline: **Monday, November 2, 2026 @ 11:59 PM**

## Breaks a rule (fix first)

- [x]  Set `BASE_MAX_HP = 1` in Config.h — spec says ONE enemy reaching the base = game over
- [x]  Set `START_COINS = 150` (enough for 3 Melee; only Melee unlocked at start)
- [ ]  Ask TA: 150 coins also unlocks Defense (3× Melee cost) — how should that be handled?
- [x]  Remove coin auto-collect — coins must be collected only when a player walks into them
    - Coins stay on the grid where the enemy died
    - Collected only on bounding-box overlap with a player in the same lane, then added to the total and removed
    - Remove any timer, magnet, or click-based auto-collect logic
    - Coin counter and card lock/unlock states update immediately on collection

## Missing features

- [ ]  Main Menu: Start (cutscene → game), Exit, Sound Settings
- [ ]  Intro cutscene: 10–20 s, animated, ≥1 player + ≥1 enemy, short dialogue, explains what the base is and why it matters (check if a teammate owns this)
- [x]  Pause menu: add "Return to Main Menu"
    - Opened with Escape; sits alongside Continue and Restart
    - Fully resets game state (players, enemies, projectiles, coins, wave index, timers, lane selections, pause flag) so Play replays the cutscene
    - Match existing button style, hover, and click handling
- [x]  Win/Lose screen: add "Return to Main Menu"
    - Sits alongside Restart
    - Same full reset as the pause menu option; Play starts a fresh game with the cutscene
    - Match existing button style, hover, and click handling
- [x]  Rocket hit animation (small explosion) before it disappears
    - On collision: apply damage once, stop the rocket, play a ~0.3 s explosion at the impact point, then remove it
    - Built from several primitives (expanding/fading flash circle + sparks/debris moving outward), not one shape
    - Explosion deals no extra damage and doesn't block other projectiles
- [ ]  Add ≥5 themed environment elements (lab machinery, pipes, crates, warning signs, screens…)
- [x]  Make the home base distinct and detailed (not just a few rectangles)

## Sound bonus (optional)

- [x]  Sound Settings menu reachable from the Main Menu (volume up/down + mute)
- [x]  Add ≥4 sound effects (attack, death, placement, coin pickup…) using `Audio::playSfx`

## Verify

- [x]  Characters.h stats — HP: Melee 5, Ranged 5, Defense 7, Easy 5, Medium 5, Hard 7
- [x]  Damage: every player attack = 1; enemies = 1 except Hard = 2
- [x]  Easy attacks from above (duck), Medium from below (jump), Hard runs in for melee
- [x]  UI.cpp: locked cards are grayscale and ignore hover

## Submission

- [ ]  Get the professor's AI-use permission in writing (email/forum post)
- [ ]  Clean build with zero errors (compile errors = zero grade)
- [ ]  Zip only .h/.cpp files as `[Team name]-[Member name].zip`, upload to Drive, grant access
- [ ]  Record 2–5 min video with voiceover: GUC logo → course & semester → project name → names → gameplay → thank you
- [ ]  Submit zip + video link through the form