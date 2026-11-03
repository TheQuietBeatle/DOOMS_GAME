#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include "Audio.h"

#include "Config.h"
#include "Game.h"
#include "Renderer.h"
#include "Sprites.h"
#include "TiltView.h"   // NEW

// ---------------------------------------------------------------------
// NEW: tilt view + background chooser
//   Background chooser (shown first): click a thumbnail or press 1-5
//   In game:  F1 tilt on/off   F2 next background   F3/F4 less/more tilt
//             F5 back to the background chooser
// ---------------------------------------------------------------------
static TiltView g_tilt;
static bool     g_picking = true;   // true while the background chooser is shown

// Only tilt during actual gameplay, not menus or the cutscene.
// If Game has something like isPlaying(), use it here:
//     return g_tilt.enabled && game.isPlaying();
static bool tiltNow(Game& /*game*/)
{
    return g_tilt.enabled;
}

// window pixels (y down) -> world pixels (y up)
static void toWorld(GLFWwindow* w, double mx, double my, float& wx, float& wy)
{
    int ww = 0, wh = 0;
    glfwGetWindowSize(w, &ww, &wh);
    if (ww == 0 || wh == 0) { wx = wy = -1.0f; return; }
    float scale = std::min(ww / cfg::WORLD_W, wh / cfg::WORLD_H);
    float offX = (ww - cfg::WORLD_W * scale) / 2.0f;
    float offY = (wh - cfg::WORLD_H * scale) / 2.0f;
    wx = (float)((mx - offX) / scale);
    wy = (float)(cfg::WORLD_H - (my - offY) / scale);
}

static Game* gameOf(GLFWwindow* w) { return static_cast<Game*>(glfwGetWindowUserPointer(w)); }

// NEW: same as toWorld, but undoes the tilt when the mouse is over the board
static void toGameWorld(GLFWwindow* w, double mx, double my, float& wx, float& wy)
{
    toWorld(w, mx, my, wx, wy);
    if (!tiltNow(*gameOf(w))) return;
    if (wy >= g_tilt.splitY) return;            // flat UI strip: unchanged

    float gx, gy;
    if (g_tilt.screenToBoard(wx, wy, gx, gy)) { wx = gx; wy = gy; }
    else { wx = wy = -1.0f; }   // background, not the board
}

static void chooseBackground(int i)
{
    if (i < 0 || i >= g_tilt.bgCount()) return;
    g_tilt.currentBg = i;
    g_picking = false;
}

static void cursorCallback(GLFWwindow* w, double mx, double my)
{
    float wx, wy;
    if (g_picking) {
        toWorld(w, mx, my, wx, wy);
        g_tilt.hoveredThumb = g_tilt.thumbAt(wx, wy);
        return;
    }
    toGameWorld(w, mx, my, wx, wy);
    gameOf(w)->onMouseMove(wx, wy);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    float wx, wy;

    if (g_picking) {
        toWorld(w, mx, my, wx, wy);
        if (button == GLFW_MOUSE_BUTTON_LEFT) chooseBackground(g_tilt.thumbAt(wx, wy));
        return;
    }
    toGameWorld(w, mx, my, wx, wy);
    gameOf(w)->onMouseButton(button, wx, wy);
}

static void keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;

    if (g_picking) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) chooseBackground(key - GLFW_KEY_1);
        if (key == GLFW_KEY_ENTER) chooseBackground(g_tilt.currentBg);
        return;
    }

    switch (key) {
    case GLFW_KEY_F1: g_tilt.enabled = !g_tilt.enabled; return;
    case GLFW_KEY_F2:
        if (g_tilt.bgCount() > 0) g_tilt.currentBg = (g_tilt.currentBg + 1) % g_tilt.bgCount();
        return;
    case GLFW_KEY_F3:
        g_tilt.tiltDeg = std::max(0.0f, g_tilt.tiltDeg - 2.0f);
        std::cout << "tilt = " << g_tilt.tiltDeg << " deg\n";
        return;
    case GLFW_KEY_F4:
        g_tilt.tiltDeg = std::min(60.0f, g_tilt.tiltDeg + 2.0f);
        std::cout << "tilt = " << g_tilt.tiltDeg << " deg\n";
        return;
    case GLFW_KEY_F5: g_picking = true; g_tilt.hoveredThumb = -1; return;
    default: break;
    }

    gameOf(w)->onKey(key);
}

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(cfg::WIN_W, cfg::WIN_H, "DoomsGame", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";

        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    Renderer renderer;
    if (!renderer.init()) return -1;
    buildSprites(renderer);   // every sprite frame -> one vertex buffer
    renderer.upload();

    // NEW: tilt view + backgrounds
    if (!g_tilt.init((int)cfg::WORLD_W, (int)cfg::WORLD_H, 2)) {
        std::cerr << "TiltView failed, running without tilt\n";
        g_tilt.enabled = false;
        g_picking = false;
    }
    const char* bgFiles[] = {
        "assets/bg1.png", "assets/bg2.png", "assets/bg3.png",
        "assets/bg4.png", "assets/bg5.png"
    };
    g_tilt.loadBackgrounds(bgFiles, 5);
    const char* bgNames[] = { "WAKANDA", "DEEP SPACE", "LATVERIA", "BATTLEGROUND", "BAXTER BUILDING" };
    g_tilt.setBackgroundNames(bgNames, 5);
    Game game;
    glfwSetWindowUserPointer(window, &game);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!Audio::init("assets/theme.mp3"))
        std::cerr << "Audio failed to start\n";   // game keeps running without music


    float last = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = std::min(now - last, 0.05f);   // avoid huge steps after a stall
        last = now;

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        if (g_picking) {
            // NEW: background chooser; the game waits until one is picked
            g_tilt.drawPicker(fbW, fbH, now);
        }
        else if (tiltNow(game)) {
            game.update(dt);
            // NEW: the game renders exactly as before, but into a texture...
            g_tilt.beginGamePass();
            renderer.beginFrame(g_tilt.fboWidth(), g_tilt.fboHeight());
            game.render(renderer);
            g_tilt.endGamePass();
            // ...which is then shown tilted over the background
            g_tilt.present(fbW, fbH);
        }
        else {
            // Original path, untouched
            game.update(dt);
            renderer.beginFrame(fbW, fbH);
            game.render(renderer);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_tilt.shutdown();   // NEW
    renderer.shutdown();
    Audio::shutdown();
    glfwTerminate();
    return 0;
}