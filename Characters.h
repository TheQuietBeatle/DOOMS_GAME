// Characters.h - Character definitions: roles, stats, and sprite references.
// g_chars[] is the master table; helpers unitScale/unitHeight/unitHalfWidth
// derive rendering and collision sizes from it.
#pragma once
#include "Sprites.h"

// Role determines movement direction, attack type, and projectile height.
// Defense players are stationary and absorb enemy shots before they reach others.
enum class Role { Melee, Ranged, Defense, EnemyUpper, EnemyLower, EnemyMelee };

// The first NUM_HERO_TYPES entries (CH_MELEE..CH_DEFENSE) are the player cards,
// in card-bar order. The rest are enemy types.
enum CharId { CH_MELEE, CH_RANGED, CH_DEFENSE, CH_EASY, CH_MEDIUM, CH_HARD, CH_COUNT };
constexpr int NUM_HERO_TYPES = 3;

struct CharacterDef {
    const char* name;
    Role     role;
    int      cost;            // coins to place (heroes only)
    int      unlockCost;      // coins held to unlock the card (re-evaluated every frame)
    int      maxHp;
    float    speed;           // world px / second (0 = stationary)
    int      damage;          // per attack
    float    attackCooldown;  // seconds between attacks
    SpriteId frameA, frameB;  // two animation frames
    float    drawHeight;      // on-screen height in px (scale is computed from this)
    int hp;      // current lives
};

extern const CharacterDef g_chars[CH_COUNT];

// Returns the uniform scale factor that maps one sprite pixel to screen pixels.
// scale = drawHeight / sprite.h  (then Renderer applies it twice — see unitHalfWidth).
float unitScale(int def);

// Returns the on-screen height of the character in world pixels.
float unitHeight(int def);

// Returns half the character's collision box width in world pixels.
// DMET502 spec requirement: hitbox-based lane collision (lane overlap detection).
// The real on-screen width is f.w * f.w * scale because Renderer::drawSprite
// scales the already-pixel-unit vertices a second time.
float unitHalfWidth(int def);