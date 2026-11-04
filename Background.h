#pragma once
// ---------------------------------------------------------------------------
// [George] Full-screen background image drawn behind the (flat) game board.
// Replaces the old TiltView: no tilt, no off-screen pass, no picker screen.
//
//   Background bg;
//   bg.init(files, count);          // once, after the GL context exists
//   bg.draw(fbW, fbH);              // every frame, BEFORE renderer.beginFrame(fbW, fbH, false)
//   bg.next();                      // F2: switch to the next image
// ---------------------------------------------------------------------------
#include <glad/gl.h>
#include <vector>

class Background {
public:
    bool init(const char* const* paths, int count);
    void shutdown();

    void draw(int fbW, int fbH, float dimOverride = -1.0f) const;   // clears the window, then draws the image (cover-fit, letterboxed like the game)
    void next() { if (!imgs.empty()) current = (current + 1) % (int)imgs.size(); }

    // [George] background chooser (shown after the Main Menu)
    void drawPicker(int fbW, int fbH, int hovered) const;   // dimmed current bg + a row of thumbnails
    int  thumbAt(float wx, float wy) const;                 // world coords (y up); -1 = none
    void thumbRect(int i, float& x, float& y, float& w, float& h) const;
    int  count() const { return (int)imgs.size(); }
    void setNames(const char* const* n, int c) { names.assign(n, n + c); }
    const char* name(int i) const { return (i >= 0 && i < (int)names.size()) ? names[i] : ""; }

    int   current = 0;      // which image is shown
    float dim = 0.85f;      // < 1 darkens the image so the board and characters stand out

private:
    struct Img { GLuint id = 0; int w = 1, h = 1; };
    std::vector<Img> imgs;
    std::vector<const char*> names;
    void quad(float x, float y, float w, float h, GLuint tex, float r, float g, float b,
              float u0 = 0, float v0 = 0, float uw = 1, float vh = 1) const;
    GLuint prog = 0, vao = 0, vbo = 0;
    GLint  uUVRect = -1, uDim = -1, uTex = -1, uRect = -1, uUseTex = -1, uColor = -1;
};
