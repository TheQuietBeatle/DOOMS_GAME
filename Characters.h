#pragma once
#include "Sprites.h"

enum class Role { Melee, Ranged, Defense, EnemyUpper, EnemyLower, EnemyMelee };

// The first NUM_HERO_TYPES entries are the player cards, in card order.
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

float unitScale(int def);      // sprite pixel -> screen px
float unitHeight(int def);     // on-screen height
float unitHalfWidth(int def);  // half the collision width