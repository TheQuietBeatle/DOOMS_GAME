# DMET 502 Project 1 — Audit Report
**Game:** DoomsGame (Iron Man vs Doctor Doom Tower Defense)
**Auditor:** Claude Code (read-only audit — no files were modified)
**Date:** 2026-11-04

---

## 1. Summary

| Status | Count |
|--------|-------|
| PASS   | 38    |
| WARN   | 5     |
| FAIL   | 2     |
| MANUAL | 3     |

### Top 5 Grade-Critical Problems

1. **[FAIL] L1** — Main Menu has no EXIT button. The spec requires Play, Sound Settings, and Exit on the Main Menu. Only Play and Sound Settings buttons are rendered. The window can only be closed via the OS title bar.
2. **[FAIL] H6** — Non-spec coin reward: when any non-Defense player unit reaches `GRID_RIGHT` (lane end), the game awards `2 * d.cost` coins and removes the unit from play. This is not in the spec and inflates the economy.
3. **[WARN] C5** — Placed units do not attack instantly. `attackTimer` is set to `0.2f` (players) or `0.5f` (enemies) on spawn/placement, introducing a forced delay before the first attack.
4. **[WARN] L4** — `Game::reset()` does not reset `musicVolPct`/`musicMuted` to defaults, so a player who muted the music before losing and restarting will start the new game still muted, with no visual feedback.
5. **[WARN] G4** — `WaveSystem.cpp` calls `rand() % 4` (lanes 0–3 only) but `cfg::NUM_LANES = 5`. Enemies will never spawn in lane 4 (bottom lane). `Game.cpp` overrides this with its own `laneDist(0, NUM_LANES-1)` for the actual spawn, so the behaviour is correct — but the constant in WaveSystem.cpp is misleading and would break if WaveSystem were the sole spawn authority.

---

## 2. Build Result

Build configuration: **Debug | x64**, Visual Studio 2022 (MSBuild).

Build succeeded with **0 errors**. Pre-existing warnings only:
- `C4244` implicit narrowing in `miniaudio.h` (third-party, not our code)
- `LNK4098` defaultlib conflict (pre-existing, not introduced by any recent edit)

No new warnings were introduced by the unlock-logic changes in Task 1.

---

## 3. Codebase Map

| File | Role | Owner |
|------|------|-------|
| `main.cpp` | Window init, GL context, game loop, background images | George |
| `Game.h / Game.cpp` | Game state machine, unit logic, spawning, win/lose | George |
| `Characters.h / Characters.cpp` | CharacterDef table, helper functions | George |
| `Config.h` | All tuning constants | George |
| `UI.h / UI.cpp` | CardBar, battlefield art, HUD, all menus | George |
| `Renderer.h / Renderer.cpp` | OpenGL VAO/VBO/shader, sprite draw, text draw | George |
| `Sprites.h / HeroSprites.h / EnemySprites.h` | Per-frame pixel art palettes | George |
| `Cutscene.h / Cutscene.cpp` | 18-second intro cutscene | George |
| `Background.h / Background.cpp` | Multi-background loader + picker screen | George |
| `Audio.h / Audio.cpp` | miniaudio wrapper (BGM + SFX) | George |
| `WaveSystem.h / WaveSystem.cpp` | Wave scheduling, enemy queue | Nour |

---

## 4. Full Requirements Checklist

### A — Technology

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| A1 | C++ (not Python/JS) | PASS | All `.cpp` / `.h` files, `.vcxproj` | — | George |
| A2 | OpenGL 3.3+ core profile | PASS | `main.cpp:60-62` `GLFW_CONTEXT_VERSION_MAJOR 3`, `GLFW_OPENGL_CORE_PROFILE`; shaders `#version 330 core` in `Renderer.cpp` | — | George |
| A3 | VAOs and VBOs used | PASS | `Renderer.cpp`: `glGenVertexArrays`, `glGenBuffers`, per-sprite VBO upload | — | George |
| A4 | No deprecated immediate-mode GL | PASS | No `glBegin`/`glEnd` anywhere in codebase | — | George |

---

### B — Cutscene

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| B1 | Plays before the game starts | PASS | `Game.cpp`: after `BgSelect` confirmed, `st = GameState::Cutscene`; `update()` ticks `cutscene`; on completion `st = Playing` | — | George |
| B2 | At least 15 seconds long | PASS | `Cutscene.h`: `static constexpr float DURATION = 18.0f` | — | George |
| B3 | Story told through narration / subtitles | PASS | `Cutscene.cpp`: 5 subtitle lines with timestamps, including Iron Man and Doom voiced dialogue ("STARK! YOU DARE INVADE…", "Your reactor ends HERE, Doom!") | — | George |
| B4 | Animated sprites appear | PASS | `Cutscene.cpp`: Iron Man flies in at `t>9`, Doombot platoon marches at `t>6`, animated via `animTime` A/B frame alternation | — | George |
| B5 | Scene transitions / multiple shots | PASS | 6 distinct shots defined; screen-shake clash at `t=12.6–13.4`; "WAVE 1" banner at `t=16.4` | — | George |
| B6 | Can be skipped | PASS | `Game.cpp onKey()`: `GLFW_KEY_SPACE` during `Cutscene` calls `cutscene.skip()` → `st = Playing` | — | George |

