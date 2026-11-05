# LinkedIn Notes — DoomsGame

## The project in one sentence

- DoomsGame is a five-lane, three-wave tower-defense game where Iron Man defends Stark Tower from Doctor Doom's Doombots — written from scratch in C++17 and OpenGL 3.3 for DMET502 Lab 2.

## Problem and audience

- The lab brief starts from a bare OpenGL window and asks for a complete game, so every system had to be built: rendering, input, menus, gameplay rules, audio, and asset loading.
- Aimed at anyone reviewing graphics coursework, gameplay programming, or small-engine architecture — and at students who want to see how a template becomes a playable build.

## Strongest features and technical decisions

- **One batch for all art:** every sprite and glyph is an ASCII grid converted once into a single vertex buffer, drawn with one shader program. Why it matters: zero asset pipeline, and pixel art can be edited as plain text.
- **Data-driven tuning:** hero and enemy stats live in one table in `Characters.cpp`, so costs, HP, speed, and damage can be rebalanced without touching gameplay code.
- **Swept-segment projectile collision:** rockets test the segment they crossed each frame instead of a point, so fast shots cannot tunnel through enemies (`Game.cpp:443`).
- **Five switchable backgrounds:** painted PNGs are drawn cover-fit behind the board, with a thumbnail chooser and an `F2` cycle during play (`Background.cpp`).
- **Explicit state machine:** menu, settings, background chooser, cutscene, playing, paused, won, and lost are all handled by one `GameState` enum — clear ownership of input and simulation (`Game.h:55`).

## Hardest challenge and the fix

- **Bug:** melee units fought back-to-back with almost no contact, as if their hitboxes were a few pixels wide.
- **Cause:** `Renderer::drawSprite` applies the sprite width twice (pixel-unit vertices are scaled by `f.w × scaleX`, so on-screen width is `f.w × f.w × scaleX`).
- **Fix:** `unitHalfWidth` now compensates for the double scaling at `Characters.cpp:34-39`, with a comment explaining the formula so the workaround is not "simplified" away later.
- **Related ordering fix:** `reset()` now runs when the cutscene ends, not when Play is clicked, so the wave timer does not start during the intro (`Game.cpp:237-245`).

## Numbers I can defend

- 5,213 lines of first-party C++ (`.cpp` + `.h`, measured with `wc -l`, excluding the bundled `miniaudio.h` and `stb_image.h`)
- 22 first-party source/header files in the repository root; no `package.json`, no web layer
- 5 lanes; 3 waves of 3 / 5 / 10 enemies (18 total); at most 10 enemies alive; one-hit-point base
- 3 hero classes (Melee 50, Ranged 100, Defense 150) and 3 enemy types
- 5 backgrounds, 6 audio files (theme, 3 effects, 2 voice lines), one 18-second intro cutscene
- 7 commits between 24 October and 4 November 2026; solo project (single author in the Git history)

## Tech stack hashtags

- #Cpp #CPlusPlus #OpenGL #GLFW #GLM #GameDev #ComputerGraphics #GameProgramming #StudentProject #IndieDev

## Best screenshots to post

- `docs/screenshots/04-gameplay.png` — shows the core loop at once: three hero placements, incoming Doombots, health bars, coin HUD, wave counter.
- `docs/screenshots/02-background-select.png` — demonstrates the five-background system and the thumbnail chooser in one frame.
- `docs/screenshots/03-intro-cutscene.png` — shows the scripted cutscene and the custom pixel-art look; good contrast against the gameplay shot.

## Three hook ideas

- "Every character in my C++ tower-defense game is ASCII art compiled into a single vertex buffer — no engine, no spritesheets."
- "My units were fighting back-to-back because one sprite dimension was being scaled twice. The fix was one line, and the comment explaining it is longer than the code."
- "Five lanes, three waves, five backgrounds, one 18-second cutscene — what it took to turn a bare OpenGL template into a finished lab project."

## Capture notes

- All screenshots are 1440×900 captures of the running build.
- No mobile screenshot was included: the playfield is a fixed 16:9 world and letterboxes in portrait, so there is no responsive mobile layout to show.
