#include "TiltView.h"

// If your project ALREADY defines STB_IMAGE_IMPLEMENTATION in another
// .cpp file, delete the next line (keep the #include).
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

const char* const* fontGlyphRows(char c);   // defined in Renderer.cpp
namespace {

    const float PI = 3.14159265f;

    const char* kVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;   // flat world coords (y up)
layout(location = 1) in vec2 aUV;
uniform vec2  uWorld;
uniform int   uTilted;
uniform float uTilt;        // radians
uniform float uCamDist;     // world units
uniform vec2  uVanish;      // vanishing point, world units
uniform float uScale;       // board size multiplier
uniform float uBase;        // screen y of the board's near edge
uniform float uCrop;        // world y where the board starts
uniform float uDepthRange;  // board depth in world units (for fog)
uniform vec4  uUVRect;      // xy offset, zw scale
out vec2  vUV;
out float vDepth;
void main() {
    vUV = uUVRect.xy + aUV * uUVRect.zw;
    if (uTilted == 1) {
        // Shrink around the center, rotate backwards around the near edge,
        // then project with a pinhole camera. Depth goes into w so the
        // texture stays perspective-correct.
        float dIn = aPos.y - uCrop;
        float d   = dIn * uScale;
        float x   = uVanish.x + (aPos.x - uVanish.x) * uScale;
        float y   = uBase + d * cos(uTilt);
        float z   = d * sin(uTilt);
        float w   = uCamDist + z;
        float xn  = uVanish.x * w + (x - uVanish.x) * uCamDist;
        float yn  = uVanish.y * w + (y - uVanish.y) * uCamDist;
        gl_Position = vec4(2.0 * xn / uWorld.x - w, 2.0 * yn / uWorld.y - w, 0.0, w);
        vDepth = clamp(dIn / uDepthRange, 0.0, 1.0);
    } else {
        gl_Position = vec4(aPos / uWorld * 2.0 - 1.0, 0.0, 1.0);
        vDepth = 0.0;
    }
}
)";

    const char* kFrag = R"(
#version 330 core
in vec2  vUV;
in float vDepth;
out vec4 FragColor;
uniform sampler2D uTex;
uniform int   uUseTex;
uniform vec4  uColor;
uniform float uFog;
uniform vec3  uFogColor;
void main() {
    vec4 c = (uUseTex == 1) ? texture(uTex, vUV) * uColor : uColor;
    c.rgb = mix(c.rgb, uFogColor, vDepth * uFog);
    FragColor = c;
}
)";

    GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log); printf("TiltView shader: %s\n", log); }
        return s;
    }

    void setCap(GLenum cap, GLboolean on) { if (on) glEnable(cap); else glDisable(cap); }

    // Saves the GL state the game's Renderer might rely on, and restores it
    // when it goes out of scope, so TiltView never breaks the original code.
    struct GLStateGuard {
        GLint prog, vao, vbo, fbo, active, tex, vp[4];
        GLint srcRGB, dstRGB, srcA, dstA;
        GLboolean blend, depth, cull, scissor;
        GLfloat clear[4];
        GLStateGuard() {
            glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
            glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
            glActiveTexture(GL_TEXTURE0);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
            glGetIntegerv(GL_VIEWPORT, vp);
            glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
            glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
            glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcA);
            glGetIntegerv(GL_BLEND_DST_ALPHA, &dstA);
            blend = glIsEnabled(GL_BLEND);
            depth = glIsEnabled(GL_DEPTH_TEST);
            cull = glIsEnabled(GL_CULL_FACE);
            scissor = glIsEnabled(GL_SCISSOR_TEST);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
        }
        ~GLStateGuard() {
            glUseProgram(prog);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glActiveTexture(active);
            glViewport(vp[0], vp[1], vp[2], vp[3]);
            glBlendFuncSeparate(srcRGB, dstRGB, srcA, dstA);
            setCap(GL_BLEND, blend);
            setCap(GL_DEPTH_TEST, depth);
            setCap(GL_CULL_FACE, cull);
            setCap(GL_SCISSOR_TEST, scissor);
            glClearColor(clear[0], clear[1], clear[2], clear[3]);
        }
    };

} // namespace

