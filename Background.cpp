#include "Background.h"
#include "Config.h"

// stb_image implementation lives here now (it used to be in TiltView.cpp).
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cstdio>

namespace {

const char* kBgVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;   // -1..1 (full viewport)
layout(location = 1) in vec2 aUV;    //  0..1
uniform vec4 uUVRect;                // xy = offset, zw = scale (cover-fit crop)
uniform vec4 uRect;                  // [George] target rect in NDC: xy = min corner, zw = size
out vec2 vUV;
void main() {
    vUV = uUVRect.xy + aUV * uUVRect.zw;
    gl_Position = vec4(uRect.xy + (aPos * 0.5 + 0.5) * uRect.zw, 0.0, 1.0);
}
)";

const char* kBgFrag = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
uniform float uDim;
uniform int   uUseTex;   // [George] 0 = solid colour (thumbnail borders)
uniform vec3  uColor;
out vec4 FragColor;
void main() {
    vec3 c = (uUseTex == 1) ? texture(uTex, vUV).rgb : uColor;
    // [George] 2 = monochrome image tinted by uColor (Main Menu split background)
    if (uUseTex == 2) {
        float l = dot(texture(uTex, vUV).rgb, vec3(0.299, 0.587, 0.114));
        c = pow(l, 1.4) * uColor;
    }
    FragColor = vec4(c * uDim, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log); printf("Background shader: %s\n", log); }
    return s;
}

} // namespace

