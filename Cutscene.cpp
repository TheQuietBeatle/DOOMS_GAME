// Cutscene.cpp - Pre-game intro cutscene (Iron Man vs. Doctor Doom, ~18 s).
// Self-contained: owns all geometry, timeline math, and subtitle data.
// Called by Game::render() when st == GameState::Cutscene; driven by
// Cutscene::update(dt) each frame and skipped via Cutscene::skip().
#include "Cutscene.h"
#include "Config.h"
#include "Sprites.h"

#include <cmath>
#include <string>

// ===========================================================================
// Small helpers
// ===========================================================================
namespace {

    constexpr int CUT_LANES = cfg::NUM_LANES;   // same lane count as the game

    float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
    float lerpf(float a, float b, float k) { return a + (b - a) * k; }

    // 0 before t0, 1 after t1, smooth in between
    float prog(float t, float t0, float t1)
    {
        float k = clamp01((t - t0) / (t1 - t0));
        return k * k * (3.0f - 2.0f * k);          // smoothstep
    }

    glm::vec3 mix3(glm::vec3 a, glm::vec3 b, float k)
    {
        return glm::vec3(lerpf(a.x, b.x, k), lerpf(a.y, b.y, k), lerpf(a.z, b.z, k));
    }

    // Filled circle from horizontal strips (no extra geometry needed)
    void drawDisc(Renderer& r, float cx, float cy, float rad, glm::vec3 c, float a)
    {
        if (rad <= 0.5f || a <= 0.0f) return;
        const int strips = 10;
        float h = (2.0f * rad) / strips;
        for (int i = 0; i < strips; ++i) {
            float y = -rad + (i + 0.5f) * h;
            float hw = std::sqrt(std::fmax(0.0f, rad * rad - y * y));
            r.drawRect(cx - hw, cy + y - h * 0.5f, hw * 2.0f, h + 0.5f, c, a);
        }
    }

    // Thick line made of small squares (used for lightning)
    void drawLine(Renderer& r, float x0, float y0, float x1, float y1,
        float thick, glm::vec3 c, float a)
    {
        float dx = x1 - x0, dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        int steps = (int)(len / (thick * 0.5f)) + 1;
        for (int i = 0; i <= steps; ++i) {
            float k = (float)i / steps;
            r.drawRect(x0 + dx * k - thick * 0.5f, y0 + dy * k - thick * 0.5f, thick, thick, c, a);
        }
    }

    // Draw a sprite so it is `height` px tall on screen (feet at x,y)
    void drawUnit(Renderer& r, SpriteId id, float x, float y, float height,
        bool flipX, float squashY = 1.0f, float alpha = 1.0f)
    {
        const SpriteFrame& f = g_sprites[id];
        if (f.h <= 0.0f) return;
        float s = height / f.h;
        DrawFx fx;
        fx.alpha = alpha;
        r.drawSprite(f, x, y, s * (2.0f - squashY), s * squashY, flipX, fx);
    }

    // Where a sprite drawn with drawUnit() actually lands on screen.
    // Mirrors Renderer::drawSprite's math, so beam start points follow the
    // characters automatically when you change their size or position.
    void unitBox(SpriteId id, float x, float y, float height, bool flipX,
        float& left, float& right, float& top)
    {
        const SpriteFrame& f = g_sprites[id];
        float s = (f.h > 0.0f) ? height / f.h : 0.0f;
        float w = f.w * s;
        if (!flipX) { left = x - w * 0.5f;  right = left + w * f.w; }
        else        { right = x + w * 0.5f; left = right - w * f.w; }
        top = y + f.h * s * f.h;
    }

    // where each character's hand is, as a fraction of its sprite box
    // (x: 0 = left edge, 1 = right edge;  y: 0 = feet, 1 = top of head)
    const float IM_HAND_FX   = 0.93f, IM_HAND_FY   = 0.40f;   // Iron Man's palm (facing right)
    const float DOOM_HAND_FX = 0.11f,  DOOM_HAND_FY = 0.68f;   // Doom's glowing hand (facing left)

