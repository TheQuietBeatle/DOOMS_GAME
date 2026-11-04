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
        bool unlocked = coins >= d.unlockCost;

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
// Environment helpers  (called from drawBattlefield)
// ---------------------------------------------------------------------------

// Elements drawn on top of tiles, BELOW lane-highlights:
//   4 - Rubble + Doombot Parts   5 - Green Energy Cracks
//   6 - Stark Power Conduits     8 - Hazard Bollards   9 - Broken Energy Fence
static void drawEnvironmentFloor(Renderer& r, float time)
{
    const float gridBottom = laneBottom(NUM_LANES - 1);
    const float gridW      = GRID_RIGHT - GRID_LEFT;
    const float cellW      = gridW / GRID_COLS;
    const float midX       = GRID_LEFT + cellW * (GRID_COLS / 2);  // 700

    // 8. HAZARD BOLLARDS on lane-divider lines
    // 6x12 body centred on divider y; at x = GRID_LEFT+cellW*1 and *3
    // 3 alternating yellow/black stripes, cyan blinking tip
    for (int L = 0; L < NUM_LANES; ++L) {
        float dy = laneBottom(L);
        float bxArr[2] = { GRID_LEFT + cellW * 1.0f - 3.0f,
                           GRID_LEFT + cellW * 3.0f - 3.0f };
        for (int bi = 0; bi < 2; ++bi) {
            float bx = bxArr[bi];
            r.drawRect(bx - 2, dy,       10,  3, { 0.18f, 0.18f, 0.18f }); // base flange
            r.drawRect(bx,     dy - 12,   6, 12, { 0.26f, 0.26f, 0.26f }); // body
            r.drawRect(bx,     dy -  3,   6,  3, { 0.75f, 0.58f, 0.04f }); // yellow stripe 1
            r.drawRect(bx,     dy -  6,   6,  2, { 0.10f, 0.10f, 0.10f }); // black stripe
            r.drawRect(bx,     dy -  9,   6,  2, { 0.75f, 0.58f, 0.04f }); // yellow stripe 2
            float tp = 0.5f + 0.5f * std::sin(time * 3.0f + (float)(L * 2 + bi) * 0.9f);
            r.drawRect(bx + 1, dy - 15,   4,  3,
                { 0.10f * tp, 0.65f * tp, 0.85f * tp });                   // cyan tip
        }
    }

    // 6. STARK POWER CONDUITS with travelling pulse
    for (int lane = 0; lane < NUM_LANES; ++lane) {
        float ly       = laneBottom(lane) + 6.0f;
        float conduitW = midX - GRID_LEFT - 10.0f;
        float conduitX = GRID_LEFT + 5.0f;
        r.drawRect(conduitX, ly, conduitW, 5, { 0.14f, 0.18f, 0.26f });
        r.drawRect(conduitX, ly, conduitW, 1, { 0.24f, 0.30f, 0.42f });
        for (int c = 0; c < 5; ++c) {
            float nx = GRID_LEFT + c * cellW + cellW * 0.5f;
            float cp = 0.45f + 0.45f * std::sin(time * 2.0f + (float)c * 0.9f);
            r.drawRect(nx - 5, ly - 2, 10, 9, { 0.10f, 0.22f, 0.36f });
            r.drawRect(nx - 3, ly,      6,  5,
                { 0.08f * cp, 0.44f * cp, 0.72f * cp }, 0.85f);
        }
        // Travelling cyan pulse 12x3
        float pulseX = conduitX +
            std::fmod(time * 120.0f + (float)lane * 90.0f, conduitW - 12.0f);
        r.drawRect(pulseX, ly + 1, 12, 3, { 0.40f, 0.90f, 1.00f }, 0.80f);
    }

    // 5. GREEN ENERGY CRACKS (Doom / stone side)
    for (int lane = 0; lane < NUM_LANES; ++lane) {
        float ly = laneBottom(lane);
        for (int c = 5; c < GRID_COLS; c += 2) {
            float cx = GRID_LEFT + c * cellW + 28.0f;
            float cp = 0.25f + 0.75f * std::max(0.0f,
                std::sin(time * 1.8f + (float)lane * 1.1f + (float)c * 0.5f));
            r.drawRect(cx,     ly + 18,  3, 20, { 0.0f, cp * 0.55f, 0.0f  }, 0.85f * cp);
            r.drawRect(cx - 5, ly + 25, 13,  3, { 0.0f, cp * 0.60f, 0.05f }, 0.75f * cp);
            r.drawRect(cx - 3, ly + 16,  9, 24, { 0.0f, cp * 0.25f, 0.0f  }, 0.30f * cp);
        }
    }

    // 4. RUBBLE + DOOMBOT PARTS (fixed per-lane table, varied layout)
    // type 0 = rock cluster, 1 = doombot helmet, 2 = detached arm
    struct RubbleSpec { int lane; int col; float dx; float dy; int type; };
    static const RubbleSpec specs[16] = {
        { 0, 5, 10.0f, LANE_H - 22.0f, 0 }, { 0, 6, 52.0f, LANE_H - 18.0f, 0 },
        { 0, 7, 20.0f, 16.0f,           1 }, { 0, 8, 30.0f, 26.0f,           2 },
        { 1, 5, 58.0f, LANE_H - 16.0f, 0 }, { 1, 6, 16.0f, LANE_H - 22.0f, 0 },
        { 1, 7, 50.0f, 14.0f,           1 }, { 1, 8, 10.0f, 30.0f,           2 },
        { 2, 5, 30.0f, 24.0f,           2 }, { 2, 6, 62.0f, LANE_H - 18.0f, 0 },
        { 2, 7, 10.0f, LANE_H - 24.0f, 0 }, { 2, 8, 44.0f, 16.0f,           1 },
        { 3, 5, 52.0f, 16.0f,           1 }, { 3, 6, 14.0f, LANE_H - 16.0f, 0 },
        { 3, 7, 68.0f, 26.0f,           2 }, { 3, 8, 24.0f, LANE_H - 22.0f, 0 },
    };
    for (int si = 0; si < 16; ++si) {
        const RubbleSpec& s = specs[si];
        float bx = GRID_LEFT + (float)s.col * cellW + s.dx;
        float by = laneBottom(s.lane) + s.dy;
        if (s.type == 0) {
            // Rock cluster: 4 rocks with shadow underneath
            r.drawRect(bx,      by,        17, 10, { 0.11f, 0.12f, 0.09f }); // shadow
            r.drawRect(bx +  1, by +  1,   15,  9, { 0.22f, 0.24f, 0.18f }); // rock A
            r.drawRect(bx + 10, by,        10,  8, { 0.19f, 0.21f, 0.16f }); // rock B
            r.drawRect(bx +  4, by +  8,    9,  5, { 0.25f, 0.27f, 0.20f }); // rock C top
            r.drawRect(bx + 16, by +  2,    8,  6, { 0.20f, 0.22f, 0.17f }); // rock D
        } else if (s.type == 1) {
            // Doombot helmet: dome + face plate + eye slits + dent notch + spark
            r.drawRect(bx,      by,         22,  5, { 0.10f, 0.11f, 0.09f }); // shadow
            r.drawRect(bx +  1, by +  1,    20, 14, { 0.28f, 0.30f, 0.25f }); // dome
            r.drawRect(bx +  3, by +  4,    16,  8, { 0.36f, 0.38f, 0.32f }); // dome highlight
            r.drawRect(bx +  2, by + 10,    18, 10, { 0.24f, 0.26f, 0.21f }); // face plate
            r.drawRect(bx +  4, by + 12,     5,  3, { 0.06f, 0.07f, 0.05f }); // eye slit L
            r.drawRect(bx + 13, by + 12,     5,  3, { 0.06f, 0.07f, 0.05f }); // eye slit R
            r.drawRect(bx +  7, by + 17,     8,  2, { 0.10f, 0.11f, 0.08f }); // mouth grille
            r.drawRect(bx + 10, by +  5,     4,  5, { 0.18f, 0.19f, 0.16f }); // dent notch
            float sp = std::max(0.0f, std::sin(time * 3.5f + (float)s.lane * 2.1f));
            r.drawRect(bx +  5, by + 11,     5,  2,
                { 0.05f, 0.65f * sp, 0.10f * sp }, 0.90f * sp);            // green spark
        } else {
            // Detached arm: upper arm + round joint + forearm
            r.drawRect(bx,      by,          9,  5, { 0.10f, 0.11f, 0.09f }); // shadow
            r.drawRect(bx +  1, by +  1,     6, 14, { 0.26f, 0.28f, 0.23f }); // upper arm
            r.drawRect(bx +  2, by +  2,     4, 10, { 0.33f, 0.35f, 0.29f }); // highlight
            r.drawRect(bx -  1, by + 12,    10,  8, { 0.22f, 0.23f, 0.19f }); // joint body
            r.drawRect(bx,      by + 13,     8,  6, { 0.30f, 0.32f, 0.26f }); // joint highlight
            r.drawRect(bx -  2, by + 18,    14,  8, { 0.24f, 0.26f, 0.21f }); // forearm
            r.drawRect(bx -  1, by + 19,    12,  4, { 0.30f, 0.32f, 0.26f }); // forearm highlight
        }
    }

    // 9. BROKEN ENERGY FENCE at midline
    // 9 posts; 3 are broken (shorter, dark tip, fallen piece beside)
    // Cyan beams connect consecutive intact neighbours; 4 scorch marks
    {
        const float fieldH = GRID_TOP - gridBottom;
        static const bool broken[9] = {
            false, false, true, false, false, true, false, true, false };
        for (int i = 0; i < 9; ++i) {
            float py    = gridBottom + (float)i * (fieldH / 8.0f);
            bool  brk   = broken[i];
            float postH = brk ? 12.0f : 22.0f;
            r.drawRect(midX - 3, py, 6, postH, { 0.15f, 0.18f, 0.22f });
            r.drawRect(midX - 3, py, 1, postH, { 0.28f, 0.32f, 0.38f }); // left edge
            glm::vec3 tipCol = brk ? glm::vec3(0.12f, 0.14f, 0.16f)
                                   : glm::vec3(0.20f, 0.80f, 0.90f);
            r.drawRect(midX - 2, py + postH, 4, 4, tipCol);
            if (brk) {
                r.drawRect(midX + 5, py + 2, 14, 4, { 0.12f, 0.15f, 0.18f }); // fallen piece
                r.drawRect(midX + 5, py + 2, 14, 1, { 0.22f, 0.26f, 0.30f }); // top edge
            }
        }
        for (int i = 0; i < 8; ++i) {
            if (broken[i] || broken[i + 1]) continue;
            float y0 = gridBottom + (float)i       * (fieldH / 8.0f) + 22.0f;
            float y1 = gridBottom + (float)(i + 1) * (fieldH / 8.0f);
            float bh = y1 - y0;
            if (bh < 1.0f) bh = 1.0f;
            float ba = 0.25f + 0.20f * std::sin(time * 6.0f + (float)i);
            r.drawRect(midX - 1, y0, 2, bh, { 0.20f, 0.80f, 0.90f }, ba);
        }
        static const float scorchFrac[4] = { 0.15f, 0.38f, 0.62f, 0.85f };
        for (int i = 0; i < 4; ++i) {
            float sy = gridBottom + scorchFrac[i] * fieldH;
            float sx = (i % 2 == 0) ? midX - 28.0f : midX + 6.0f;
            r.drawRect(sx,     sy,        18, 12, { 0.05f, 0.06f, 0.05f }, 0.85f);
            r.drawRect(sx + 2, sy +  2,   14,  8, { 0.08f, 0.10f, 0.07f }, 0.70f);
            r.drawRect(sx + 4, sy +  4,   10,  4, { 0.12f, 0.14f, 0.10f }, 0.55f);
        }
    }
}

