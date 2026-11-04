#include "Renderer.h"
#include "Config.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>

namespace {

    const char* kVertexSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 u_Proj;
uniform mat4 u_Model;

out vec3 vColor;

void main()
{
    gl_Position = u_Proj * u_Model * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

    const char* kFragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

uniform vec4  u_Tint;
uniform float u_Red;    // damage: blend towards red
uniform float u_Gray;   // locked cards: blend towards grayscale

void main()
{
    vec3 c = vColor * u_Tint.rgb;
    c = mix(c, vec3(1.0, 0.12, 0.10), u_Red);
    float g = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(c, vec3(g), u_Gray);
    FragColor = vec4(c, u_Tint.a);
}
)";

    GLuint compileShader(GLenum type, const char* src)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "Shader compile error:\n" << log << std::endl;
        }
        return s;
    }

    // ---------------------------------------------------------------------------
    // 5x7 pixel font ('#' = lit). Lowercase is drawn as uppercase.
    // ---------------------------------------------------------------------------
    struct Glyph { char c; const char* rows[7]; };

    const Glyph kFont[] = {
        { '0',{ " ### ","#   #","#  ##","# # #","##  #","#   #"," ### " } },
        { '1',{ "  #  "," ##  ","  #  ","  #  ","  #  ","  #  "," ### " } },
        { '2',{ " ### ","#   #","    #","   # ","  #  "," #   ","#####" } },
        { '3',{ "#####","   # ","  #  ","   # ","    #","#   #"," ### " } },
        { '4',{ "   # ","  ## "," # # ","#  # ","#####","   # ","   # " } },
        { '5',{ "#####","#    ","#### ","    #","    #","#   #"," ### " } },
        { '6',{ "  ## "," #   ","#    ","#### ","#   #","#   #"," ### " } },
        { '7',{ "#####","    #","   # ","  #  "," #   "," #   "," #   " } },
        { '8',{ " ### ","#   #","#   #"," ### ","#   #","#   #"," ### " } },
        { '9',{ " ### ","#   #","#   #"," ####","    #","   # "," ##  " } },
        { 'A',{ " ### ","#   #","#   #","#####","#   #","#   #","#   #" } },
        { 'B',{ "#### ","#   #","#   #","#### ","#   #","#   #","#### " } },
        { 'C',{ " ### ","#   #","#    ","#    ","#    ","#   #"," ### " } },
        { 'D',{ "#### ","#   #","#   #","#   #","#   #","#   #","#### " } },
        { 'E',{ "#####","#    ","#    ","#### ","#    ","#    ","#####" } },
        { 'F',{ "#####","#    ","#    ","#### ","#    ","#    ","#    " } },
        { 'G',{ " ### ","#   #","#    ","# ###","#   #","#   #"," ####" } },
        { 'H',{ "#   #","#   #","#   #","#####","#   #","#   #","#   #" } },
        { 'I',{ " ### ","  #  ","  #  ","  #  ","  #  ","  #  "," ### " } },
        { 'J',{ "  ###","   # ","   # ","   # ","   # ","#  # "," ##  " } },
        { 'K',{ "#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #" } },
        { 'L',{ "#    ","#    ","#    ","#    ","#    ","#    ","#####" } },
        { 'M',{ "#   #","## ##","# # #","# # #","#   #","#   #","#   #" } },
        { 'N',{ "#   #","#   #","##  #","# # #","#  ##","#   #","#   #" } },
        { 'O',{ " ### ","#   #","#   #","#   #","#   #","#   #"," ### " } },
        { 'P',{ "#### ","#   #","#   #","#### ","#    ","#    ","#    " } },
        { 'Q',{ " ### ","#   #","#   #","#   #","# # #","#  # "," ## #" } },
        { 'R',{ "#### ","#   #","#   #","#### ","# #  ","#  # ","#   #" } },
        { 'S',{ " ####","#    ","#    "," ### ","    #","    #","#### " } },
        { 'T',{ "#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  " } },
        { 'U',{ "#   #","#   #","#   #","#   #","#   #","#   #"," ### " } },
        { 'V',{ "#   #","#   #","#   #","#   #","#   #"," # # ","  #  " } },
        { 'W',{ "#   #","#   #","#   #","# # #","# # #","# # #"," # # " } },
        { 'X',{ "#   #","#   #"," # # ","  #  "," # # ","#   #","#   #" } },
        { 'Y',{ "#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  " } },
        { 'Z',{ "#####","    #","   # ","  #  "," #   ","#    ","#####" } },
        { ':',{ "     ","  #  ","  #  ","     ","  #  ","  #  ","     " } },
        { '/',{ "    #","    #","   # ","  #  "," #   ","#    ","#    " } },
        { '!',{ "  #  ","  #  ","  #  ","  #  ","  #  ","     ","  #  " } },
        { '-',{ "     ","     ","     ","#####","     ","     ","     " } },
        { '.',{ "     ","     ","     ","     ","     "," ##  "," ##  " } },
        { ',',{ "     ","     ","     ","     "," ##  ","  #  "," #   " } },   // [George] comma for cutscene subtitles
        { '+',{ "     ","  #  ","  #  ","#####","  #  ","  #  ","     " } },
        { '%',{ "##   ","##  #","   # ","  #  "," #   ","#  ##","   ##" } },
        { '(',{ "   # ","  #  "," #   "," #   "," #   ","  #  ","   # " } },
        { ')',{ " #   ","  #  ","   # ","   # ","   # ","  #  "," #   " } },
        { '[',{ " ### "," #   "," #   "," #   "," #   "," #   "," ### " } },
        { ']',{ " ### ","   # ","   # ","   # ","   # ","   # "," ### " } },
    };

    const Glyph* findGlyph(char c)
    {
        c = (char)std::toupper((unsigned char)c);
        for (const Glyph& g : kFont)
            if (g.c == c) return &g;
        return nullptr;   // space / unknown -> blank
    }

}
// namespace
const char* const* fontGlyphRows(char c)
{
    const Glyph* g = findGlyph(c);
    return g ? g->rows : nullptr;
}
// ---------------------------------------------------------------------------