    // Subtitle data
    struct Line {
        float t0, t1;          // show from t0 to t1 (seconds) - t1 is the END time, not a duration
        const char* speaker;   // "" = narrator caption
        glm::vec3 color;
        const char* text;
    };

    // Uppercase only: the pixel font is built for A-Z, 0-9 and basic punctuation.
    const glm::vec3 COL_NARR = { 0.85f, 0.90f, 1.00f };
    const glm::vec3 COL_DOOM = { 0.45f, 1.00f, 0.45f };
    const glm::vec3 COL_IRON = { 1.00f, 0.80f, 0.30f };

    const Line kLines[] = {
        { 0.4f,  3.0f, "",          COL_NARR, "NEW YORK. THE ARC REACTOR POWERS THE WHOLE CITY." },
        { 6.2f,  8.2f, "DOOM",      COL_DOOM, "STARK! THAT REACTOR WILL POWER LATVERIA. HAND IT OVER." },
        { 10.5f, 12.9f, "IRON MAN",  COL_IRON, "IT KEEPS THIS CITY ALIVE, VICTOR. NOT HAPPENING." },
        { 13.0f, 15.2f, "DOOM",      COL_DOOM, "THEN MY ARMY WILL TAKE IT, ONE LANE AT A TIME." },
        { 15.6f, 18.0f, "IRON MAN",  COL_IRON, "JARVIS, DEPLOY THE SUITS!" },
    };

} // namespace

// ===========================================================================
// Control
// ===========================================================================
void Cutscene::start() { t = 0.0f; done = false; }

void Cutscene::update(float dt)
{
    if (done) return;
    t += dt;
    if (t >= DURATION) { t = DURATION; done = true; }
}

void Cutscene::skip() { t = DURATION; done = true; }

