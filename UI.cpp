#include "UI.h"
#include "Characters.h"
#include "Config.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace cfg;

// ---------------------------------------------------------------------------
// Card bar
// ---------------------------------------------------------------------------
void CardBar::cardRect(int i, float& x, float& y) const
{
    x = x0 + i * (w + gap);
    y = y0;
}

int CardBar::hitTest(float wx, float wy) const
{
    for (int i = 0; i < NUM_HERO_TYPES; ++i) {
        float x, y;
        cardRect(i, x, y);
        if (wx >= x && wx <= x + w && wy >= y && wy <= y + h) return i;
    }
    return -1;
}

// [George] Renderer::drawSprite scales the model by (f.w * scaleX, f.h * scaleY),
// but sprite vertices are already 0..f.w / 0..f.h, so the real on-screen size is
// f.w * f.w * scaleX by f.h * f.h * scaleY. In-game units are tuned around that
// (tiny drawHeight values), so instead of changing the renderer, HUD code goes
// through this helper: px = on-screen size of ONE sprite pixel.
static void drawSpritePx(Renderer& r, const SpriteFrame& f,
    float cx, float bottomY, float px, const DrawFx& fx = DrawFx())
{
    r.drawSprite(f, cx, bottomY, px / f.w, px / f.h, false, fx);
}

// Fits sprite f inside box (bx,by,bw,bh), centred, aspect-ratio preserved.
static void drawSpriteFit(Renderer& r, const SpriteFrame& f,
    float bx, float by, float bw, float bh, const DrawFx& fx = DrawFx())
{
    float s = std::min(bw / f.w, bh / f.h);   // on-screen px per sprite pixel
    float cx = bx + bw * 0.5f;                 // horizontal centre of box
    float cy = by + (bh - f.h * s) * 0.5f;     // feet of vertically-centred sprite
    drawSpritePx(r, f, cx, cy, s, fx);
}

// [George] per-card portrait tweaks, in card order: Melee, Ranged, Defense
// zoom: 1.0 = fit the box, 1.2 = 20% bigger, 0.8 = 20% smaller
// dx/dy: nudge in pixels (+x = right, +y = up)
static const float kPortraitZoom[NUM_HERO_TYPES] = { 1.10f, 1.10f, 1.10f };
static const float kPortraitDX[NUM_HERO_TYPES] = { -17.0f,  -17.0f,  -17.0f };
static const float kPortraitDY[NUM_HERO_TYPES] = { 0.0f,  0.0f,  0.0f };

void CardBar::draw(Renderer& r, int coins, int armedCard, int hoveredCard) const
{
    for (int i = 0; i < NUM_HERO_TYPES; ++i) {
        float x, y;
        cardRect(i, x, y);
        const CharacterDef& d = g_chars[i];
        bool unlocked = coins >= d.cost;

        // border + background
        glm::vec3 border = { 0.30f, 0.34f, 0.40f };
        if (unlocked && hoveredCard == i) border = { 0.90f, 0.95f, 1.00f };
        if (unlocked && armedCard == i)   border = { 1.00f, 0.80f, 0.20f };
        r.drawRect(x - 3, y - 3, w + 6, h + 6, border);
        r.drawRect(x, y, w, h, unlocked ? glm::vec3(0.10f, 0.22f, 0.36f) : glm::vec3(0.16f, 0.16f, 0.16f));

        // portrait: top 70% of card, 6 px padding on all sides
        DrawFx fx; fx.gray = unlocked ? 0.0f : 1.0f;
        const SpriteFrame& pf = g_sprites[d.frameA];
        float boxX = x + 4.0f, boxY = y + h * 0.30f + 4.0f;
        float boxW = w - 8.0f, boxH = h * 0.60f - 8.0f;
        float ps = std::min(boxW / pf.w, boxH / pf.h) * kPortraitZoom[i];   // px per sprite pixel
        float pcx = boxX + boxW * 0.5f + kPortraitDX[i];
        float pby = boxY + (boxH - pf.h * ps) * 0.5f + kPortraitDY[i];
        drawSpritePx(r, pf, pcx, pby, ps, fx);

        // cost row: bottom 30% — 16 px coin + number, centred together
        const SpriteFrame& cf = g_sprites[SPR_COIN];
        const float COIN_PX = 16.0f;
        std::string cost = std::to_string(d.cost);
        float tw = r.textWidth(cost, 3.0f);
        float totalW = COIN_PX + 3.0f + tw;
        float rowX = x + (w - totalW) * 0.5f;
        float rowCY = y + h * 0.15f;
        DrawFx cfx; cfx.gray = unlocked ? 0.0f : 1.0f;
        drawSpritePx(r, cf, rowX + COIN_PX * 0.5f, rowCY - COIN_PX * 0.5f, COIN_PX / cf.w, cfx);
        r.drawText(cost, rowX + COIN_PX + 3.0f, rowCY - 10.5f, 3.0f,
            unlocked ? glm::vec3(1.0f, 0.88f, 0.45f) : glm::vec3(0.55f));
    }
}