bool Renderer::init()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader link error:\n" << log << std::endl;
        return false;
    }

    uProj = glGetUniformLocation(program, "u_Proj");
    uModel = glGetUniformLocation(program, "u_Model");
    uTint = glGetUniformLocation(program, "u_Tint");
    uRed = glGetUniformLocation(program, "u_Red");
    uGray = glGetUniformLocation(program, "u_Gray");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // first 6 vertices: white unit square (0,0)-(1,1) for rectangles + text
    const float q[6][2] = { { 0,0 },{ 1,0 },{ 1,1 },{ 1,1 },{ 0,1 },{ 0,0 } };
    quad.first = 0;
    for (auto& p : q) verts.insert(verts.end(), { p[0], p[1], 1.0f, 1.0f, 1.0f });
    quad.count = 6;
    quad.w = quad.h = 1.0f;
    return true;
}

void Renderer::shutdown()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
}

SpriteFrame Renderer::addSprite(const char* const* rows, int numRows, const Palette& pal)
{
    SpriteFrame f;
    f.first = (GLint)(verts.size() / 5);

    int width = 0;
    for (int r = 0; r < numRows; ++r)
        width = std::max(width, (int)std::strlen(rows[r]));
    f.w = (float)width;
    f.h = (float)numRows;

    for (int row = 0; row < numRows; ++row) {
        const char* line = rows[row];
        int len = (int)std::strlen(line);
        for (int col = 0; col < width; ++col) {
            unsigned char ch = (col < len) ? (unsigned char)line[col] : (unsigned char)' ';
            if (!pal.used[ch]) continue;
            glm::vec3 c = pal.color[ch];
            // pixel at (col, numRows - 1 - row) so origin bottom-left
            float x = (float)col;
            float y = (float)(numRows - 1 - row);
            // two triangles forming a unit square
            verts.insert(verts.end(), { x, y, c.r, c.g, c.b });
            verts.insert(verts.end(), { x + 1.0f, y, c.r, c.g, c.b });
            verts.insert(verts.end(), { x + 1.0f, y + 1.0f, c.r, c.g, c.b });
            verts.insert(verts.end(), { x + 1.0f, y + 1.0f, c.r, c.g, c.b });
            verts.insert(verts.end(), { x, y + 1.0f, c.r, c.g, c.b });
            verts.insert(verts.end(), { x, y, c.r, c.g, c.b });
        }
    }

    f.count = (GLint)((verts.size() / 5) - f.first);
    return f;
}

