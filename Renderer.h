#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <initializer_list>
#include <string>
#include <vector>

// One colour per sprite character, e.g. {'R', 0.73f, 0.24f, 0.20f}
struct PaletteEntry { char ch; float r, g, b; };

struct Palette {
    glm::vec3 color[128]{};
    bool      used[128]{};
    Palette(std::initializer_list<PaletteEntry> entries) {
        for (const PaletteEntry& e : entries) {
            unsigned char c = (unsigned char)e.ch;
            if (c < 128) { color[c] = glm::vec3(e.r, e.g, e.b); used[c] = true; }
        }
    }
};

// Where one animation frame lives inside the shared vertex buffer.
// w/h are in sprite pixels. Origin is bottom-centre (the feet).
struct SpriteFrame {
    GLint first = 0;
    GLint count = 0;
    float w = 0.0f;
    float h = 0.0f;
};

// Per-draw effects handled by the fragment shader
struct DrawFx {
    float red = 0.0f;   // 0..1  damage tint
    float gray = 0.0f;   // 0..1  grayscale (locked cards)
    float alpha = 1.0f;   // 0..1  transparency (ghost preview)
};

class Renderer {
public:
    bool init();
    void shutdown();

    // --- build phase: call addSprite for every frame, then upload() once ---
    SpriteFrame addSprite(const char* const* rows, int numRows, const Palette& pal);
    void upload();

    // --- per frame ---
    void beginFrame(int fbWidth, int fbHeight, bool clear = true);   // [George] clear=false keeps a background drawn before it
    void drawRect(float x, float y, float w, float h, glm::vec3 color, float alpha = 1.0f);
    void drawSprite(const SpriteFrame& f, float x, float y, float scaleX, float scaleY,
        bool flipX, const DrawFx& fx = DrawFx());
    void drawText(const std::string& s, float x, float y, float px, glm::vec3 color, float alpha = 1.0f);
    float textWidth(const std::string& s, float px) const;
    // [George] big title text: vertical colour gradient (top row -> bottom row), a dark
    // outline 'outline' px thick around every lit pixel and a drop shadow offset (dx, -dy).
    void drawTextStyled(const std::string& s, float x, float y, float px,
        glm::vec3 top, glm::vec3 bottom, glm::vec3 outlineColor, float outline,
        float shadowDx, float shadowDy, float alpha = 1.0f);

private:
    void draw(GLint first, GLint count, const glm::mat4& model, glm::vec4 tint, float red, float gray);

    GLuint program = 0, vao = 0, vbo = 0;
    GLint  uProj = -1, uModel = -1, uTint = -1, uRed = -1, uGray = -1;
    std::vector<float> verts;   // x, y, r, g, b per vertex
    SpriteFrame quad;           // unit square used for rectangles and text
};