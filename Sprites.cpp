// Sprites.cpp - Registers every sprite frame into the Renderer's vertex list.
// Called once at startup (before Renderer::upload()) by main.cpp.
// Pixel-art data lives in HeroSprites.h and EnemySprites.h.
#include "Sprites.h"
#include "HeroSprites.h"
#include "EnemySprites.h"

SpriteFrame g_sprites[SPR_COUNT];

// sizeof(arr)/sizeof(arr[0]) = number of rows in the array
#define ADD_SPRITE(id, arr, pal) \
    g_sprites[id] = r.addSprite(arr, (int)(sizeof(arr) / sizeof(arr[0])), pal)

void buildSprites(Renderer& r)
{
    // heroes - each one with ITS OWN palette
    ADD_SPRITE(SPR_MELEE_A, kMeleeA, kMeleePalette);
    ADD_SPRITE(SPR_MELEE_B, kMeleeB, kMeleePalette);
    ADD_SPRITE(SPR_RANGED_A, kLauncher, kLauncherPalette);
    ADD_SPRITE(SPR_RANGED_B, kLauncher, kLauncherPalette);
    ADD_SPRITE(SPR_DEFENSE, kDefense, kHeroPalette);
    ADD_SPRITE(SPR_COIN, kCoin, kHeroPalette);

    // enemies - all share kEnemyPalette
    ADD_SPRITE(SPR_EASY_A, kEasyA, kEnemyPalette);
    ADD_SPRITE(SPR_EASY_B, kEasyB, kEnemyPalette);
    ADD_SPRITE(SPR_MEDIUM_A, kMediumA, kEnemyPalette);
    ADD_SPRITE(SPR_MEDIUM_B, kMediumB, kEnemyPalette);
    ADD_SPRITE(SPR_HARD_A, kHardA, kEnemyPalette);
    ADD_SPRITE(SPR_HARD_B, kHardB, kEnemyPalette);
}
#undef ADD_SPRITE