// ---------------------------------------------------------------------------
// Battlefield: background, lanes, home base, enemy gate
// ---------------------------------------------------------------------------
void drawBattlefield(Renderer& r, int hoveredLane, const bool* selectedLanes, float time)
{
    const float gridBottom = laneBottom(NUM_LANES - 1);
    const float gridW = GRID_RIGHT - GRID_LEFT;
    const float cellW = gridW / GRID_COLS;
    const float pulse = 0.65f + 0.35f * std::sin(time * 3.0f);

    // background + top bar
    r.drawRect(0, 0, WORLD_W, WORLD_H, { 0.05f, 0.07f, 0.11f });
    r.drawRect(0, 606, WORLD_W, WORLD_H - 606, { 0.07f, 0.11f, 0.19f });
    r.drawRect(0, 604, WORLD_W, 3, { 0.20f, 0.55f, 0.75f });

    // grid frame
    r.drawRect(GRID_LEFT - 4, gridBottom - 4, gridW + 8, GRID_TOP - gridBottom + 8, { 0.02f, 0.03f, 0.05f });

    // tiles: tech floor on the hero side, mossy stone on the enemy side
    for (int lane = 0; lane < NUM_LANES; ++lane) {
        float y = laneBottom(lane);
        for (int c = 0; c < GRID_COLS; ++c) {
            float x = GRID_LEFT + c * cellW;
            bool tech = c < GRID_COLS / 2;
            bool alt = (lane + c) % 2 == 1;
            glm::vec3 col = tech
                ? (alt ? glm::vec3(0.10f, 0.30f, 0.45f) : glm::vec3(0.12f, 0.35f, 0.52f))
                : (alt ? glm::vec3(0.18f, 0.24f, 0.15f) : glm::vec3(0.21f, 0.28f, 0.17f));
            r.drawRect(x + 1, y + 1, cellW - 2, LANE_H - 2, col);
            // small detail per tile: rivets on tech tiles, cracks on stone
            glm::vec3 detail = col * 0.7f;
            if (tech) {
                r.drawRect(x + 6, y + 6, 4, 4, detail);
                r.drawRect(x + cellW - 10, y + LANE_H - 10, 4, 4, detail);
            }
            else {
                r.drawRect(x + 20, y + 30, 18, 3, detail);
                r.drawRect(x + 36, y + 26, 3, 8, detail);
            }
        }
    }

    // lane highlights
    for (int lane = 0; lane < NUM_LANES; ++lane) {
        float y = laneBottom(lane);
        if (selectedLanes[lane]) {
            r.drawRect(GRID_LEFT, y, gridW, LANE_H, { 0.30f, 0.90f, 1.00f }, 0.22f);
            r.drawRect(GRID_LEFT, y, 6, LANE_H, { 0.30f, 0.90f, 1.00f });
        }
        if (lane == hoveredLane)
            r.drawRect(GRID_LEFT, y, gridW, LANE_H, { 1.0f, 1.0f, 1.0f }, 0.12f);
    }

    // =========================================================================
    // HOME BASE  –  Stark Tech Tower / Iron Man Defense Citadel
    // =========================================================================
    {
        // ── coordinate helpers (all derived from grid constants) ──────────────
        const float BW = GRID_LEFT - 6.0f;               // column width  ≈134
        const float BY = gridBottom - 4.0f;              // column bottom ≈ 46
        const float BH = GRID_TOP - gridBottom + 8.0f;   // column height ≈558
        const float BTOP = BY + BH;                         //               ≈604
        const float BCX = BW * 0.5f;                      // h-centre      ≈ 67

        // Tower body occupies the left portion; turrets mount on the right
        const float TX = 2.0f;                            // tower left edge
        const float TW = 66.0f;                           // tower body width
        const float TBY = BY + 22.0f;                     // tower body bottom y
        const float TBH = BH - 44.0f;                     // tower body height ≈514
        const float TBT = TBY + TBH;                      // tower body top y  ≈582
        const float TCX = TX + TW * 0.5f;                 // tower body centre x ≈35

        // ── extra pulses ──────────────────────────────────────────────────────
        const float blink = (std::fmod(time, 1.4f) < 0.25f) ? 1.0f : 0.12f;
        const float slowPulse = 0.50f + 0.50f * std::sin(time * 1.6f);
        const float reactP = 0.55f + 0.45f * std::sin(time * 4.2f);

        // ── 1. BACKGROUND STEEL WALL ─────────────────────────────────────────
        r.drawRect(0, BY, BW, BH, { 0.07f, 0.09f, 0.13f });

        // ── 2. FOUNDATION PLATFORM ───────────────────────────────────────────
        r.drawRect(0, BY, BW, 22.0f, { 0.20f, 0.24f, 0.31f });   // slab
        r.drawRect(0, BY, BW, 3.0f, { 0.32f, 0.38f, 0.48f });   // bottom highlight
        r.drawRect(0, BY + 19, BW, 3.0f, { 0.28f, 0.34f, 0.44f });   // top ledge
        // Rivets along the foundation
        for (int i = 0; i < 5; ++i) {
            float rx = 10.0f + i * (BW - 20.0f) / 4.0f;
            r.drawRect(rx - 2, BY + 8, 4, 5, { 0.32f, 0.38f, 0.48f });
        }

        // ── 3. MAIN TOWER BODY (left column x = TX..TX+TW) ──────────────────
        // Base coat – dark steel
        r.drawRect(TX, TBY, TW, TBH, { 0.13f, 0.16f, 0.22f });
        // Metallic gradient simulation: three overlapping brightness bands
        r.drawRect(TX + TW * 0.20f, TBY, TW * 0.60f, TBH, { 0.17f, 0.21f, 0.28f });
        r.drawRect(TX + TW * 0.37f, TBY, TW * 0.26f, TBH, { 0.22f, 0.27f, 0.36f });
        // Dark edge shadows (left & right)
        r.drawRect(TX, TBY, 5.0f, TBH, { 0.07f, 0.08f, 0.12f });
        r.drawRect(TX + TW - 5.0f, TBY, 5.0f, TBH, { 0.07f, 0.08f, 0.12f });
        // Decorative trim plates flanking the center
        r.drawRect(TX + 8, TBY, 4, TBH, { 0.18f, 0.22f, 0.30f });
        r.drawRect(TX + TW - 12, TBY, 4, TBH, { 0.18f, 0.22f, 0.30f });
        // Horizontal panel seams – divide body into 6 zones
        for (int s = 1; s <= 5; ++s) {
            float sy = TBY + s * TBH / 6.0f;
            r.drawRect(TX + 4, sy, TW - 8, 2, { 0.07f, 0.09f, 0.13f }); // recess
            r.drawRect(TX + 4, sy + 2, TW - 8, 1, { 0.26f, 0.32f, 0.44f }); // highlight
        }
        // Rim / outline lighting (thin cyan edge glow on both sides)
        r.drawRect(TX - 1, TBY, 2, TBH, { 0.15f, 0.55f, 0.82f }, 0.55f);
        r.drawRect(TX + TW - 1, TBY, 2, TBH, { 0.15f, 0.55f, 0.82f }, 0.55f);

        // ── 4. VERTICAL CYAN LIGHT BARS ─────────────────────────────────────
        const float LBY = TBY + TBH * 0.08f;
        const float LBH = TBH * 0.84f;
        // Left bar
        r.drawRect(TX + 15, LBY, 5, LBH, { 0.08f, 0.42f * slowPulse, 0.68f * slowPulse }, 0.90f);
        r.drawRect(TX + 17, LBY, 1, LBH, { 0.35f, 0.82f * slowPulse, 1.00f * slowPulse }, 0.70f);
        // Right bar
        r.drawRect(TX + TW - 20, LBY, 5, LBH, { 0.08f, 0.42f * slowPulse, 0.68f * slowPulse }, 0.90f);
        r.drawRect(TX + TW - 18, LBY, 1, LBH, { 0.35f, 0.82f * slowPulse, 1.00f * slowPulse }, 0.70f);

        // ── 5. WINDOW PANELS (4 rows × 2 panes, glow with slowPulse) ────────
        for (int row = 0; row < 4; ++row) {
            float wy = TBY + TBH * (0.06f + row * 0.22f);
            for (int col = 0; col < 2; ++col) {
                float wx = TCX + (col == 0 ? -14.0f : 6.0f);
                r.drawRect(wx, wy, 8, 12, { 0.10f, 0.15f, 0.22f });           // frame
                r.drawRect(wx + 1, wy + 1, 6, 10,
                    { 0.16f * slowPulse, 0.52f * slowPulse, 0.78f * slowPulse });            // glass glow
                r.drawRect(wx + 2, wy + 2, 4, 6,
                    { 0.25f * slowPulse, 0.72f * slowPulse, 0.95f * slowPulse });            // bright centre
            }
        }

        // ── 6. ARC-REACTOR CORE ──────────────────────────────────────────────
        const float RCX = TCX;                          // ≈35 (tower centre)
        const float RCY = BY + BH * 0.48f;             // ≈314 (near vertical mid)
        const float R = 16.0f;

        // Outer halo (expanding soft glow)
        float halo = R * 1.85f * reactP;
        r.drawRect(RCX - halo, RCY - halo, halo * 2, halo * 2,
            { 0.03f, 0.22f, 0.52f }, 0.22f * reactP);
        // Concentric rings (4 nested squares approximating circles)
        r.drawRect(RCX - R * 1.45f, RCY - R * 1.45f, R * 2.9f, R * 2.9f, { 0.08f, 0.28f, 0.58f }, 0.55f);
        r.drawRect(RCX - R * 1.05f, RCY - R * 1.05f, R * 2.1f, R * 2.1f, { 0.12f, 0.48f, 0.82f }, 0.80f);
        r.drawRect(RCX - R * 0.68f, RCY - R * 0.68f, R * 1.36f, R * 1.36f,
            { 0.18f, 0.68f * reactP, 1.0f * reactP }, 0.95f);
        r.drawRect(RCX - R * 0.36f, RCY - R * 0.36f, R * 0.72f, R * 0.72f,
            { 0.52f, 0.88f * reactP, 1.0f }, 1.0f);
        // Bright core pixel
        r.drawRect(RCX - 3, RCY - 3, 6, 6, { 0.88f, 0.96f, 1.0f }, reactP);

        // Reactor bezel (square frame around the rings)
        const float BZR = R * 1.58f;
        r.drawRect(RCX - BZR, RCY - BZR, BZR * 2, 3, { 0.22f, 0.30f, 0.42f }); // top
        r.drawRect(RCX - BZR, RCY + BZR - 3, BZR * 2, 3, { 0.22f, 0.30f, 0.42f }); // bottom
        r.drawRect(RCX - BZR, RCY - BZR, 3, BZR * 2, { 0.22f, 0.30f, 0.42f }); // left
        r.drawRect(RCX + BZR - 3, RCY - BZR, 3, BZR * 2, { 0.22f, 0.30f, 0.42f }); // right

        // ── 7. PER-LANE TURRETS (one per lane, facing right) ─────────────────
        for (int lane = 0; lane < NUM_LANES; ++lane) {
            const float LCY = laneBottom(lane) + LANE_H * 0.5f;  // lane v-centre

            // Mounting bracket (bolted to tower at x = TX+TW ≈ 68)
            r.drawRect(TX + TW, LCY - 15, 14, 30, { 0.17f, 0.21f, 0.28f });
            r.drawRect(TX + TW, LCY - 15, 3, 30, { 0.28f, 0.34f, 0.46f }); // left highlight
            r.drawRect(TX + TW + 2, LCY + 12, 10, 3, { 0.14f, 0.17f, 0.23f }); // shadow tab

            // Turret housing (main box)
            r.drawRect(TX + TW + 14, LCY - 12, 28, 24, { 0.22f, 0.27f, 0.35f });
            r.drawRect(TX + TW + 14, LCY - 12, 28, 3, { 0.33f, 0.40f, 0.52f }); // top bevel
            r.drawRect(TX + TW + 14, LCY + 9, 28, 3, { 0.12f, 0.14f, 0.20f }); // bottom shadow
            r.drawRect(TX + TW + 18, LCY - 4, 18, 1, { 0.15f, 0.18f, 0.25f }); // panel line
            // Alignment fins
            r.drawRect(TX + TW + 16, LCY - 18, 16, 5, { 0.18f, 0.22f, 0.30f }); // top fin
            r.drawRect(TX + TW + 16, LCY + 13, 16, 5, { 0.18f, 0.22f, 0.30f }); // bottom fin

            // Barrel (slim rod extending to right edge of base)
            const float barX = TX + TW + 42.0f;
            const float barW = BW - barX - 8.0f;             // ≈18 px
            r.drawRect(barX, LCY - 5, barW, 10, { 0.26f, 0.32f, 0.42f });
            r.drawRect(barX, LCY - 5, barW, 2, { 0.35f, 0.43f, 0.56f }); // top highlight
            r.drawRect(barX, LCY + 3, barW, 2, { 0.13f, 0.16f, 0.22f }); // bottom shadow

            // Energy emitter at barrel tip (glowing cyan, pulsing)
            const float emX = BW - 10.0f;
            r.drawRect(emX - 4, LCY - 4, 8, 8, { 0.04f, 0.05f, 0.07f });               // socket
            r.drawRect(emX - 3, LCY - 3, 6, 6, { 0.04f * pulse, 0.68f * pulse, 1.0f * pulse }); // glow
            r.drawRect(emX - 2, LCY - 2, 4, 4, { 0.28f * pulse, 0.88f * pulse, 1.0f * pulse }); // core
        }

        // ── 8. TOWER CROWN / TOP CAP ─────────────────────────────────────────
        r.drawRect(TX + 6, TBT, TW - 12, 13, { 0.20f, 0.25f, 0.34f });
        r.drawRect(TX + 12, TBT + 13, TW - 24, 8, { 0.17f, 0.22f, 0.30f });
        r.drawRect(TX + 6, TBT, TW - 12, 3, { 0.32f, 0.40f, 0.52f }); // crown highlight

        // ── 9. ANTENNA + BLINKING BEACON ─────────────────────────────────────
        const float antY = BTOP - 14.0f;           // antenna base y ≈590
        // Mast
        r.drawRect(BCX - 2, antY, 4, 34, { 0.20f, 0.26f, 0.35f });
        r.drawRect(BCX - 1, antY, 2, 34, { 0.34f, 0.42f, 0.55f }); // centre highlight
        // Cross-arm
        r.drawRect(BCX - 11, antY + 20, 22, 3, { 0.17f, 0.21f, 0.29f });
        // Beacon light + soft halo
        r.drawRect(BCX - 7, antY + 24, 14, 14,
            { 1.0f, 0.10f, 0.04f }, 0.22f * blink);               // halo
        r.drawRect(BCX - 4, antY + 27, 8, 8,
            { 1.0f * blink, 0.20f * blink, 0.04f * blink });       // beacon core
    }

    // enemy gate on the right edge
    r.drawRect(GRID_RIGHT + 4, gridBottom - 4, WORLD_W - GRID_RIGHT - 4, GRID_TOP - gridBottom + 8,
        { 0.12f, 0.16f, 0.10f });

    // floor strip under the grid
    r.drawRect(0, 0, WORLD_W, gridBottom - 8, { 0.04f, 0.05f, 0.08f });
}