void Renderer::upload()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // pos: vec2 at 0, color: vec3 at offset 2
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::beginFrame(int fbWidth, int fbHeight, bool clear)
{
    // clear the WHOLE window first (black bars)
    glViewport(0, 0, fbWidth, fbHeight);
    if (clear) {   // [George] skipped when a background image was already drawn this frame
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // then draw the game in the largest 16:9 area that fits, centred
    float scale = std::min(fbWidth / cfg::WORLD_W, fbHeight / cfg::WORLD_H);
    int vw = (int)(cfg::WORLD_W * scale);
    int vh = (int)(cfg::WORLD_H * scale);
    glViewport((fbWidth - vw) / 2, (fbHeight - vh) / 2, vw, vh);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program);
    // projection in WORLD pixels (1280x720), not window pixels,
    // so the game scales with the window
    glm::mat4 proj = glm::ortho(0.0f, cfg::WORLD_W, 0.0f, cfg::WORLD_H);
    glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));
    glBindVertexArray(vao);
}

void Renderer::draw(GLint first, GLint count, const glm::mat4& model, glm::vec4 tint, float red, float gray)
{
    glUseProgram(program);
    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(uTint, 1, glm::value_ptr(tint));
    glUniform1f(uRed, red);
    glUniform1f(uGray, gray);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, first, count);
    glBindVertexArray(0);
}

void Renderer::drawRect(float x, float y, float w, float h, glm::vec3 color, float alpha)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    draw(quad.first, quad.count, model, glm::vec4(color, alpha), 0.0f, 0.0f);
}

void Renderer::drawSprite(const SpriteFrame& f, float x, float y, float scaleX, float scaleY,
    bool flipX, const DrawFx& fx)
{
    // origin is bottom-centre
    float w = f.w * scaleX;
    float h = f.h * scaleY;
    float sx = flipX ? -w : w;
    float tx = x - (w * 0.5f);
    if (flipX) tx += w; // adjust because of negative scale

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(tx, y, 0.0f));
    model = glm::scale(model, glm::vec3(sx, h, 1.0f));

    // Tint of 1.0 means use vertex colors directly
    glm::vec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
    draw(f.first, f.count, model, tint, fx.red, fx.gray);
}

void Renderer::drawText(const std::string& s, float x, float y, float px, glm::vec3 color, float alpha)
{
    float cx = x;
    float scale = px; // px is pixel scale
    for (char ch : s) {
        const Glyph* g = findGlyph(ch);
        if (!g) { cx += scale * 6.0f; continue; }
        // each glyph is 5x7 with 1 px spacing
        for (int row = 0; row < 7; ++row) {
            const char* rowStr = g->rows[row];
            for (int col = 0; col < 5; ++col) {
                if (rowStr[col] == '#') {
                    float pxX = cx + col * scale;
                    float pxY = y + (6 - row) * scale; // top-to-bottom -> bottom origin
                    drawRect(pxX, pxY, scale, scale, color, alpha);
                }
            }
        }
        cx += scale * 6.0f; // advance
    }
}

float Renderer::textWidth(const std::string& s, float px) const
{
    return (float)s.size() * (px * 6.0f);
}