// Background.cpp - Loads PNG background images as OpenGL textures and draws
// them cover-fit into the game's 16:9 letterboxed viewport.
// Also provides the background chooser (drawPicker) and main-menu split view.
// stb_image is included here (IMPLEMENTATION define) so it is compiled once.
#include "Background.h"
#include "Config.h"

// stb_image implementation lives here now (it used to be in TiltView.cpp).
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cstdio>

namespace {

// Vertex shader: maps a full-screen quad (-1..1) into a sub-rect of NDC,
// and remaps UV coordinates to implement cover-fit cropping.
// uRect  = target area in NDC (xy = bottom-left corner, zw = width/height)
// uUVRect= UV sub-rect for the crop (xy = bottom-left UV offset, zw = UV size)
const char* kBgVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;   // -1..1 (full viewport)
layout(location = 1) in vec2 aUV;    //  0..1
uniform vec4 uUVRect;                // xy = offset, zw = scale (cover-fit crop)
uniform vec4 uRect;                  // target rect in NDC: xy = min corner, zw = size
out vec2 vUV;
void main() {
    vUV = uUVRect.xy + aUV * uUVRect.zw;
    gl_Position = vec4(uRect.xy + (aPos * 0.5 + 0.5) * uRect.zw, 0.0, 1.0);
}
)";