// ---------------------------------------------------------------------------
// HUD pieces
// ---------------------------------------------------------------------------
void drawCoinPanel(Renderer& r, int coins)
{
    // inner box: x=20..145, y=615..710  (125×95 px)
    r.drawRect(17, 612, 131, 101, { 0.30f, 0.34f, 0.40f });
    r.drawRect(20, 615, 125, 95, { 0.06f, 0.14f, 0.24f });

    // coin icon: 30 px, left side of box, vertically centred
    const SpriteFrame& cf = g_sprites[SPR_COIN];
    const float COIN_PX = 30.0f;
    float coinCX = 20.0f + 6.0f + COIN_PX * 0.5f;        // = 41 (6 px left pad)
    float coinCY = 615.0f + (95.0f - COIN_PX) * 0.5f;    // = 647.5 (vertically centred)
    drawSpritePx(r, cf, coinCX, coinCY, COIN_PX / cf.w);

    // number: centred in right zone (x=62..145)
    std::string s = std::to_string(coins);
    float tw = r.textWidth(s, 4.0f);
    float textCX = (coinCX + COIN_PX * 0.5f + 6.0f + 145.0f) * 0.5f; // = 103.5
    float textY = 615.0f + (95.0f - 28.0f) * 0.5f;                  // = 648.5
    r.drawText(s, textCX - tw * 0.5f, textY, 4.0f, { 1.0f, 0.88f, 0.45f });
}
void drawHealthBar(Renderer& r, float cx, float y, float width, int hp, int maxHp, bool isEnemy)
{
    float ratio = std::clamp((float)hp / (float)maxHp, 0.0f, 1.0f);
    float x = cx - width / 2.0f;
    r.drawRect(x - 2, y - 2, width + 4, 10, { 0.0f, 0.0f, 0.0f }, 0.85f);
    r.drawRect(x, y, width * ratio, 6,
        isEnemy ? glm::vec3(0.95f, 0.30f, 0.20f) : glm::vec3(0.30f, 0.95f, 0.45f));
    // one notch per hit point so remaining health is easy to count
    for (int i = 1; i < maxHp; ++i)
        r.drawRect(x + width * i / maxHp - 1, y, 2, 6, { 0.0f, 0.0f, 0.0f }, 0.6f);
}

void drawCenteredText(Renderer& r, const std::string& s, float y, float px, glm::vec3 color)
{
    r.drawText(s, (WORLD_W - r.textWidth(s, px)) / 2.0f, y, px, color);
}