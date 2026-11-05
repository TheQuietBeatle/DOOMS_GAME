// Sprites.h - Declares every animation frame id and the global sprite table.
// HeroSprites.h and EnemySprites.h hold the pixel-art data; Sprites.cpp
// builds them into renderer vertices via buildSprites().
#pragma once
#include "Renderer.h"

// Every animation frame in the game. Add new ids here + in buildSprites().
enum SpriteId {
    SPR_MELEE_A, SPR_MELEE_B,
    SPR_RANGED_A, SPR_RANGED_B,
    SPR_DEFENSE,
    SPR_EASY_A, SPR_EASY_B,
    SPR_MEDIUM_A, SPR_MEDIUM_B,
    SPR_HARD_A, SPR_HARD_B,
    SPR_COIN,
    SPR_COUNT
};

extern SpriteFrame g_sprites[SPR_COUNT];

// Builds every frame into the renderer's vertex list. Call before Renderer::upload().
void buildSprites(Renderer& r);