bool Background::init(const char* const* paths, int count)
{
    // --- shader ---
    GLuint vs = compileShader(GL_VERTEX_SHADER, kBgVert);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kBgFrag);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { printf("Background: shader link failed\n"); return false; }
    uUVRect = glGetUniformLocation(prog, "uUVRect");
    uDim    = glGetUniformLocation(prog, "uDim");
    uTex    = glGetUniformLocation(prog, "uTex");
    uRect   = glGetUniformLocation(prog, "uRect");
    uUseTex = glGetUniformLocation(prog, "uUseTex");
    uColor  = glGetUniformLocation(prog, "uColor");

    // --- one full-screen quad: x, y, u, v ---
    const float quad[] = {
        -1, -1, 0, 0,    1, -1, 1, 0,    1,  1, 1, 1,
        -1, -1, 0, 0,    1,  1, 1, 1,   -1,  1, 0, 1,
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- images ---
    stbi_set_flip_vertically_on_load(true);
    for (int i = 0; i < count; ++i) {
        Img img;
        int ch = 0;
        unsigned char* data = stbi_load(paths[i], &img.w, &img.h, &ch, 4);
        if (!data) {
            printf("Background: failed to load %s\n", paths[i]);
            continue;                       // missing file: just skip it
        }
        glGenTextures(1, &img.id);
        glBindTexture(GL_TEXTURE_2D, img.id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(data);
        imgs.push_back(img);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    if (current >= (int)imgs.size()) current = 0;
    return true;
}

void Background::shutdown()
{
    for (Img& im : imgs) if (im.id) glDeleteTextures(1, &im.id);
    imgs.clear();
    if (vbo)  glDeleteBuffers(1, &vbo);
    if (vao)  glDeleteVertexArrays(1, &vao);
    if (prog) glDeleteProgram(prog);
    vbo = vao = prog = 0;
}

void Background::draw(int fbW, int fbH, float dimOverride) const
{
    // clear the whole window (black bars), like Renderer::beginFrame does
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (imgs.empty() || !prog) return;

    // same centred 16:9 area as the game
    float scale = std::min(fbW / cfg::WORLD_W, fbH / cfg::WORLD_H);
    int vw = (int)(cfg::WORLD_W * scale), vh = (int)(cfg::WORLD_H * scale);
    glViewport((fbW - vw) / 2, (fbH - vh) / 2, vw, vh);

    glDisable(GL_BLEND);
    glUseProgram(prog);
    glUniform1f(uDim, dimOverride >= 0.0f ? dimOverride : dim);

    // cover-fit: fill the area without stretching, crop the overflow
    const Img& im = imgs[current];
    float ra = cfg::WORLD_W / cfg::WORLD_H, ia = (float)im.w / (float)im.h;
    float u0 = 0, v0 = 0, uw = 1, vh2 = 1;
    if (ia > ra) { uw = ra / ia;  u0 = (1 - uw) * 0.5f; }
    else         { vh2 = ia / ra; v0 = (1 - vh2) * 0.5f; }
    quad(0, 0, cfg::WORLD_W, cfg::WORLD_H, im.id, 1, 1, 1, u0, v0, uw, vh2);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Renderer::beginFrame(fbW, fbH, false) re-enables blending and sets its own program/viewport.
}

// ---------------------------------------------------------------------------
// [George] helpers + background chooser
// ---------------------------------------------------------------------------

// One rect in WORLD coords (1280x720, y up), textured or a solid colour.
// Expects prog bound and the 16:9 viewport set.
void Background::quad(float x, float y, float w, float h, GLuint tex, float r, float g, float b,
                      float u0, float v0, float uw, float vh) const
{
    glUniform4f(uRect, x / cfg::WORLD_W * 2.0f - 1.0f, y / cfg::WORLD_H * 2.0f - 1.0f,
                       w / cfg::WORLD_W * 2.0f,        h / cfg::WORLD_H * 2.0f);
    glUniform4f(uUVRect, u0, v0, uw, vh);
    glUniform1i(uUseTex, tex ? (tintMode ? 2 : 1) : 0);   // [George] tintMode -> monochrome tint
    glUniform3f(uColor, r, g, b);
    glUniform1i(uTex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Background::thumbRect(int i, float& x, float& y, float& w, float& h) const
{
    int n = std::max(1, count());
    w = 205.0f;                       // 16:9 thumbnails
    h = w * 9.0f / 16.0f;
    float gap = 30.0f;
    float total = n * w + (n - 1) * gap;
    x = (cfg::WORLD_W - total) * 0.5f + i * (w + gap);
    y = 300.0f;
}

int Background::thumbAt(float wx, float wy) const
{
    for (int i = 0; i < count(); ++i) {
        float x, y, w, h;
        thumbRect(i, x, y, w, h);
        if (wx >= x && wx <= x + w && wy >= y && wy <= y + h) return i;
    }
    return -1;
}

void Background::drawPicker(int fbW, int fbH, int hovered) const
{
    // the currently chosen background, darker, behind the thumbnails
    draw(fbW, fbH, 0.35f);
    if (imgs.empty() || !prog) return;

    glUseProgram(prog);
    glUniform1f(uDim, 1.0f);
    for (int i = 0; i < count(); ++i) {
        float x, y, w, h;
        thumbRect(i, x, y, w, h);
        bool sel = (i == current), hov = (i == hovered);
        float pad = hov ? 6.0f : 4.0f;
        float r = sel ? 0.35f : (hov ? 0.85f : 0.20f);
        float g = sel ? 0.90f : (hov ? 0.92f : 0.30f);
        float b = sel ? 1.00f : (hov ? 1.00f : 0.42f);
        quad(x - pad, y - pad, w + 2 * pad, h + 2 * pad, 0, r, g, b);   // border

        const Img& im = imgs[i];
        float ra = w / h, ia = (float)im.w / (float)im.h;
        float u0 = 0, v0 = 0, uw = 1, vh = 1;
        if (ia > ra) { uw = ra / ia; u0 = (1 - uw) * 0.5f; }
        else         { vh = ia / ra; v0 = (1 - vh) * 0.5f; }
        quad(x, y, w, h, im.id, 1, 1, 1, u0, v0, uw, vh);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ---------------------------------------------------------------------------
// [George] Main Menu: left half = Stark city (blue), right half = Latveria (green).
// Each half shows a cover-fit crop of one image, turned monochrome and tinted.
// Missing images fall back to flat dark colours.
// ---------------------------------------------------------------------------
void Background::drawMenuSplit(int fbW, int fbH, int leftImg, int rightImg) const
{
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!prog) return;

    float scale = std::min(fbW / cfg::WORLD_W, fbH / cfg::WORLD_H);
    int vw = (int)(cfg::WORLD_W * scale), vh = (int)(cfg::WORLD_H * scale);
    glViewport((fbW - vw) / 2, (fbH - vh) / 2, vw, vh);
    glDisable(GL_BLEND);
    glUseProgram(prog);
    glUniform1f(uDim, 1.0f);

    const float halfW = cfg::WORLD_W * 0.5f;
    struct Side { int img; float x; float r, g, b; float uBias; };   // uBias 0 = left crop, 1 = right crop
    const Side sides[2] = {
        { leftImg,  0.0f,  0.20f, 0.34f, 0.72f, 0.10f },   // Stark: towers on the left of bg5
        { rightImg, halfW, 0.16f, 0.46f, 0.18f, 1.00f },   // Doom: castle on the right of bg3
    };
    for (const Side& sd : sides) {
        if (sd.img < 0 || sd.img >= (int)imgs.size()) {
            quad(sd.x, 0, halfW, cfg::WORLD_H, 0, sd.r * 0.35f, sd.g * 0.35f, sd.b * 0.35f);
            continue;
        }
        const Img& im = imgs[sd.img];
        float ra = halfW / cfg::WORLD_H, ia = (float)im.w / (float)im.h;
        float u0 = 0, v0 = 0, uw = 1, vh2 = 1;
        if (ia > ra) { uw = ra / ia; u0 = (1 - uw) * sd.uBias; }
        else         { vh2 = ia / ra; v0 = (1 - vh2) * 0.5f; }
        tintMode = true;
        quad(sd.x, 0, halfW, cfg::WORLD_H, im.id, sd.r, sd.g, sd.b, u0, v0, uw, vh2);
        tintMode = false;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