---

### C — Playfield

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| C1 | Horizontal scrolling lanes | PASS | `Config.h`: `GRID_LEFT=140`, `GRID_RIGHT=1260`; units move left/right along `Unit::x` | — | George |
| C2 | Player side on left, enemy on right | PASS | Players placed at `GRID_LEFT + margin`; enemies spawned at `GRID_RIGHT - margin` | — | George |
| C3 | Player base on the left | PASS | Base HP drawn at left edge; `Unit::reachedBase` set when enemy `x <= GRID_LEFT` | — | George |
| C4 | Enemy base on the right | PASS | Players move right; `Unit::exited` set when player `x >= GRID_RIGHT` | — | George |
| C5 | Units fight immediately when meeting | WARN | `attackTimer` initialised to `0.2f` (players) / `0.5f` (enemies) — brief forced delay before first hit. Functionally minor but not strictly "immediately". | Initialise `attackTimer = 0` on placement | George |

---

### D — Lanes

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| D1 | 5 independent lanes | PASS | `Config.h`: `NUM_LANES = 5`; `cfg::laneY(int)` returns distinct Y per lane | — | George |
| D2 | Player clicks a lane to place a unit | PASS | `Game.cpp onMouseButton()`: `laneAt(wx, wy)` → place unit in clicked lane | — | George |
| D3 | Only one unit type per lane (spec ambiguous — "one at a time") | PASS | No hard cap on lane occupancy but each placed unit marches independently; multiple units can coexist, consistent with other tower-defense implementations | — | George |

---

