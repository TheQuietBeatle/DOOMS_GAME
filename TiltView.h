#pragma once
// =====================================================================
//  TiltView - shows the finished game frame tilted in perspective over
//  a background image, plus a small "choose your background" screen.
//
//  It does NOT touch Game / Renderer / Sprites. The game renders exactly
//  as before, just into an off-screen texture (FBO). TiltView then draws:
//    1) the chosen background image (full screen)
//    2) a soft shadow, then the board part of the frame (world y < splitY),
//       tilted, smaller, with far lanes slightly fogged for depth
//    3) the UI part of the frame (world y >= splitY) -> flat, on top
// =====================================================================

#include <glad/gl.h>
#include <string>
#include <vector>

class TiltView {
public:
    // worldW/worldH = cfg::WORLD_W / cfg::WORLD_H. supersample = 2 keeps
    // the tilted board sharp.
    bool init(int worldW, int worldH, int supersample = 2);
    void shutdown();

    // Load up to 5 background images (png/jpg). Missing files show grey.
    void loadBackgrounds(const char* const* paths, int count);
    void setBackgroundNames(const char* const* names, int count);
    // ---- Game pass: wrap renderer.beginFrame + game.render with these ----
    void beginGamePass();          // binds the off-screen target
    void endGamePass();            // unbinds it
    int  fboWidth()  const { return fboW; }
    int  fboHeight() const { return fboH; }

    // ---- Screen pass ----
    void present(int fbW, int fbH);                 // bg + tilted board + flat UI
    void drawPicker(int fbW, int fbH, float time);  // background chooser screen

    // ---- Input helpers (all in world coords, y up) ----
    // Converts a point on the screen to the game's flat world coords.
    // Returns false if the point is not on the tilted board.
    bool screenToBoard(float sx, float sy, float& gx, float& gy) const;
    int  thumbAt(float wx, float wy) const;         // -1 if none

    // ---- Settings you can tweak ----
    bool  enabled = true;    // F1 toggles
    float tiltDeg = 30.0f;   // F3 / F4 change it (0 = flat, 75 = max)
    float camDistFac = 1.2f;    // camera distance (smaller = stronger perspective)
    float vanishYFac = 0.9f;    // vanishing point height (lower = far lanes shrink more)

    float splitY = 0.0f;    // world y where the flat UI strip starts (init: 84% of height)
    float cropBottom = 0.0f;    // world y where your lanes start: cuts the empty band under the board
    float boardScale = 0.90f;   // shrink the board so more background shows (1 = full width)
    float boardBaseFac = 0.04f;   // height of the board's near edge, fraction of world height

    float fogAmount = 0.35f;   // 0 = off; darkens/blues the far lanes for depth
    float shadowAmount = 0.35f;   // 0 = off; soft shadow under the board
    float bgDim = 0.85f;   // < 1 darkens the background so the board pops
    float groundColor[4] = { 0.11f, 0.12f, 0.14f, 1.0f };   // ground below the horizon
    float uiStretch = 1.3f;    // height of the top bar (1 = original, 1.3 = 30% taller)

    // Where the (stretched) top bar starts on screen, and how to map a
    // mouse y inside it back to the game's own y.
    float uiTop() const { return H - (H - splitY) * uiStretch; }
    float uiToGame(float wy) const { return H - (H - wy) / uiStretch; }

    int   currentBg = 0;
    int   hoveredThumb = -1;
    int   bgCount() const { return (int)bgs.size(); }

private:
    struct Img { GLuint id = 0; int w = 1, h = 1; };

    void beginScreen(int fbW, int fbH);
    void drawRect(float x, float y, float w, float h,
        const float color[4], GLuint tex,
        float u0, float v0, float uw, float vh, bool tilted);
    void drawImageCover(const Img& img, float x, float y, float w, float h, float dim);
    void drawLabel(const std::string& s, float centerX, float y, float px, const float color[4]);
    void thumbRect(int i, float& x, float& y, float& w, float& h) const;

    float W = 1, H = 1;
    int   fboW = 0, fboH = 0;
    GLuint fbo = 0, colorTex = 0, depthRb = 0;

    GLuint prog = 0, vao = 0, vbo = 0;
    GLint uWorld, uTilted, uTilt, uCamDist, uVanish, uUVRect, uTex, uUseTex, uColor;
    GLint uScale, uBase, uCrop, uDepthRange, uFog, uFogColor;
    std::vector<std::string> bgNames;
    std::vector<Img> bgs;
};