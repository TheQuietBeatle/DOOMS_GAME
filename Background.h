// Background.h - Full-screen background image drawn behind the game board.
// Owns a separate OpenGL shader, VAO/VBO, and one texture per loaded image.
// draw() must be called BEFORE renderer.beginFrame(fbW, fbH, false) so the
// renderer's sprites composite on top without erasing the background.
// Also provides drawPicker() (background chooser) and drawMenuSplit() (main menu).
//
//   Background bg;
//   bg.init(files, count);          // once, after the GL context exists
//   bg.draw(fbW, fbH);              // every frame, BEFORE renderer.beginFrame(fbW, fbH, false)
//   bg.next();                      // F2: switch to the next image
#pragma once
#include <glad/gl.h>
#include <vector>

class Background {
public:
    bool init(const char* const* paths, int count);
    void shutdown();

    // Clears the window (black bars), then draws the current image cover-fit into the 16:9 viewport.
    // dimOverride >= 0 overrides the dim member; pass -1 to use dim.
    void draw(int framebufferWidth, int framebufferHeight, float dimOverride = -1.0f) const;
    void next() { if (!imgs.empty()) current = (current + 1) % (int)imgs.size(); }

    // Background chooser (shown after the Main Menu, before the cutscene).
    void drawPicker(int framebufferWidth, int framebufferHeight, int hovered) const;   // dimmed current bg + a row of thumbnails
    int  thumbAt(float wx, float wy) const;                 // world coords (y up); -1 = none
    void thumbRect(int i, float& x, float& y, float& w, float& h) const;
    int  count() const { return (int)imgs.size(); }
    void setNames(const char* const* n, int c) { names.assign(n, n + c); }
    const char* name(int i) const { return (i >= 0 && i < (int)names.size()) ? names[i] : ""; }

    // Main Menu split background: left half = Stark city (blue tint), right = Latveria (green tint).
    void drawMenuSplit(int framebufferWidth, int framebufferHeight, int leftImg, int rightImg) const;

    int   current = 0;      // which image is shown
    float dim = 0.85f;      // < 1 darkens the image so the board and characters stand out

private:
    struct Img { GLuint id = 0; int w = 1, h = 1; };
    std::vector<Img> imgs;
    std::vector<const char*> names;
    void quad(float x, float y, float w, float h, GLuint tex, float r, float g, float b,
              float u0 = 0, float v0 = 0, float uw = 1, float vh = 1) const;
    GLuint prog = 0, vao = 0, vbo = 0;
    mutable bool tintMode = false;   // when true, next quad() converts the texture to monochrome then tints it
    GLint  uUVRect = -1, uDim = -1, uTex = -1, uRect = -1, uUseTex = -1, uColor = -1;
};