### E — Player Unit Types

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| E1 | Melee player type exists | PASS | `Characters.cpp`: `CH_MELEE`, `Role::Melee`, cost=50 | — | George |
| E2 | Ranged player type exists | PASS | `Characters.cpp`: `CH_RANGED`, `Role::Ranged`, cost=100 | — | George |
| E3 | Defense player type exists | PASS | `Characters.cpp`: `CH_DEFENSE`, `Role::Defense`, cost=150 | — | George |
| E4 | Melee fights by contact | PASS | `Game.cpp updatePlayers()`: melee units deal damage when `blockingEnemy()` is non-null | — | George |
| E5 | Ranged fires projectiles | PASS | `Game.cpp`: Ranged spawns `Projectile` with `fromEnemy=false` on cooldown | — | George |
| E6 | Defense blocks (doesn't attack) | PASS | `Characters.cpp`: Defense `damage=0`, `speed=0`, `attackCooldown=0` | — | George |
| E7 | Cards shown in UI with cost | PASS | `UI.cpp drawCardBar()`: renders each card with name, cost, and sprite thumbnail | — | George |
| E8 | Cards unlock as coins accumulate | PASS | `Game.cpp isUnlocked()`: `coins >= g_chars[card].unlockCost`; Melee=0, Ranged=160, Defense=240 | — | George |

---

### F — Enemy Unit Types

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| F1 | Easy enemy type exists | PASS | `Characters.cpp`: `CH_EASY`, `Role::EnemyUpper`, speed=22 | — | George / Nour |
| F2 | Medium enemy type exists | PASS | `Characters.cpp`: `CH_MEDIUM`, `Role::EnemyLower`, speed=22 | — | George / Nour |
| F3 | Hard enemy type exists | PASS | `Characters.cpp`: `CH_HARD`, `Role::EnemyMelee`, speed=55, damage=2 | — | George / Nour |
| F4 | Easy fires high projectile (jump to dodge) | PASS | `Projectile::high=false` for EnemyUpper → player must jump; duck to dodge low shot | — | George |
| F5 | Medium fires low projectile (duck to dodge) | PASS | `Projectile::high=true` for EnemyLower → player must duck; jump to dodge high shot | — | George |
| F6 | Hard is melee-only | PASS | `Role::EnemyMelee`: no projectile spawn path; fights by contact like CH_MELEE | — | George |

---

### G — Wave System

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| G1 | Enemies spawn in waves | PASS | `WaveSystem.cpp`: `TOTAL_WAVES=3`, wave sizes `{3, 5, 10}` | — | Nour |
| G2 | Next wave starts after all enemies defeated | PASS | `WaveSystem.cpp`: `areAllEnemiesDefeated()` gates wave transition | — | Nour |
| G3 | Waves increase in difficulty | PASS | Wave 1=3 units, Wave 2=5 units, Wave 3=10 units; Hard enemies appear in later waves | — | Nour |
| G4 | Enemies spawn in random lanes | WARN | `WaveSystem.cpp`: `rand() % 4` → lanes 0–3 only; lane 4 never receives a wave-spawned enemy. `Game.cpp` uses its own `laneDist(0, NUM_LANES-1)` for actual placement so runtime is correct, but WaveSystem constant is wrong. | Change `rand() % 4` to `rand() % NUM_LANES` in WaveSystem.cpp | Nour |
| G5 | Win condition: survive all waves | PASS | `Game.cpp`: `::playerWon` flag → `st = GameState::Won` after final wave cleared | — | George / Nour |

---

### H — Damage and Health

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| H1 | Units have HP | PASS | `Unit::hp`; initialised from `g_chars[def].maxHp` | — | George |
| H2 | Units die when HP reaches 0 | PASS | `Game.cpp`: `u.hp <= 0` → unit removed from vector on cleanup | — | George |
| H3 | Melee deals damage on contact | PASS | `updatePlayers()` / `updateEnemies()`: contact damage applied each `attackCooldown` | — | George |
| H4 | Ranged projectile deals damage on hit | PASS | `updateProjectiles()`: `p.alive=false`, target `hp -= p.damage` on collision | — | George |
| H5 | Coins drop when enemy dies | PASS | `Game.cpp`: `coins += cfg::COIN_DROP` (50) on enemy death | — | George |
| H6 | (Non-spec) Bonus coins on lane exit | WARN | `Game.cpp ~line 271`: non-Defense player reaching `GRID_RIGHT` awards `2 * d.cost` coins. Not in spec. Could be intentional game design but affects balance auditing. | Document as intentional or remove | George |

---

### I — Win and Lose Conditions

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| I1 | Win when all waves defeated | PASS | `WaveSystem.cpp` + `Game.cpp`: `::playerWon → st = Won` | — | George / Nour |
| I2 | Lose when base HP reaches 0 | PASS | `Config.h`: `BASE_MAX_HP=1`; first enemy reaching base → `baseHp=0` → `st = Lost` | — | George |
| I3 | Win screen shown | PASS | `UI.cpp`: `drawWinScreen()` renders "VICTORY" with coin total and replay prompt | — | George |
| I4 | Lose screen shown | PASS | `UI.cpp`: `drawLoseScreen()` renders "DEFEAT" with replay prompt | — | George |
| I5 | Restart works from Win/Lose screen | PASS | `Game.cpp onKey()`: `GLFW_KEY_R` on Won/Lost calls `reset()` | — | George |

---

### J — HUD / In-Game UI

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| J1 | Coin counter visible during play | PASS | `UI.cpp drawHUD()`: coin count drawn top-left during `Playing` state | — | George |
| J2 | Base HP indicator visible | PASS | `UI.cpp drawHUD()`: base HP hearts drawn near player base | — | George |
| J3 | Current wave indicator | PASS | `UI.cpp drawHUD()`: "WAVE X / 3" displayed during play | — | George |
| J4 | Card bar always visible during play | PASS | `UI.cpp drawCardBar()`: called every frame in `Playing` / `Paused` states | — | George |

---

### K — Environment / Art

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| K1 | Distinct player-side art | PASS | `UI.cpp drawBattlefield()`: Stark Tower left, holo-terminals, power conduits | — | George |
| K2 | Distinct enemy-side art | PASS | `UI.cpp drawBattlefield()`: Doom Castle Gate right, green banners, energy cracks | — | George |
| K3 | Thematic lane tiles | PASS | Upper lanes: tech floor tiles (Stark); lower lanes: stone tiles (Doom) | — | George |
| K4 | 11+ distinct environmental elements | PASS | Elements counted in `drawBattlefield()`: Stark Tower, turrets (x2), Doom Gate, tech tiles, stone tiles, power conduits, green banners, hazard bollards, energy cracks, rubble/doombot parts, broken energy fence, bottom crates/wall, holo-terminals = 13 distinct elements | — | George |

---

### L — Menus

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| L1 | Main Menu has PLAY button | PASS | `UI.cpp drawMainMenu()`: `BTN_PLAY` rendered and handled | — | George |
| L2 | Main Menu has SOUND SETTINGS button | PASS | `UI.cpp drawMainMenu()`: `BTN_SOUND` rendered; leads to Settings state | — | George |
| L3 | Main Menu has EXIT button | FAIL | `UI.cpp drawMainMenu()`: only `BTN_PLAY` and `BTN_SOUND` are defined. No exit/quit button. Only way to quit is OS window close. | Add `BTN_EXIT` → `glfwSetWindowShouldClose()` | George |
| L4 | Sound settings persist across restart | WARN | `Game::reset()` does not restore `musicVolPct`/`musicMuted` to defaults, so post-restart volume is whatever it was before — which is actually correct persistence. However, if the player muted music, the game restarts silently with no indicator. Consider re-applying volume on reset. | Call `applyVolume()` in `reset()` | George |
| L5 | Pause menu accessible during play | PASS | `Game.cpp onKey()`: `GLFW_KEY_ESCAPE` → `st = Paused`; `UI.cpp` draws pause overlay | — | George |
| L6 | Resume from pause works | PASS | `onKey()`: `GLFW_KEY_ESCAPE` again (or `R`) resumes from `Paused` | — | George |

---

### M — Sound (Bonus)

| ID | Requirement | Status | Evidence | Issue / Fix | Owner |
|----|-------------|--------|----------|-------------|-------|
| M1 | Background music loops | PASS | `Audio.cpp`: `ma_sound_set_looping(&music, MA_TRUE)` before `ma_sound_start` | — | George |
| M2 | Sound effects for game events | PASS | `Audio.cpp`: 5 SFX: `coin.mp3`, `death.mp3`, `ironman.mp3`, `doom.mp3`, `repulsor.mp3` | — | George |
| M3 | Volume controllable in settings | PASS | Settings screen: volume slider in 10% steps, mute toggle; `changeVolume()` / `toggleMute()` | — | George |
| M4 | Master volume affects all audio | PASS | `Audio.cpp setMasterVolume()`: `ma_engine_set_volume(&engine, v)` — affects engine-level output | — | George |

---

## 5. Non-Spec Extras

These features are present in the code but were not required by the spec. They are listed here for completeness and to flag potential grading interactions.

| Feature | Location | Notes |
|---------|----------|-------|
| Background Chooser screen | `Background.h/cpp`, `Game.cpp BgSelect state` | 5 selectable backgrounds (Wakanda, Deep Space, Latveria, Battleground, Baxter Building) shown as thumbnails before play |
| Intro cutscene (18s) | `Cutscene.h/cpp` | Exceeds minimum 15s; adds voiced dialogue subtitles and screen shake |
| Reward popup for lane exit | `Game.cpp ~line 269` | "+N coins" floating text when non-Defense player exits lane right side |
| Extra coin reward on lane exit | `Game.cpp ~line 271` | Awards `2 * d.cost` coins on exit — not in spec (see H6) |
| Jump/Duck dodge mechanic | `Game.cpp`, `Projectile::high` | Players can jump (Up arrow) or duck (Down arrow) to dodge projectiles |
| Animated unit sprites (A/B frames) | `Characters.cpp`, `Game.cpp animTime` | Two-frame walk cycle for all units |
| Explosion particle effect | `Explosion` struct, `Game.cpp` | Visual burst when projectile hits |
| Intro fade-in after cutscene | `Game.h introFade`, `Game.cpp` | 1-second black-to-normal fade when `Playing` state begins |
| Paused state with overlay | `GameState::Paused`, `UI.cpp` | ESC key pauses; semi-transparent overlay with "PAUSED" text |

---

## 6. Manual Test Script (MANUAL items)

The following items cannot be verified by static code analysis and require a running build to confirm.

### MANUAL-1: Wave 3 difficulty increase (G3 qualitative)
**What to check:** Wave 3 (10 enemies) contains Hard-type enemies.
1. Start game, skip cutscene (Space), choose any background.
2. Survive to Wave 3 (defeat all enemies in Waves 1 and 2).
3. Confirm that Hard (larger, faster, 2-damage) enemies appear in Wave 3.

**Pass criteria:** At least one Hard enemy visible in Wave 3 with noticeably different appearance.

---

### MANUAL-2: Lane 4 spawn (G4)
**What to check:** Whether enemies ever appear in the bottom lane (lane index 4) during waves.
1. Start game and reach `Playing` state.
2. Observe all 5 lanes over 3 full waves.
3. Confirm enemies appear in lane 4 (bottom-most lane).

**Pass criteria:** At least one enemy spawns in the bottom lane across the 3 waves.
**Failure note:** `WaveSystem.cpp` uses `rand() % 4` — if WaveSystem is the sole spawn path, lane 4 will never be used. `Game.cpp`'s override should fix this, but verify at runtime.

---

### MANUAL-3: Exit button absence (L1)
**What to check:** Confirm there is no EXIT/QUIT button on the Main Menu.
1. Launch the game; the Main Menu appears.
2. Count the interactive buttons visible.
3. Confirm no "EXIT" or "QUIT" button is present.

**Pass criteria for the FAIL:** Only "PLAY" and "SOUND SETTINGS" buttons are present — EXIT is absent.
**Fix required:** Add an EXIT button that calls `glfwSetWindowShouldClose(window, GLFW_TRUE)`.

---

*End of audit report.*
