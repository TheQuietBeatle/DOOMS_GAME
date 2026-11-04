#include "Characters.h"

// ---------------------------------------------------------------------------
// Tuning table. Change numbers here; no logic needs to change.
// Costs: ranged = 2x melee, defense = 3x melee (per the spec).
// Unlock thresholds: base = 80 coins; Ranged = 2x80 = 160, Defense = 3x80 = 240.
// Melee unlockCost = 0 so the player can always buy all 3 Melee with starting 150 coins
// (an 80-coin threshold would re-lock Melee after the 2nd purchase at 50 coins held).
// ---------------------------------------------------------------------------
const CharacterDef g_chars[CH_COUNT] = {
    // name       role             cost  unlockCost  maxHp  speed   dmg   cd     frames                         height
    { "MELEE",   Role::Melee,       50,   0,          5,    40.0f,  1,  0.60f, SPR_MELEE_A,  SPR_MELEE_B,  2.00f },
    { "RANGED",  Role::Ranged,      100,  160,         5,   20.0f,  1,  1.40f, SPR_RANGED_A, SPR_RANGED_B, 3.0f  },
    { "DEFENSE", Role::Defense,     150,  240,         7,    0.0f,  0,  0.00f, SPR_DEFENSE,  SPR_DEFENSE,  1.20f },
    { "EASY",    Role::EnemyUpper,  0,    0,           5,   22.0f,  1,  3.50f, SPR_EASY_A,   SPR_EASY_B,   1.4f  },
    { "MEDIUM",  Role::EnemyLower,  0,    0,           5,   22.0f,  1,  3.50f, SPR_MEDIUM_A, SPR_MEDIUM_B, 1.20f },
    { "HARD",    Role::EnemyMelee,  0,    0,           7,   55.0f,  2,  1.10f, SPR_HARD_A,   SPR_HARD_B,   1.40f },
};

float unitScale(int def)
{
    const SpriteFrame& f = g_sprites[g_chars[def].frameA];
    return f.h > 0.0f ? g_chars[def].drawHeight / f.h : 1.0f;
}

float unitHeight(int def)
{
    return g_chars[def].drawHeight;
}

float unitHalfWidth(int def)
{
    // 60% of the sprite width counts as the body (ignores weapons/empty margin)
    // [George] Renderer::drawSprite draws a sprite f.w * (f.w * scale) px wide (it scales
    // pixel-unit vertices a second time), so the REAL on-screen width is f.w * f.w * scale.
    // Using only f.w * scale made the hitbox ~1 px wide, so players and enemies walked
    // into each other and fought back-to-back.
    const SpriteFrame& f = g_sprites[g_chars[def].frameA];
    return f.w * f.w * unitScale(def) * 0.30f;
}