// ---------------------------------------------------------------------
bool TiltView::init(int worldW, int worldH, int supersample) {
    GLStateGuard guard;

    W = (float)worldW;
    H = (float)worldH;
    splitY = H * 0.84f;
    fboW = worldW * supersample;
    fboH = worldH * supersample;

    // Off-screen target the game renders into
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboW, fboH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    glGenRenderbuffers(1, &depthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fboW, fboH);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRb);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("TiltView: framebuffer incomplete\n");
        return false;
    }

    // Shader
    GLuint vs = compile(GL_VERTEX_SHADER, kVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFrag);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { printf("TiltView: shader link failed\n"); return false; }

    uWorld = glGetUniformLocation(prog, "uWorld");
    uTilted = glGetUniformLocation(prog, "uTilted");
    uTilt = glGetUniformLocation(prog, "uTilt");
    uCamDist = glGetUniformLocation(prog, "uCamDist");
    uVanish = glGetUniformLocation(prog, "uVanish");
    uScale = glGetUniformLocation(prog, "uScale");
    uBase = glGetUniformLocation(prog, "uBase");
    uCrop = glGetUniformLocation(prog, "uCrop");
    uDepthRange = glGetUniformLocation(prog, "uDepthRange");
    uUVRect = glGetUniformLocation(prog, "uUVRect");
    uTex = glGetUniformLocation(prog, "uTex");
    uUseTex = glGetUniformLocation(prog, "uUseTex");
    uColor = glGetUniformLocation(prog, "uColor");
    uFog = glGetUniformLocation(prog, "uFog");
    uFogColor = glGetUniformLocation(prog, "uFogColor");

    // One dynamic quad (6 verts: x, y, u, v)
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return true;
}

void TiltView::shutdown() {
    for (Img& b : bgs) if (b.id) glDeleteTextures(1, &b.id);
    bgs.clear();
    if (fbo)      glDeleteFramebuffers(1, &fbo);
    if (colorTex) glDeleteTextures(1, &colorTex);
    if (depthRb)  glDeleteRenderbuffers(1, &depthRb);
    if (vbo)      glDeleteBuffers(1, &vbo);
    if (vao)      glDeleteVertexArrays(1, &vao);
    if (prog)     glDeleteProgram(prog);
}

void TiltView::loadBackgrounds(const char* const* paths, int count) {
    GLStateGuard guard;
    stbi_set_flip_vertically_on_load(true);

    for (int i = 0; i < count; i++) {
        Img img;
        int ch;
        unsigned char* data = stbi_load(paths[i], &img.w, &img.h, &ch, 4);
        if (!data) {
            printf("TiltView: failed to load %s\n", paths[i]);
            img.w = img.h = 1;
        }
        else {
            glGenTextures(1, &img.id);
            glBindTexture(GL_TEXTURE_2D, img.id);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            stbi_image_free(data);
        }
        bgs.push_back(img);
    }
}

// ---------------------------------------------------------------------
// Game pass
// ---------------------------------------------------------------------
void TiltView::beginGamePass() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fboW, fboH);
}

void TiltView::endGamePass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLint prevActive, prevTex;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glGenerateMipmap(GL_TEXTURE_2D);       // keeps the far lanes from shimmering
    glBindTexture(GL_TEXTURE_2D, prevTex);
    glActiveTexture(prevActive);
}

// ---------------------------------------------------------------------
// Screen pass helpers
// ---------------------------------------------------------------------
void TiltView::beginScreen(int fbW, int fbH) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, fbW, fbH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Same letterboxing as toWorld() in main.cpp
    float scale = std::min(fbW / W, fbH / H);
    int vw = (int)(W * scale), vh = (int)(H * scale);
    glViewport((fbW - vw) / 2, (fbH - vh) / 2, vw, vh);

    glUseProgram(prog);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(uTex, 0);
    glUniform2f(uWorld, W, H);
    glUniform1f(uTilt, tiltDeg * PI / 180.0f);
    glUniform1f(uCamDist, camDistFac * H);
    glUniform2f(uVanish, W * 0.5f, vanishYFac * H);
    glUniform1f(uScale, boardScale);
    glUniform1f(uBase, boardBaseFac * H);
    glUniform1f(uCrop, cropBottom);
    glUniform1f(uDepthRange, std::max(1.0f, splitY - cropBottom));
    glUniform1f(uFog, 0.0f);
    glUniform3f(uFogColor, 0.06f, 0.09f, 0.16f);
}

