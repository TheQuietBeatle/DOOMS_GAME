#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include "Audio.h"

#include "Config.h"
#include "Game.h"
#include "Renderer.h"
#include "Sprites.h"
#include "Background.h"   // [George] replaces TiltView (tilt removed)

// [George] background image behind the board (F2 = next background)
static Background g_bg;

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

static void cursorCallback(GLFWwindow* w, double mx, double my)
{
    float wx, wy;
    toWorld(w, mx, my, wx, wy);
    gameOf(w)->onMouseMove(wx, wy);
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    float wx, wy;
    toWorld(w, mx, my, wx, wy);
    gameOf(w)->onMouseButton(button, wx, wy);
}

static void keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_F2) { g_bg.next(); return; }   // [George] next background image
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

    // [George] background images (missing files are skipped). F2 switches between them.
    const char* bgFiles[] = {
        "assets/bg1.png", "assets/bg2.png", "assets/bg3.png",
        "assets/bg4.png", "assets/bg5.png"
    };
    if (!g_bg.init(bgFiles, 5))
        std::cerr << "Background failed to load, running without it\n";
    g_bg.current = 0;   // which image is highlighted first (0 = bg1.png ... 4 = bg5.png)
    const char* bgNames[] = { "WAKANDA", "DEEP SPACE", "LATVERIA", "BATTLEGROUND", "BAXTER BUILDING" };
    g_bg.setNames(bgNames, 5);   // shown under the thumbnails in the chooser

    Game game;
    glfwSetWindowUserPointer(window, &game);
    game.setBackground(&g_bg);   // [George] the chooser after the Main Menu uses these images
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!Audio::init("assets/theme.mp3"))
        std::cerr << "Audio failed to start\n";   // game keeps running without music


    float last = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window) && !game.wantsExit()) {
        float now = (float)glfwGetTime();
        float dt = std::min(now - last, 0.05f);   // avoid huge steps after a stall
        last = now;

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        game.update(dt);
        if (game.state() == GameState::BgSelect) {
            // [George] background chooser: thumbnails first, then title/names/hints on top
            g_bg.drawPicker(fbW, fbH, game.hoveredThumb());
            renderer.beginFrame(fbW, fbH, false);
        }
        else if (game.state() == GameState::Cutscene) {
            // [George] cutscene: full screen, no background image behind it
            renderer.beginFrame(fbW, fbH);
        }
        else if (game.state() == GameState::MainMenu) {
            // [George] Main Menu: split Stark (bg5 Baxter city) / Doom (bg3 Latveria) background
            g_bg.drawMenuSplit(fbW, fbH, 4, 2);
            renderer.beginFrame(fbW, fbH, false);
        }
        else {
            // [George] background image first, then the game drawn flat on top
            // (the board is semi-transparent, see BOARD_ALPHA in UI.cpp)
            g_bg.draw(fbW, fbH);
            renderer.beginFrame(fbW, fbH, false);
        }
        game.render(renderer);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_bg.shutdown();   // [George]
    renderer.shutdown();
    Audio::shutdown();
    glfwTerminate();
    return 0;
}