// ===========================================================================
// Drawing
// ===========================================================================
void Cutscene::draw(Renderer& r) const
{
    const float W = cfg::WORLD_W;
    const float H = cfg::WORLD_H;
    const float groundY = H * 0.30f;           // top of the ground
    const float horizon = groundY;

    // ---- timeline values -------------------------------------------------
    const float kGreen = prog(t, 3.0f, 5.0f);        // right sky turns green
    const float kCastle = prog(t, 3.6f, 5.5f);        // castle fades in
    const float kDoomIn = prog(t, 3.8f, 6.0f);        // Doom floats in
    const float kMarch = prog(t, 6.0f, 9.0f);        // Doombots march in
    const float kFly = clamp01((t - 9.0f) / 1.5f); // Iron Man descent (linear, eased below)
    const float kBeam = prog(t, 12.0f, 12.6f);      // beams grow to the middle
    const float kBeamOff = prog(t, 14.6f, 15.0f);      // beams fade
    const float kSpread = prog(t, 15.0f, 16.8f);      // bots slide into lanes
    const float kGrid = prog(t, 15.2f, 16.6f);      // lane lines fade in
    const float kReturn = prog(t, 16.2f, 17.6f);      // Iron Man back to base

    // ---- screen shake during the clash ----------------------------------
    float sx = 0.0f, sy = 0.0f;
    if (t > 12.6f && t < 13.4f) {
        float amp = 7.0f * (1.0f - (t - 12.6f) / 0.8f);
        sx = std::sin(t * 90.0f) * amp;
        sy = std::cos(t * 70.0f) * amp;
    }

    // =====================================================================
    // 1. SKY  (vertical gradient, right half blends to Doom green)
    // =====================================================================
    const glm::vec3 skyTopBlue = { 0.20f, 0.50f, 0.90f };
    const glm::vec3 skyLowBlue = { 0.60f, 0.82f, 1.00f };
    const glm::vec3 skyTopGreen = { 0.04f, 0.14f, 0.07f };
    const glm::vec3 skyLowGreen = { 0.18f, 0.40f, 0.20f };
    const int bands = 16;
    float bandH = (H - horizon) / bands;
    for (int i = 0; i < bands; ++i) {
        float k = (float)i / (bands - 1);                 // 0 = low, 1 = top
        glm::vec3 blue = mix3(skyLowBlue, skyTopBlue, k);
        glm::vec3 green = mix3(skyLowGreen, skyTopGreen, k);
        float y = horizon + i * bandH;
        // left half: always blue (city); right half: blue -> green
        r.drawRect(sx, y + sy, W * 0.5f, bandH + 1.0f, blue);
        r.drawRect(W * 0.5f + sx, y + sy, W * 0.5f + 10.0f, bandH + 1.0f, mix3(blue, green, kGreen));
    }
    // soft blend strip in the middle so the split isn't a hard edge
    for (int i = 0; i < 6; ++i) {
        float x = W * 0.5f - 60.0f + i * 20.0f;
        r.drawRect(x + sx, horizon + sy, 20.0f, H - horizon,
            mix3(skyTopBlue, skyTopGreen, kGreen), 0.12f * kGreen);
    }

    // =====================================================================
    // 2. CITY SKYLINE (left)  - buildings with lit windows
    // =====================================================================
    const float bldW[] = { 46, 60, 38, 70, 52, 44, 64, 40 };
    const float bldH[] = { 150, 220, 120, 260, 180, 140, 210, 110 };
    float bx = W * 0.10f;
    for (int i = 0; i < 8; ++i) {
        float w = bldW[i] * (W / 1280.0f);
        float h = bldH[i] * (H / 720.0f);
        if (bx + w > W * 0.50f) break;
        glm::vec3 body = (i % 2) ? glm::vec3(0.30f, 0.40f, 0.58f) : glm::vec3(0.36f, 0.47f, 0.66f);
        r.drawRect(bx + sx, horizon + sy, w, h, body);
        r.drawRect(bx + sx, horizon + h + sy, w, 4.0f, glm::vec3(0.55f, 0.70f, 0.90f)); // roof trim
        if (i == 3) // antenna on the tallest
            r.drawRect(bx + w * 0.5f - 2.0f + sx, horizon + h + sy, 4.0f, 40.0f, glm::vec3(0.6f, 0.7f, 0.85f));
        for (float windowY = horizon + 12.0f; windowY < horizon + h - 14.0f; windowY += 22.0f)
            for (float windowX = bx + 7.0f; windowX < bx + w - 10.0f; windowX += 14.0f) {
                bool lit = ((int)(windowX * 7 + windowY * 3) % 5) != 0;
                r.drawRect(windowX + sx, windowY + sy, 7.0f, 10.0f,
                    lit ? glm::vec3(0.95f, 0.92f, 0.60f) : glm::vec3(0.20f, 0.28f, 0.40f));
            }
        bx += w + 8.0f;
    }

    // =====================================================================
    // 3. DOOM'S CASTLE (right) - fades in with the green sky
    // =====================================================================
    if (kCastle > 0.0f) {
        const glm::vec3 stone = { 0.22f, 0.26f, 0.22f };
        const glm::vec3 dark = { 0.12f, 0.15f, 0.12f };
        float cx = W * 0.62f, cw = W * 0.36f, wallH = H * 0.22f;
        r.drawRect(cx + sx, horizon + sy, cw, wallH, stone, kCastle);                 // wall
        for (float x = cx; x < cx + cw; x += 24.0f)                                   // battlements
            r.drawRect(x + sx, horizon + wallH + sy, 14.0f, 14.0f, stone, kCastle);
        const float towerX[] = { 0.64f, 0.78f, 0.92f };
        const float towerH[] = { 0.36f, 0.46f, 0.34f };
        for (int i = 0; i < 3; ++i) {
            float tx = W * towerX[i], tw = 52.0f, th = H * towerH[i];
            r.drawRect(tx + sx, horizon + sy, tw, th, stone, kCastle);
            for (int s = 0; s < 6; ++s) {                                             // stepped spire
                float sw = tw - s * 9.0f;
                r.drawRect(tx + (tw - sw) * 0.5f + sx, horizon + th + s * 10.0f + sy, sw, 10.0f, dark, kCastle);
            }
            r.drawRect(tx + tw * 0.5f - 5.0f + sx, horizon + th * 0.6f + sy, 10.0f, 18.0f,  // glowing window
                glm::vec3(0.40f, 1.0f, 0.40f), kCastle);
            float sway = std::sin(t * 3.0f + i) * 4.0f;                                // banner
            r.drawRect(tx + 8.0f + sx, horizon + th * 0.25f + sy, 14.0f, th * 0.3f,
                glm::vec3(0.10f, 0.45f, 0.15f), kCastle);
            r.drawRect(tx + 8.0f + sway * 0.5f + sx, horizon + th * 0.25f - 10.0f + sy, 14.0f, 10.0f,
                glm::vec3(0.10f, 0.45f, 0.15f), kCastle);
        }
        r.drawRect(cx + cw * 0.40f + sx, horizon + sy, 56.0f, wallH * 0.65f, dark, kCastle); // gate
        for (int i = 0; i < 2; ++i) {                                                    // torches
            float tx = cx + cw * 0.40f + (i ? 70.0f : -22.0f);
            float fl = 0.8f + 0.2f * std::sin(t * 17.0f + i * 2.0f);
            r.drawRect(tx + sx, horizon + 30.0f + sy, 6.0f, 26.0f, glm::vec3(0.35f, 0.25f, 0.15f), kCastle);
            drawDisc(r, tx + 3.0f + sx, horizon + 62.0f + sy, 8.0f * fl, glm::vec3(1.0f, 0.55f, 0.10f), kCastle);
            drawDisc(r, tx + 3.0f + sx, horizon + 63.0f + sy, 4.0f * fl, glm::vec3(1.0f, 0.95f, 0.50f), kCastle);
        }
    }

    // =====================================================================
    // 4. LIGHTNING  (two strikes, with a screen flash)
    // =====================================================================
    const float strikes[] = { 3.2f, 4.5f };
    for (int s = 0; s < 2; ++s) {
        float age = t - strikes[s];
        if (age < 0.0f || age > 0.35f) continue;
        float a = 1.0f - age / 0.35f;
        float x = W * (0.70f + 0.15f * s), y = H;
        for (int seg = 0; seg < 6; ++seg) {
            float nx = x + ((seg % 2) ? 26.0f : -22.0f);
            float ny = y - (H - horizon) / 6.0f;
            drawLine(r, x + sx, y + sy, nx + sx, ny + sy, 5.0f, glm::vec3(0.75f, 1.0f, 0.75f), a);
            x = nx; y = ny;
        }
        r.drawRect(0, 0, W, H, glm::vec3(0.8f, 1.0f, 0.8f), 0.25f * a);
    }

    // =====================================================================
    // 5. GROUND + LANES
    // =====================================================================
    r.drawRect(sx, sy, W * 0.5f, groundY, glm::vec3(0.16f, 0.30f, 0.45f));                  // tech floor
    r.drawRect(W * 0.5f + sx, sy, W * 0.5f + 10.0f, groundY,
        mix3(glm::vec3(0.16f, 0.30f, 0.45f), glm::vec3(0.20f, 0.26f, 0.18f), kGreen)); // stone floor
    for (float x = 0; x < W; x += 64.0f)                                                     // floor tiles
        r.drawRect(x + sx, sy, 2.0f, groundY, glm::vec3(0.30f, 0.60f, 0.85f), 0.25f);

    const float laneH = groundY / CUT_LANES;
    if (kGrid > 0.0f)
        for (int i = 0; i <= CUT_LANES; ++i)
            r.drawRect(W * 0.08f + sx, i * laneH - 1.5f + sy, W * 0.92f, 3.0f,
                glm::vec3(0.40f, 0.85f, 1.0f), 0.8f * kGrid);

    // =====================================================================
    // 6. STARK TOWER + ARC REACTOR (home base, far left)
    // =====================================================================
    {
        float tx = W * 0.01f, tw = W * 0.07f, th = H * 0.62f;
        r.drawRect(tx + sx, sy, tw, th, glm::vec3(0.75f, 0.82f, 0.90f));                        // body
        r.drawRect(tx + tw * 0.15f + sx, sy, tw * 0.7f, th, glm::vec3(0.45f, 0.70f, 0.90f));     // glass
        for (float y = 20.0f; y < th; y += 26.0f)                                               // floors
            r.drawRect(tx + sx, y + sy, tw, 3.0f, glm::vec3(0.85f, 0.90f, 0.95f));
        r.drawRect(tx - 8.0f + sx, th + sy, tw + 30.0f, 10.0f, glm::vec3(0.80f, 0.85f, 0.92f)); // landing pad
        r.drawRect(tx + tw * 0.4f + sx, th + 10.0f + sy, 4.0f, 30.0f, glm::vec3(0.7f, 0.75f, 0.8f));

        // the reactor: pulsing glow rings
        float pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
        float rx = tx + tw * 0.5f + sx, ry = groundY + 40.0f + sy;
        drawDisc(r, rx, ry, 42.0f + 8.0f * pulse, glm::vec3(0.30f, 0.80f, 1.0f), 0.18f);
        drawDisc(r, rx, ry, 28.0f, glm::vec3(0.55f, 0.75f, 0.85f), 1.0f);
        drawDisc(r, rx, ry, 21.0f, glm::vec3(0.40f, 0.90f, 1.0f), 1.0f);
        drawDisc(r, rx, ry, 10.0f + 3.0f * pulse, glm::vec3(0.95f, 1.0f, 1.0f), 1.0f);
    }

    // sentry drones in front of the tower (bobbing)
    for (int i = 0; i < 3; ++i) {
        float dx = W * 0.10f + sx;
        float dy = groundY * (0.2f + 0.3f * i) + std::sin(t * 2.5f + i) * 5.0f + sy;
        r.drawRect(dx - 12.0f, dy, 24.0f, 14.0f, glm::vec3(0.55f, 0.60f, 0.66f));
        r.drawRect(dx - 16.0f, dy + 4.0f, 32.0f, 4.0f, glm::vec3(0.35f, 0.40f, 0.45f));
        r.drawRect(dx - 4.0f, dy + 4.0f, 8.0f, 6.0f, glm::vec3(0.4f, 0.9f, 1.0f));
    }

    // =====================================================================
    // 7. DOOMBOTS  (march in, then spread into lanes)
    // =====================================================================
    struct Bot { SpriteId a, b; float height, targetX, gatherY; int lane; };
    const Bot bots[] = {
        { SPR_EASY_A, SPR_EASY_B, 2.0f, 0.70f, 0.12f, 0 },
        { SPR_EASY_A, SPR_EASY_B, 2.0f, 0.76f, 0.05f, 1 },
        { SPR_HARD_A, SPR_HARD_B, 3.0f, 0.66f, 0.02f, 2 },
        { SPR_EASY_A, SPR_EASY_B, 2.0f, 0.82f, 0.10f, 3 },
        { SPR_EASY_A, SPR_EASY_B, 2.0f, 0.88f, 0.04f, 4 },
    };
    for (int i = 0; i < 5; ++i) {
        const Bot& b = bots[i];
        float startX = W + 60.0f + i * 50.0f;
        float x = lerpf(startX, W * b.targetX, kMarch);
        int lane = b.lane < CUT_LANES ? b.lane : CUT_LANES - 1;
        float laneY = lane * laneH + 4.0f;
        float y = lerpf(groundY * b.gatherY + groundY * 0.55f, laneY, kSpread);
        bool moving = (kMarch > 0.0f && kMarch < 1.0f) || (kSpread > 0.0f && kSpread < 1.0f);
        SpriteId frame = (moving && ((int)(t * 6.0f + i) % 2)) ? b.b : b.a;
        if (t > 6.0f) drawUnit(r, frame, x + sx, y + sy, b.height, true);
    }

    // =====================================================================
    // 8. DOOM (floating sorcerer)
    // =====================================================================
    float doomX = lerpf(W + 80.0f, W * 1.0f, kDoomIn);
    float doomY = groundY - 5.0f + std::sin(t * 2.0f) * 6.0f;
    float doomH = 2.5f;
    if (t > 3.8f) {
        // green swirl around the hands
        float sw = 0.6f + 0.4f * std::sin(t * 8.0f);
        drawUnit(r, SPR_MEDIUM_A, doomX + sx, doomY + sy, doomH, true);
    }

    // =====================================================================
    // 9. IRON MAN  (flies down, lands, then returns to the tower)
    // =====================================================================
    float imX = W * 0.24f, imY = groundY * 0.45f, imH = 2.5f, squash = 1.0f;
    if (t >= 9.0f) {
        float k = kFly * kFly;                                     // accelerate downwards
        imY = lerpf(H + 120.0f, groundY * 0.45f, k);
        if (t > 10.5f && t < 10.9f)                                // landing squash
            squash = 1.0f - 0.22f * std::sin((t - 10.5f) / 0.4f * 3.14159f);

        imX = lerpf(imX, W * 0.10f, kReturn);                      // fly home at the end
        imY = lerpf(imY, groundY * 0.55f + 60.0f * std::sin(kReturn * 3.14159f), kReturn);

        bool flying = (t < 10.5f) || (kReturn > 0.0f && kReturn < 1.0f);
        if (flying)                                                // boot thrusters
            for (int j = 0; j < 4; ++j) {
                float fy = imY - 6.0f - j * 9.0f;
                drawDisc(r, imX + sx, fy + sy, 7.0f - j * 1.4f, glm::vec3(0.5f, 0.9f, 1.0f), 0.9f - j * 0.2f);
            }
        if (t > 10.5f && t < 11.3f) {                              // landing dust
            float d = (t - 10.5f) / 0.8f;
            for (int j = -1; j <= 1; j += 2)
                drawDisc(r, imX + j * (20.0f + 50.0f * d) + sx, imY + 6.0f + sy, 12.0f * (1.0f - d),
                    glm::vec3(0.75f, 0.80f, 0.85f), 0.7f * (1.0f - d));
        }
        SpriteId frame = (t > 12.0f && t < 15.0f) ? SPR_MELEE_B : SPR_MELEE_A;  // arm out during clash
        drawUnit(r, frame, imX + sx, imY + sy, imH, false, squash);
    }

    // =====================================================================
    // 10. BEAM CLASH  (repulsor vs. Doom bolt)
    // =====================================================================
    if (t > 12.0f && t < 15.0f) {
        float a = 1.0f - kBeamOff;
        float clashX = W * 0.52f;
        // beams start at the characters' hands (see IM_HAND_* / DOOM_HAND_*)
        float bl, br, bt;
        unitBox(SPR_MELEE_B, imX, imY, imH, false, bl, br, bt);
        float handIM = lerpf(bl, br, IM_HAND_FX), yIM = lerpf(imY, bt, IM_HAND_FY);
        unitBox(SPR_MEDIUM_A, doomX, doomY, doomH, true, bl, br, bt);
        float handDM = lerpf(bl, br, DOOM_HAND_FX), yDM = lerpf(doomY, bt, DOOM_HAND_FY);
        float flick = 0.85f + 0.15f * std::sin(t * 40.0f);

        float imEnd = lerpf(handIM, clashX, kBeam);
        float dmEnd = lerpf(handDM, clashX, kBeam);
        float yMid = lerpf(yIM, yDM, 0.5f);
        // blue repulsor beam
        drawLine(r, handIM + sx, yIM + sy, imEnd + sx, lerpf(yIM, yMid, kBeam) + sy, 14.0f * flick, glm::vec3(0.35f, 0.85f, 1.0f), 0.55f * a);
        drawLine(r, handIM + sx, yIM + sy, imEnd + sx, lerpf(yIM, yMid, kBeam) + sy, 5.0f, glm::vec3(0.90f, 1.0f, 1.0f), a);
        // green Doom bolt
        drawLine(r, handDM + sx, yDM + sy, dmEnd + sx, lerpf(yDM, yMid, kBeam) + sy, 14.0f * flick, glm::vec3(0.30f, 1.0f, 0.35f), 0.55f * a);
        drawLine(r, handDM + sx, yDM + sy, dmEnd + sx, lerpf(yDM, yMid, kBeam) + sy, 5.0f, glm::vec3(0.85f, 1.0f, 0.85f), a);

        if (kBeam >= 1.0f) {                                       // energy ball where they meet
            float p = 0.5f + 0.5f * std::sin(t * 25.0f);
            drawDisc(r, clashX + sx, yMid + sy, 34.0f + 10.0f * p, glm::vec3(0.55f, 1.0f, 0.80f), 0.35f * a);
            drawDisc(r, clashX + sx, yMid + sy, 18.0f + 4.0f * p, glm::vec3(1.0f, 1.0f, 1.0f), 0.90f * a);
        }
        if (t > 12.6f && t < 12.9f)                                // impact flash
            r.drawRect(0, 0, W, H, glm::vec3(1, 1, 1), 0.35f * (1.0f - (t - 12.6f) / 0.3f));
    }

    // =====================================================================
    // 11. UI: subtitles, "WAVE 1", skip hint, fades
    // =====================================================================
    for (const Line& L : kLines) {
        if (t < L.t0 || t > L.t1) continue;
        float a = std::fmin(prog(t, L.t0, L.t0 + 0.25f), 1.0f - prog(t, L.t1 - 0.25f, L.t1));
        std::string spk = L.speaker;
        std::string txt = L.text;
        float px = 3.0f;
        while (px > 1.0f && r.textWidth(txt, px) > W - 60.0f) px -= 0.5f;
        float barH = spk.empty() ? 7.0f * px + 30.0f : 7.0f * px * 2.0f + 46.0f;
        r.drawRect(0, 0, W, barH, glm::vec3(0, 0, 0), 0.65f * a);
        float ty = 14.0f;
        r.drawText(txt, (W - r.textWidth(txt, px)) * 0.5f, ty, px, glm::vec3(1, 1, 1), a);
        if (!spk.empty())
            r.drawText(spk, (W - r.textWidth(spk, px)) * 0.5f, ty + 7.0f * px + 12.0f, px, L.color, a);
    }

    if (t > 16.4f) {                                               // WAVE 1 banner
        float a = prog(t, 16.4f, 16.9f);
        std::string s = "WAVE 1";
        float px = 7.0f;
        float bw = r.textWidth(s, px) + 60.0f;
        r.drawRect((W - bw) * 0.5f, H * 0.62f - 20.0f, bw, 7.0f * px + 40.0f, glm::vec3(0.05f, 0.10f, 0.20f), 0.8f * a);
        r.drawText(s, (W - r.textWidth(s, 7.0f)) * 0.5f, H * 0.62f, px, glm::vec3(0.45f, 0.90f, 1.0f), a);
    }

    {   // skip hint, top right
        std::string s = "SPACE TO SKIP";
        r.drawText(s, W - r.textWidth(s, 2.0f) - 16.0f, H - 26.0f, 2.0f, glm::vec3(1, 1, 1), 0.6f);
    }

    float fadeIn = 1.0f - prog(t, 0.0f, 0.8f);
    float fadeOut = prog(t, 17.4f, 18.0f);
    float black = std::fmax(fadeIn, fadeOut);
    if (black > 0.0f) r.drawRect(0, 0, W, H, glm::vec3(0, 0, 0), black);
}