// Elements drawn at the very END (on top of everything in drawBattlefield):
//   1 - Doom Castle Gate (wide)   2 - Torches (on gate left face)
//   3 - Green Banners             7 - Holo-Terminals (in bottom strip)
//  10 - Bottom Strip (Stark crates + crumbled wall)
static void drawEnvironmentEdges(Renderer& r, float time)
{
    const float gridBottom = laneBottom(NUM_LANES - 1);
    const float gridW      = GRID_RIGHT - GRID_LEFT;
    const float cellW      = gridW / GRID_COLS;

    // 1 + 2. DOOM CASTLE GATE (x=1225..1280) + TORCHES on left face
    {
        const float GX      = 1225.0f;
        const float GW      = WORLD_W - GX;                    // 55
        const float GY      = gridBottom - 4.0f;
        const float GH      = GRID_TOP - gridBottom + 8.0f;    // 448
        const float GTOP    = GY + GH;
        const float pillarW = 12.0f;
        const float openX   = GX + pillarW;                    // 1237
        const float openW   = GW - 2.0f * pillarW;             // 31

        // Dark stone base fill
        r.drawRect(GX, GY, GW, GH, { 0.12f, 0.14f, 0.10f });

        // Block sizes cycling (3 sizes: 17, 14, 16)
        static const float blkSizes[3] = { 17.0f, 14.0f, 16.0f };

        // Left pillar blocks (staggered 2px on alternate rows)
        {
            float py = GY;
            int   bi = 0;
            while (py < GTOP - 12.0f && bi < 40) {
                float avail = GTOP - 12.0f - py;
                if (avail <= 0.0f) break;
                float bh = blkSizes[bi % 3];
                if (bh > avail) bh = avail;
                if (bh < 1.0f) break;
                float stagger = (bi % 2 == 0) ? 0.0f : 2.0f;
                float bw2     = pillarW - stagger;
                r.drawRect(GX + stagger, py,     bw2,         bh, { 0.17f, 0.19f, 0.14f });
                r.drawRect(GX + stagger, py,     bw2,          1, { 0.10f, 0.11f, 0.08f }); // mortar
                r.drawRect(GX + stagger, py + 1, bw2 - 1, bh - 2, { 0.21f, 0.23f, 0.17f }); // face
                py += bh;
                ++bi;
            }
            // Iron band
            r.drawRect(GX, GY + GH * 0.48f, pillarW, 5, { 0.16f, 0.17f, 0.18f });
            r.drawRect(GX, GY + GH * 0.48f, pillarW, 1, { 0.30f, 0.31f, 0.33f });
        }

        // Right pillar blocks (offset start for variety)
        {
            float rpx0 = GX + GW - pillarW;
            float py   = GY;
            int   bi   = 1;
            while (py < GTOP - 12.0f && bi < 40) {
                float avail = GTOP - 12.0f - py;
                if (avail <= 0.0f) break;
                float bh = blkSizes[bi % 3];
                if (bh > avail) bh = avail;
                if (bh < 1.0f) break;
                float stagger = (bi % 2 == 0) ? 0.0f : 2.0f;
                float rpx     = rpx0 + stagger;
                float bw2     = pillarW - stagger;
                r.drawRect(rpx, py,     bw2,         bh, { 0.17f, 0.19f, 0.14f });
                r.drawRect(rpx, py,     bw2,          1, { 0.10f, 0.11f, 0.08f });
                r.drawRect(rpx, py + 1, bw2 - 1, bh - 2, { 0.21f, 0.23f, 0.17f });
                py += bh;
                ++bi;
            }
            // Iron band
            r.drawRect(rpx0, GY + GH * 0.52f, pillarW, 5, { 0.16f, 0.17f, 0.18f });
            r.drawRect(rpx0, GY + GH * 0.52f, pillarW, 1, { 0.30f, 0.31f, 0.33f });
        }

        // Dark opening + portcullis per lane
        for (int lane = 0; lane < NUM_LANES; ++lane) {
            float lcy   = laneBottom(lane) + LANE_H * 0.5f;
            float archH = LANE_H * 0.76f;
            float archY = lcy - archH * 0.5f;

            // Dark arch opening
            r.drawRect(openX, archY, openW, archH, { 0.04f, 0.05f, 0.03f });

            // Portcullis: 5 vertical bars
            float barGap = (openW - 5.0f * 2.0f) / 6.0f;
            for (int vb = 0; vb < 5; ++vb) {
                float bxb = openX + barGap * (float)(vb + 1) + 2.0f * (float)vb;
                r.drawRect(bxb,     archY, 2, archH, { 0.18f, 0.18f, 0.20f });
                r.drawRect(bxb,     archY, 1, archH, { 0.30f, 0.30f, 0.33f }); // lighter edge
            }
            // 3 horizontal bars
            static const float hbFrac[3] = { 0.08f, 0.48f, 0.90f };
            for (int hb = 0; hb < 3; ++hb) {
                float hby = archY + hbFrac[hb] * archH;
                r.drawRect(openX, hby,     openW, 2, { 0.18f, 0.18f, 0.20f });
                r.drawRect(openX, hby,     openW, 1, { 0.30f, 0.30f, 0.33f }); // lighter edge
            }

            // Pulsing green energy at opening mouth
            float gp = 0.30f + 0.40f * std::sin(time * 2.2f + (float)lane * 0.9f);
            r.drawRect(openX, archY, 2, archH,
                { 0.06f * gp, 0.44f * gp, 0.06f * gp }, 0.70f * gp);
        }

        // Crenellations on top (6 slots, even indices = filled merlon)
        {
            float merW = GW / 6.0f;
            for (int m = 0; m < 6; m += 2) {
                float mx = GX + (float)m * merW;
                r.drawRect(mx, GTOP - 12, merW - 2, 12, { 0.16f, 0.18f, 0.13f });
                r.drawRect(mx, GTOP -  2, merW - 2, 10, { 0.20f, 0.23f, 0.16f }); // top face
                r.drawRect(mx, GTOP -  2, merW - 2,  2, { 0.26f, 0.30f, 0.20f }); // highlight
            }
        }

        // 2. TORCHES on left face of gate (iron bracket + bowl + flickering flame)
        for (int lane = 0; lane < NUM_LANES; ++lane) {
            float ty = laneBottom(lane) + LANE_H - 26.0f;
            float tx = GX;
            float fp = 0.55f + 0.45f * std::sin(time * 4.8f + (float)lane * 1.5f);
            // Iron bracket (horizontal bar sticking left out of pillar)
            r.drawRect(tx - 8, ty + 8,  10, 2, { 0.20f, 0.21f, 0.22f });
            r.drawRect(tx - 8, ty + 8,  10, 1, { 0.32f, 0.33f, 0.35f }); // highlight
            // Bowl
            r.drawRect(tx - 10, ty + 10,  8, 4, { 0.26f, 0.21f, 0.16f });
            r.drawRect(tx - 10, ty + 13,  8, 2, { 0.20f, 0.16f, 0.12f }); // rim
            // Flickering flame (top layer height = base + 3*sin + 2*sin)
            float fh = 6.0f + 3.0f * std::sin(time * 9.0f  + (float)lane)
                            + 2.0f * std::sin(time * 13.0f + (float)lane * 2.0f);
            if (fh < 2.0f) fh = 2.0f;
            r.drawRect(tx - 10, ty + 15,  8,  8,
                { 0.72f * fp, 0.22f * fp, 0.0f   }, 0.90f);
            r.drawRect(tx -  9, ty + 17,  6,  7,
                { 1.00f * fp, 0.52f * fp, 0.06f  }, 0.85f);
            r.drawRect(tx -  8, ty + 19,  4,  fh,
                { 1.00f,      0.80f * fp, 0.35f * fp }, fp);
        }
    }

    // 3. GREEN BANNERS (horizontal rod at top, Doom mask, swallowtail bottom)
    for (int b = 0; b < 3; ++b) {
        float bx   = GRID_LEFT + (float)(6 + b) * cellW + cellW * 0.35f;
        float bh   = 50.0f + (float)b * 6.0f;
        float bw   = 20.0f;
        float btop = GRID_TOP - 2.0f;
        float sway = 1.5f * std::sin(time * 1.3f + (float)b * 0.7f);
        float swayB = sway + 2.0f * std::sin(time * 1.5f + (float)b); // extra for tail

        // Horizontal hanging rod at top with end caps
        r.drawRect(bx - 3 + sway,      btop - 2, bw + 6, 4, { 0.30f, 0.26f, 0.20f });
        r.drawRect(bx - 3 + sway,      btop - 2,       4, 4, { 0.40f, 0.35f, 0.28f }); // left cap
        r.drawRect(bx + bw + 3 + sway, btop - 2,       4, 4, { 0.40f, 0.35f, 0.28f }); // right cap
        r.drawRect(bx - 3 + sway,      btop - 1, bw + 6, 1, { 0.48f, 0.42f, 0.34f }); // highlight

        // Banner body
        r.drawRect(bx + sway,     btop - bh - 2, bw,     bh, { 0.07f, 0.21f, 0.07f });
        r.drawRect(bx + 2 + sway, btop - bh,     bw - 4, bh - 4, { 0.09f, 0.28f, 0.09f });

        // Doom mask: face plate + forehead + jaw + 2 eye slits + mouth grille
        float maskY = btop - bh + bh * 0.28f;
        float maskW = bw - 6.0f;
        r.drawRect(bx + 3 + sway, maskY,        maskW,     16, { 0.28f, 0.30f, 0.26f }); // face plate
        r.drawRect(bx + 4 + sway, maskY +  9,   maskW - 2,  6, { 0.34f, 0.36f, 0.31f }); // forehead
        r.drawRect(bx + 4 + sway, maskY,        maskW - 2,  4, { 0.23f, 0.25f, 0.21f }); // jaw
        r.drawRect(bx + 4 + sway, maskY +  6,   5,          3, { 0.05f, 0.06f, 0.04f }); // eye L
        r.drawRect(bx + bw - 9 + sway, maskY + 6, 5,        3, { 0.05f, 0.06f, 0.04f }); // eye R
        r.drawRect(bx + 5 + sway, maskY +  2,   maskW - 4,  1, { 0.08f, 0.09f, 0.07f }); // mouth grille

        // Swallowtail bottom: 2 offset rects with notch
        float tailY = btop - bh - 2.0f;
        r.drawRect(bx + swayB,             tailY - 8, bw / 2.0f - 2.0f, 8,
            { 0.07f, 0.21f, 0.07f });  // left tail
        r.drawRect(bx + bw / 2.0f + 2.0f + swayB, tailY - 7, bw / 2.0f - 2.0f, 7,
            { 0.07f, 0.21f, 0.07f });  // right tail (slightly shorter)
    }

    // 7. STARK HOLO-TERMINALS in bottom strip (x ~152 and ~270, y 4..40)
    static const float termX[2] = { 152.0f, 270.0f };
    for (int t = 0; t < 2; ++t) {
        float tx = termX[t];
        float ty = 4.0f;
        float hp = 0.45f + 0.45f * std::sin(time * 2.1f + (float)t * 1.3f);
        // Base cabinet
        r.drawRect(tx,     ty,      60, 36, { 0.10f, 0.13f, 0.20f });
        r.drawRect(tx,     ty,      60,  2, { 0.18f, 0.24f, 0.36f }); // top edge
        r.drawRect(tx,     ty,       2, 36, { 0.18f, 0.24f, 0.36f }); // left edge
        // Keyboard bar
        r.drawRect(tx + 4, ty +  2, 52,  5, { 0.08f, 0.10f, 0.16f });
        r.drawRect(tx + 5, ty +  3, 50,  2, { 0.13f, 0.16f, 0.24f });
        // Angled screen (2 offset rects simulating tilt)
        r.drawRect(tx + 5, ty +  8, 46, 22, { 0.06f, 0.09f, 0.15f }); // bezel
        r.drawRect(tx + 6, ty +  7, 44, 22, { 0.04f, 0.07f, 0.12f }); // offset layer
        // 4 scanlines
        r.drawRect(tx + 8, ty +  9, 40,  2, { 0.08f * hp, 0.44f * hp, 0.72f * hp }, 0.90f);
        r.drawRect(tx + 8, ty + 13, 34,  2, { 0.08f * hp, 0.44f * hp, 0.72f * hp }, 0.70f);
        r.drawRect(tx + 8, ty + 17, 38,  2, { 0.08f * hp, 0.44f * hp, 0.72f * hp }, 0.60f);
        r.drawRect(tx + 8, ty + 21, 28,  2, { 0.08f * hp, 0.44f * hp, 0.72f * hp }, 0.50f);
        // 3 status lights at different blink rates (green / amber / red)
        float sl1 = 0.4f + 0.6f * std::max(0.0f, std::sin(time * 2.3f + (float)t));
        float sl2 = 0.4f + 0.6f * std::max(0.0f, std::sin(time * 3.7f + (float)t * 1.2f));
        float sl3 = 0.4f + 0.6f * std::max(0.0f, std::sin(time * 1.5f + (float)t * 0.8f));
        r.drawRect(tx +  4, ty + 4, 4, 4, { 0.10f, 0.70f * sl1, 0.10f });
        r.drawRect(tx + 10, ty + 4, 4, 4, { 0.70f * sl2, 0.55f * sl2, 0.0f });
        r.drawRect(tx + 16, ty + 4, 4, 4, { 0.70f * sl3, 0.10f,       0.10f });
    }

    // 10. BOTTOM STRIP: ledge, Stark crates (LEFT), crumbled stone wall (RIGHT)
    {
        float fgTop = gridBottom - 8.0f;  // top of floor-strip area

        // Ledge shadow + bright ledge line (kept from original)
        r.drawRect(0, fgTop - 10, WORLD_W, 10, { 0.06f, 0.08f, 0.12f }, 0.85f);
        r.drawRect(0, fgTop - 12, WORLD_W,  2, { 0.18f, 0.24f, 0.34f });

        // Stark crates (x ~345..680): 3 crates
        static const float crateX[3]  = { 345.0f, 455.0f, 568.0f };
        static const float crateH2[3] = {  28.0f,  24.0f,  30.0f };
        static const float crateW2[3] = {  44.0f,  38.0f,  46.0f };
        for (int ci = 0; ci < 3; ++ci) {
            float cx = crateX[ci];
            float cy = fgTop - crateH2[ci] - 1.0f;
            float cw = crateW2[ci];
            float ch = crateH2[ci];
            // Steel frame
            r.drawRect(cx,         cy,          cw,  ch, { 0.13f, 0.16f, 0.22f });
            r.drawRect(cx,         cy,          cw,   2, { 0.22f, 0.28f, 0.38f }); // top edge
            r.drawRect(cx,         cy,           2,  ch, { 0.22f, 0.28f, 0.38f }); // left edge
            r.drawRect(cx + cw-2,  cy,           2,  ch, { 0.10f, 0.12f, 0.17f }); // right shadow
            r.drawRect(cx,         cy + ch - 2, cw,   2, { 0.10f, 0.12f, 0.17f }); // bottom shadow
            // X cross-brace (4 steps per diagonal)
            for (int di = 0; di < 4; ++di) {
                float ddx = cx + 3.0f + (float)di * (cw - 6.0f) / 4.0f;
                float ddy = cy + 3.0f + (float)di * (ch - 6.0f) / 4.0f;
                r.drawRect(ddx, ddy, 3, 2, { 0.10f, 0.12f, 0.17f });
            }
            for (int di = 0; di < 4; ++di) {
                float ddx = cx + cw - 6.0f - (float)di * (cw - 6.0f) / 4.0f;
                float ddy = cy + 3.0f + (float)di * (ch - 6.0f) / 4.0f;
                r.drawRect(ddx, ddy, 3, 2, { 0.10f, 0.12f, 0.17f });
            }
            // Red/gold logo square
            r.drawRect(cx + cw / 2.0f - 5, cy + ch / 2.0f - 4, 10, 8, { 0.55f, 0.10f, 0.10f });
            r.drawRect(cx + cw / 2.0f - 3, cy + ch / 2.0f - 2,  6, 4, { 0.80f, 0.55f, 0.10f });
            // Yellow hazard stripe along the bottom
            r.drawRect(cx, cy + ch - 5, cw, 4, { 0.65f, 0.52f, 0.05f });
            for (int hi = 0; hi < 3; ++hi)
                r.drawRect(cx + 4.0f + (float)hi * (cw - 8.0f) / 3.0f,
                    cy + ch - 5, 4, 4, { 0.10f, 0.10f, 0.10f });
        }

        // Crumbled stone wall (x ~720..1220): 6 segments of uneven heights
        static const float wallX[6]  = { 720.0f, 808.0f, 896.0f, 990.0f, 1085.0f, 1175.0f };
        static const float wallH3[6] = {  22.0f,  16.0f,  28.0f,  12.0f,   24.0f,   18.0f };
        static const float wallW3[6] = {  70.0f,  72.0f,  78.0f,  68.0f,   74.0f,   70.0f };
        for (int wi = 0; wi < 6; ++wi) {
            float wx = wallX[wi];
            float wh = wallH3[wi];
            float ww = wallW3[wi];
            float wy = fgTop - wh;
            r.drawRect(wx,     wy,     ww, wh, { 0.16f, 0.18f, 0.13f });
            r.drawRect(wx + 1, wy + 1, ww - 2, wh - 2, { 0.20f, 0.22f, 0.16f }); // face
            // Mortar lines (2 horizontal)
            r.drawRect(wx, wy + wh * 0.40f, ww, 1, { 0.10f, 0.11f, 0.08f });
            r.drawRect(wx, wy + wh * 0.70f, ww, 1, { 0.10f, 0.11f, 0.08f });
            // Block seams (2 vertical)
            r.drawRect(wx + ww * 0.33f, wy, 1, wh, { 0.10f, 0.11f, 0.08f });
            r.drawRect(wx + ww * 0.66f, wy, 1, wh, { 0.10f, 0.11f, 0.08f });
            // Green moss tufts on top (3 small rects)
            r.drawRect(wx +  4, wy -  3,  8, 3, { 0.08f, 0.28f, 0.08f });
            r.drawRect(wx + 18, wy -  4,  6, 4, { 0.07f, 0.24f, 0.07f });
            r.drawRect(wx + 38, wy -  3,  7, 3, { 0.09f, 0.26f, 0.09f });
        }
        // 2 fallen blocks on the ground
        r.drawRect(880.0f,  fgTop - 8,  28, 8, { 0.17f, 0.19f, 0.14f });
        r.drawRect(881.0f,  fgTop - 7,  26, 6, { 0.21f, 0.23f, 0.17f });
        r.drawRect(882.0f,  fgTop - 7,  24, 1, { 0.27f, 0.30f, 0.22f }); // top highlight
        r.drawRect(1120.0f, fgTop - 6,  24, 6, { 0.17f, 0.19f, 0.14f });
        r.drawRect(1121.0f, fgTop - 5,  22, 4, { 0.21f, 0.23f, 0.17f });
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

    // [George] see-through board: the background image (drawn in main.cpp) shows through.
    // 0 = invisible board, 1 = fully solid. Raise it if the lanes get hard to read.
    const float BOARD_ALPHA = 0.45f;

    // background + top bar
    // [George] full-screen dark fill removed so the background image is visible:
    // r.drawRect(0, 0, WORLD_W, WORLD_H, { 0.05f, 0.07f, 0.11f });
    r.drawRect(0, 606, WORLD_W, WORLD_H - 606, { 0.07f, 0.11f, 0.19f });
    r.drawRect(0, 604, WORLD_W, 3, { 0.20f, 0.55f, 0.75f });

    // grid frame
    r.drawRect(GRID_LEFT - 4, gridBottom - 4, gridW + 8, GRID_TOP - gridBottom + 8, { 0.02f, 0.03f, 0.05f }, BOARD_ALPHA * 0.5f);   // [George] + alpha

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
            r.drawRect(x + 1, y + 1, cellW - 2, LANE_H - 2, col, BOARD_ALPHA);   // [George] + alpha
            // small detail per tile: rivets on tech tiles, cracks on stone
            glm::vec3 detail = col * 0.7f;
            if (tech) {
                r.drawRect(x + 6, y + 6, 4, 4, detail, BOARD_ALPHA);                       // [George] + alpha
                r.drawRect(x + cellW - 10, y + LANE_H - 10, 4, 4, detail, BOARD_ALPHA);
            }
            else {
                r.drawRect(x + 20, y + 30, 18, 3, detail, BOARD_ALPHA);                    // [George] + alpha
                r.drawRect(x + 36, y + 26, 3, 8, detail, BOARD_ALPHA);
            }
        }
    }

    drawEnvironmentFloor(r, time);

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

    // floor strip under the grid
    r.drawRect(0, 0, WORLD_W, gridBottom - 8, { 0.04f, 0.05f, 0.08f });

    drawEnvironmentEdges(r, time);
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
    float ratio = std::min(std::max((float)hp / (float)maxHp, 0.0f), 1.0f);
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