void TiltView::drawRect(float x, float y, float w, float h,
    const float color[4], GLuint tex,
    float u0, float v0, float uw, float vh, bool tilted) {
    float verts[] = {
        x,     y,     0, 0,
        x + w, y,     1, 0,
        x + w, y + h, 1, 1,
        x,     y,     0, 0,
        x + w, y + h, 1, 1,
        x,     y + h, 0, 1,
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glUniform1i(uTilted, tilted ? 1 : 0);
    glUniform4f(uUVRect, u0, v0, uw, vh);
    glUniform4fv(uColor, 1, color);
    glUniform1i(uUseTex, tex ? 1 : 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Fills a rect with an image without stretching it (crops the overflow)
void TiltView::drawImageCover(const Img& img, float x, float y, float w, float h, float dim) {
    float col[4] = { dim, dim, dim, 1.0f };
    if (!img.id) {
        float grey[4] = { 0.25f * dim, 0.25f * dim, 0.3f * dim, 1.0f };
        drawRect(x, y, w, h, grey, 0, 0, 0, 1, 1, false);
        return;
    }
    float ra = w / h, ia = (float)img.w / (float)img.h;
    float u0 = 0, v0 = 0, uw = 1, vh = 1;
    if (ia > ra) { uw = ra / ia; u0 = (1 - uw) * 0.5f; }
    else { vh = ia / ra; v0 = (1 - vh) * 0.5f; }
    drawRect(x, y, w, h, col, img.id, u0, v0, uw, vh, false);
}

// ---------------------------------------------------------------------
// Present the game: background + shadow + tilted board + flat UI
// ---------------------------------------------------------------------
void TiltView::present(int fbW, int fbH) {
    GLStateGuard guard;
    beginScreen(fbW, fbH);

    float depth = splitY - cropBottom;

    // 1) Background image, full screen
    if (!bgs.empty())
        drawImageCover(bgs[currentBg], 0, 0, W, H, bgDim);

    // 2) Soft shadow: a few growing, faint tilted rects behind the board
    if (shadowAmount > 0.0f) {
        for (int k = 3; k >= 1; k--) {
            float g = W * 0.015f * k;
            float sh[4] = { 0.0f, 0.0f, 0.0f, shadowAmount / 3.0f };
            drawRect(-g, cropBottom - g * 0.8f, W + 2 * g, depth + g * 1.6f, sh, 0, 0, 0, 1, 1, true);
        }
    }

    // 3) Dark rim, then the board itself (tilted, with depth fog)
    float rim[4] = { 0.03f, 0.04f, 0.06f, 0.9f };
    float white[4] = { 1, 1, 1, 1 };
    drawRect(-W * 0.008f, cropBottom - H * 0.008f, W * 1.016f, depth + H * 0.016f,
        rim, 0, 0, 0, 1, 1, true);

    glUniform1f(uFog, fogAmount);
    drawRect(0, cropBottom, W, depth, white, colorTex,
        0, cropBottom / H, 1, depth / H, true);
    glUniform1f(uFog, 0.0f);

    // 4) UI strip (cards, coins, wave bar) stays flat on top
    drawRect(0, splitY, W, H - splitY, white, colorTex, 0, splitY / H, 1, 1 - splitY / H, false);
}

// ---------------------------------------------------------------------
// Background chooser
// ---------------------------------------------------------------------
void TiltView::thumbRect(int i, float& x, float& y, float& w, float& h) const {
    int n = std::max(1, bgCount());
    w = W * 0.16f;
    h = w * 9.0f / 16.0f;
    float gap = W * 0.02f;
    float total = n * w + (n - 1) * gap;
    x = (W - total) * 0.5f + i * (w + gap);
    y = H * 0.42f;
}

int TiltView::thumbAt(float wx, float wy) const {
    for (int i = 0; i < bgCount(); i++) {
        float x, y, w, h;
        thumbRect(i, x, y, w, h);
        if (wx >= x && wx <= x + w && wy >= y && wy <= y + h) return i;
    }
    return -1;
}
void TiltView::setBackgroundNames(const char* const* names, int count) {
    bgNames.clear();
    for (int i = 0; i < count; i++) bgNames.push_back(names[i] ? names[i] : "");
}

// Draws text with the game's 5x7 font, one small square per lit pixel (flat, untextured)
void TiltView::drawLabel(const std::string& s, float centerX, float y, float px, const float color[4]) {
    float width = (float)s.size() * 6.0f * px - px;
    float cx = centerX - width * 0.5f;
    for (char ch : s) {
        const char* const* rows = fontGlyphRows(ch);
        if (rows) {
            for (int r = 0; r < 7; r++)
                for (int c = 0; c < 5; c++)
                    if (rows[r][c] == '#')
                        drawRect(cx + c * px, y + (6 - r) * px, px + 0.4f, px + 0.4f,
                            color, 0, 0, 0, 1, 1, false);
        }
        cx += 6.0f * px;   // advance even for spaces / unknown chars
    }
}
void TiltView::drawPicker(int fbW, int fbH, float time) {
    GLStateGuard guard;
    beginScreen(fbW, fbH);
    if (bgs.empty()) return;

    // Hovered (or current) background as a dim full-screen preview
    int preview = (hoveredThumb >= 0) ? hoveredThumb : currentBg;
    drawImageCover(bgs[preview], 0, 0, W, H, 0.35f);

    // Panel
    float x0, y0, tw, th;
    thumbRect(0, x0, y0, tw, th);
    float xl, yl, twl, thl;
    thumbRect(bgCount() - 1, xl, yl, twl, thl);
    float panelX = x0 - W * 0.025f, panelW = (xl + twl) - x0 + W * 0.05f;
    float panel[4] = { 0.02f, 0.04f, 0.08f, 0.78f };
    drawRect(panelX, y0 - H * 0.12f, panelW, th + H * 0.22f, panel, 0, 0, 0, 1, 1, false);

    // Pulsing accent line
    float glow[4] = { 0.3f, 0.8f, 1.0f, 0.6f + 0.4f * sinf(time * 3.0f) };
    drawRect(panelX, y0 + th + H * 0.07f, panelW, H * 0.006f, glow, 0, 0, 0, 1, 1, false);

    for (int i = 0; i < bgCount(); i++) {
        float x, y, w, h;
        thumbRect(i, x, y, w, h);
        bool hover = (i == hoveredThumb);
        if (hover) {
            float g = w * 0.06f;
            x -= g / 2; y -= g * 9.0f / 32.0f; w += g; h += g * 9.0f / 16.0f;
        }

        float border[4] = { 0.15f, 0.18f, 0.25f, 1.0f };
        if (i == currentBg) { border[0] = 0.2f; border[1] = 0.85f; border[2] = 0.35f; }
        if (hover) { border[0] = 0.3f; border[1] = 0.85f; border[2] = 1.0f; }
        float b = W * 0.004f;
        drawRect(x - b, y - b, w + 2 * b, h + 2 * b, border, 0, 0, 0, 1, 1, false);
        drawImageCover(bgs[i], x, y, w, h, hover ? 1.0f : 0.75f);

        // Number under the thumbnail as small squares (1..5) - no font needed
                // Name label under the thumbnail, e.g. "1 WAKANDA"
        float ox, oy, ow, oh;
        thumbRect(i, ox, oy, ow, oh);                 // un-enlarged rect, so the label doesn't jump on hover
        std::string label = std::to_string(i + 1);
        if (i < (int)bgNames.size() && !bgNames[i].empty()) label += " " + bgNames[i];

        float px = W * 0.0022f;                       // font pixel size
        float labelW = (float)label.size() * 6.0f * px - px;
        if (labelW > ow) px *= ow / labelW;           // shrink long names to fit the thumbnail

        float textCol[4] = { 0.80f, 0.80f, 0.85f, 1.0f };                               // normal
        if (i == currentBg) { textCol[0] = 0.20f; textCol[1] = 0.85f; textCol[2] = 0.35f; } // selected
        if (hover) { textCol[0] = 0.30f; textCol[1] = 0.85f; textCol[2] = 1.00f; } // hovered
        drawLabel(label, ox + ow * 0.5f, oy - H * 0.05f, px, textCol);
    }
}

// ---------------------------------------------------------------------
// Mouse on screen -> flat game world (exact inverse of the vertex shader)
// ---------------------------------------------------------------------
bool TiltView::screenToBoard(float sx, float sy, float& gx, float& gy) const {
    float t = tiltDeg * PI / 180.0f;
    float s = sinf(t), c = cosf(t);
    float D = camDistFac * H;
    float cx = W * 0.5f, cy = vanishYFac * H;
    float base = boardBaseFac * H;

    float denom = c * D - (sy - cy) * s;
    if (denom <= 1e-4f) return false;

    float d = D * (sy - base) / denom;     // distance up the scaled board
    float dIn = d / boardScale;            // back to the game's own units
    if (dIn < 0.0f || dIn > splitY - cropBottom) return false;

    float z = d * s;
    float xs = cx + (sx - cx) * (D + z) / D;
    float x = cx + (xs - cx) / boardScale;
    if (x < 0.0f || x > W) return false;

    gx = x;
    gy = dIn + cropBottom;
    return true;
}