// Fragment shader: three modes selected by uUseTex
//   0 = flat solid colour (uColor)
//   1 = textured (image pixel * uDim)
//   2 = monochrome tint: convert to luminance, gamma-boost, multiply by uColor
//       used by drawMenuSplit() to give each half a distinct colour cast
const char* kBgFrag = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
uniform float uDim;
uniform int   uUseTex;   // 0 = solid colour, 1 = textured, 2 = monochrome tint
uniform vec3  uColor;
out vec4 FragColor;
void main() {
    vec3 c = (uUseTex == 1) ? texture(uTex, vUV).rgb : uColor;
    // uUseTex == 2: convert to luminance then tint by uColor (Main Menu split)
    if (uUseTex == 2) {
        float l = dot(texture(uTex, vUV).rgb, vec3(0.299, 0.587, 0.114));
        c = pow(l, 1.4) * uColor;   // gamma 1.4 boosts contrast before tinting
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

    // --- one full-screen quad: vertex layout is [x, y, u, v] = 4 floats ---
    // Two triangles covering NDC (-1..1): bottom-left → bottom-right → top-right, then repeat.
    const float quad[] = {
        -1, -1, 0, 0,    1, -1, 1, 0,    1,  1, 1, 1,
        -1, -1, 0, 0,    1,  1, 1, 1,   -1,  1, 0, 1,
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    // attrib 0 = position (x, y): 2 floats at byte offset 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // attrib 1 = UV (u, v): 2 floats after the 2-float position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- images ---
    stbi_set_flip_vertically_on_load(true);   // flip so row 0 = bottom (OpenGL convention)
    for (int i = 0; i < count; ++i) {
        Img img;
        int ch = 0;
        // request 4 channels (RGBA) even for JPEGs so the texture format is always RGBA8
        unsigned char* data = stbi_load(paths[i], &img.w, &img.h, &ch, 4);
        if (!data) {
            printf("Background: failed to load %s\n", paths[i]);
            continue;                       // missing file: just skip it
        }
        glGenTextures(1, &img.id);
        glBindTexture(GL_TEXTURE_2D, img.id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // images may not be 4-byte aligned
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // bilinear: smooth downscale
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // bilinear: smooth upscale
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // no repeat on X
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // no repeat on Y
        stbi_image_free(data);   // CPU copy no longer needed once the texture is uploaded
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

void Background::draw(int framebufferWidth, int framebufferHeight, float dimOverride) const
{
    // clear the whole window (black bars), like Renderer::beginFrame does
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (imgs.empty() || !prog) return;

    // same centred 16:9 area as the game
    float scale = std::min(framebufferWidth / cfg::WORLD_W, framebufferHeight / cfg::WORLD_H);
    int vw = (int)(cfg::WORLD_W * scale), vh = (int)(cfg::WORLD_H * scale);
    glViewport((framebufferWidth - vw) / 2, (framebufferHeight - vh) / 2, vw, vh);

    glDisable(GL_BLEND);
    glUseProgram(prog);
    glUniform1f(uDim, dimOverride >= 0.0f ? dimOverride : dim);

    // cover-fit: fill the viewport without stretching; crop the overflow axis.
    // ra = viewport aspect ratio, ia = image aspect ratio.
    // If the image is wider than the viewport (ia > ra), we show a centred horizontal slice.
    // If taller,  we show a centred vertical  slice.
    const Img& im = imgs[current];
    float ra = cfg::WORLD_W / cfg::WORLD_H, ia = (float)im.w / (float)im.h;
    float u0 = 0, v0 = 0, uw = 1, vh2 = 1;
    if (ia > ra) { uw = ra / ia;  u0 = (1 - uw) * 0.5f; }   // crop left & right
    else         { vh2 = ia / ra; v0 = (1 - vh2) * 0.5f; }  // crop top & bottom
    quad(0, 0, cfg::WORLD_W, cfg::WORLD_H, im.id, 1, 1, 1, u0, v0, uw, vh2);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Renderer::beginFrame(fbW, fbH, false) re-enables blending and sets its own program/viewport.
}

// helpers + background chooser

// Draws one rect in WORLD coords (1280x720, y up) using the background shader.
// Requires prog to be bound and the 16:9 viewport already set.
// tex=0 draws a solid colour; tex>0 draws the texture (or monochrome if tintMode).
// Called by: draw(), drawPicker(), drawMenuSplit()
void Background::quad(float x, float y, float w, float h, GLuint tex, float r, float g, float b,
                      float u0, float v0, float uw, float vh) const
{
    // convert world coords (0..WORLD_W, 0..WORLD_H, y-up) to NDC (-1..1) for the vertex shader
    glUniform4f(uRect, x / cfg::WORLD_W * 2.0f - 1.0f, y / cfg::WORLD_H * 2.0f - 1.0f,
                       w / cfg::WORLD_W * 2.0f,        h / cfg::WORLD_H * 2.0f);
    glUniform4f(uUVRect, u0, v0, uw, vh);   // UV crop rect passed through to the vertex shader
    glUniform1i(uUseTex, tex ? (tintMode ? 2 : 1) : 0);   // tintMode -> monochrome luminance tint
    glUniform3f(uColor, r, g, b);
    glUniform1i(uTex, 0);           // sampler bound to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);   // 6 vertices = 2 triangles = 1 quad
}

void Background::thumbRect(int i, float& x, float& y, float& w, float& h) const
{
    int n = std::max(1, count());
    w = 205.0f;                       // 16:9 thumbnails, 205 px wide
    h = w * 9.0f / 16.0f;            // = ~115 px tall
    float gap = 30.0f;
    float total = n * w + (n - 1) * gap;             // total row width including gaps
    x = (cfg::WORLD_W - total) * 0.5f + i * (w + gap);  // centred in world width
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

void Background::drawPicker(int framebufferWidth, int framebufferHeight, int hovered) const
{
    // the currently chosen background, darker, behind the thumbnails
    draw(framebufferWidth, framebufferHeight, 0.35f);
    if (imgs.empty() || !prog) return;

    glUseProgram(prog);
    glUniform1f(uDim, 1.0f);   // thumbnails shown at full brightness (no dim)
    for (int i = 0; i < count(); ++i) {
        float x, y, w, h;
        thumbRect(i, x, y, w, h);
        bool sel = (i == current), hov = (i == hovered);
        float pad = hov ? 6.0f : 4.0f;   // hovered thumbnail gets a wider border
        // border colour: cyan-blue = selected, white = hovered, dim blue = default
        float r = sel ? 0.35f : (hov ? 0.85f : 0.20f);
        float g = sel ? 0.90f : (hov ? 0.92f : 0.30f);
        float b = sel ? 1.00f : (hov ? 1.00f : 0.42f);
        quad(x - pad, y - pad, w + 2 * pad, h + 2 * pad, 0, r, g, b);   // solid border rect

        // cover-fit crop for the thumbnail (same logic as draw(), but ra = thumbnail aspect)
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

// Main Menu: left half = Stark city (blue), right half = Latveria (green).
// Each half shows a cover-fit crop of one image, turned monochrome and tinted.
// Missing images fall back to flat dark colours.
void Background::drawMenuSplit(int framebufferWidth, int framebufferHeight, int leftImg, int rightImg) const
{
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!prog) return;

    float scale = std::min(framebufferWidth / cfg::WORLD_W, framebufferHeight / cfg::WORLD_H);
    int vw = (int)(cfg::WORLD_W * scale), vh = (int)(cfg::WORLD_H * scale);
    glViewport((framebufferWidth - vw) / 2, (framebufferHeight - vh) / 2, vw, vh);
    glDisable(GL_BLEND);
    glUseProgram(prog);
    glUniform1f(uDim, 1.0f);

    const float halfW = cfg::WORLD_W * 0.5f;
    // uBias controls which end of the image to favour when cropping horizontally:
    //   0.0 = keep the left side (Stark city towers), 1.0 = keep the right side (Doom castle)
    struct Side { int img; float x; float r, g, b; float uBias; };
    const Side sides[2] = {
        { leftImg,  0.0f,  0.20f, 0.34f, 0.72f, 0.10f },   // Stark: blue tint, towers on the left of bg5
        { rightImg, halfW, 0.16f, 0.46f, 0.18f, 1.00f },   // Doom: green tint, castle on the right of bg3
    };
    for (const Side& sd : sides) {
        if (sd.img < 0 || sd.img >= (int)imgs.size()) {
            // image not loaded: fall back to a flat dark colour so the split is still visible
            quad(sd.x, 0, halfW, cfg::WORLD_H, 0, sd.r * 0.35f, sd.g * 0.35f, sd.b * 0.35f);
            continue;
        }
        // cover-fit for a half-width viewport (ra = 0.5 * 16:9 = 8:9 aspect)
        const Img& im = imgs[sd.img];
        float ra = halfW / cfg::WORLD_H, ia = (float)im.w / (float)im.h;
        float u0 = 0, v0 = 0, uw = 1, vh2 = 1;
        if (ia > ra) { uw = ra / ia; u0 = (1 - uw) * sd.uBias; }  // bias selects crop side
        else         { vh2 = ia / ra; v0 = (1 - vh2) * 0.5f; }
        tintMode = true;   // tells quad() to use uUseTex=2 (monochrome luminance tint)
        quad(sd.x, 0, halfW, cfg::WORLD_H, im.id, sd.r, sd.g, sd.b, u0, v0, uw, vh2);
        tintMode = false;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
