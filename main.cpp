// main.cpp - DoomsGame entry point.
// Creates the GLFW window, initialises the renderer, audio, and background,
// and runs the frame loop (update -> draw background -> draw game -> present).

#include <glad/gl.h>      // GLAD loads the OpenGL 3.3 function pointers; must come before GLFW
#include <GLFW/glfw3.h>   // GLFW: window creation, OpenGL context, mouse/keyboard input

#include <algorithm>      // std::min (used for scaling and clamping dt)
#include <iostream>       // std::cerr for error messages
#include <iterator>       // std::size (number of elements in a C array, C++17)
#include "Audio.h"        // background music start/stop

#include "Config.h"       // cfg:: constants: window size and world (game) size
#include "Game.h"         // Game class: all game states, logic, and input handling
#include "Renderer.h"     // Renderer: shaders + vertex buffer used to draw sprites
#include "Sprites.h"      // buildSprites(): creates the geometry for every sprite frame
#include "Background.h"   // background images; replaces TiltView (tilt removed)

// Background image drawn behind the game board; F2 cycles to the next image.
static Background g_bg;

// GLFW reports the actual reason a call failed (bad driver, unsupported version, ...)
// GLFW calls this function itself whenever any GLFW call hits an error.
static void glfwErrorCallback(int code, const char* desc)
{
    std::cerr << "GLFW error " << code << ": " << desc << '\n';   // print code + readable reason
}

// window pixels (y down) -> world pixels (y up)
// This function undoes the letterbox and flips Y so clicks land on the right lane.
// Converts a GLFW cursor position (window pixels, y down) to world coordinates
// (world pixels, y up, 1280x720) accounting for the letterbox bars.
// Parameters wx, wy receive the world position; set to -1 if the window is minimised.
static void toWorld(GLFWwindow* w, double mouseX, double mouseY, float& worldX, float& worldY)
{
    int windowWidth = 0, windowHeight = 0;
    glfwGetWindowSize(w, &windowWidth, &windowHeight);    // current window size in screen pixels
    if (windowWidth == 0 || windowHeight == 0) { worldX = worldY = -1.0f; return; }  // minimised
    float scale = std::min(windowWidth / cfg::WORLD_W, windowHeight / cfg::WORLD_H);  // largest scale that still fits both axes
    float letterboxOffsetX = (windowWidth  - cfg::WORLD_W * scale) / 2.0f;   // width of one letterbox bar
    float letterboxOffsetY = (windowHeight - cfg::WORLD_H * scale) / 2.0f;   // height of one letterbox bar
    worldX = (float)((mouseX - letterboxOffsetX) / scale);                   // remove bar, convert to world units
    worldY = (float)(cfg::WORLD_H - (mouseY - letterboxOffsetY) / scale);    // flip so Y grows upward
}

// GLFW callbacks only receive the window, so the Game is stored inside the
// window's "user pointer" and fetched back here.
static Game* gameOf(GLFWwindow* w) { return static_cast<Game*>(glfwGetWindowUserPointer(w)); }

static void cursorCallback(GLFWwindow* w, double mouseX, double mouseY)
{
    Game* game = gameOf(w);
    if (!game) return;               // no game attached (e.g. during shutdown) -> ignore
    float worldX, worldY;
    toWorld(w, mouseX, mouseY, worldX, worldY);    // window pixels -> world coordinates
    game->onMouseMove(worldX, worldY);             // let the game decide what is being hovered
}

// Called by GLFW on mouse button press/release (lane select, cards, menu buttons).
static void mouseButtonCallback(GLFWwindow* w, int button, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;   // only react to the press, not the release
    Game* game = gameOf(w);
    if (!game) return;
    double mouseX, mouseY;
    glfwGetCursorPos(w, &mouseX, &mouseY);  // button events don't include position, so ask for it
    float worldX, worldY;
    toWorld(w, mouseX, mouseY, worldX, worldY);
    game->onMouseButton(button, worldX, worldY);   // which button + where it was clicked in the world
}

// Called by GLFW on keyboard events (W/S jump/duck, Esc pause, F2 background).
static void keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;                     // ignore key release and auto-repeat
    if (key == GLFW_KEY_F2) { g_bg.next(); return; }      // F2: cycle to the next background image
    if (Game* game = gameOf(w)) game->onKey(key);         // every other key goes to the game
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);   // register first so even glfwInit errors are printed
    if (!glfwInit()) return -1;                // start GLFW; nothing works without it

    // Ask for an OpenGL 3.3 core profile context (the project requires modern OpenGL 3.3+).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);   // core = no deprecated fixed-function calls
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);   // macOS refuses core contexts without this
#endif

    // Create the window and its OpenGL context (not fullscreen, no shared context).
    GLFWwindow* window = glfwCreateWindow(cfg::WIN_W, cfg::WIN_H, "DoomsGame", nullptr, nullptr);
    if (!window) {
        std::cerr << "Could not open the game window (OpenGL 3.3 core required)\n";
        glfwTerminate();                       // release GLFW before quitting
        return -1;
    }
    glfwMakeContextCurrent(window);   // all following OpenGL calls go to this window
    glfwSwapInterval(1);              // vsync: wait for the monitor refresh, avoids tearing

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Could not load OpenGL functions through GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Renderer renderer;
    if (!renderer.init()) {           // compiles/links shaders and creates the VAO/VBO
        std::cerr << "Renderer failed to initialise (shader compile/link?)\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    buildSprites(renderer);   // every sprite frame -> one vertex buffer
    renderer.upload();        // send all that vertex data to the GPU once, at startup

    // Background images (missing files are skipped). F2 switches between them at runtime.
    const char* bgFiles[] = {
        "assets/bg1.png", "assets/bg2.png", "assets/bg3.png",
        "assets/bg4.png", "assets/bg5.png"
    };
    const char* bgNames[] = { "WAKANDA", "DEEP SPACE", "LATVERIA", "BATTLEGROUND", "BAXTER BUILDING" };  // Display name for each image above, in the same order
    // Compile-time check: adding a file without a name (or vice versa) won't build.
    static_assert(std::size(bgFiles) == std::size(bgNames), "every background needs a name");

    // Load the images as textures; the game still runs if this fails.
    if (!g_bg.init(bgFiles, (int)std::size(bgFiles)))
        std::cerr << "Background failed to load, running without it\n";
    g_bg.current = 0;   // which image is highlighted first (0 = bg1.png ... 4 = bg5.png)
    g_bg.setNames(bgNames, (int)std::size(bgNames));   // shown under the thumbnails in the chooser

    Game game;                                  // owns menus, cutscene, lanes, players, enemies, waves
    glfwSetWindowUserPointer(window, &game);    // store it so the callbacks can find it (see gameOf)
    game.setBackground(&g_bg);   // the BgSelect screen after the Main Menu uses these images
    // Hook the input callbacks. Done after the user pointer is set so they never see a null game.
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!Audio::init("assets/theme.mp3"))
        std::cerr << "Audio failed to start\n";   // game keeps running without music

    // ---------------------------------------------------------------- frame loop
    float prevTime = (float)glfwGetTime();   // timestamp of the previous frame, in seconds
    // Keep running until the window's X is clicked or the game asks to exit (Exit button).
    while (!glfwWindowShouldClose(window) && !game.wantsExit()) {
        float currTime = (float)glfwGetTime();
        float deltaSeconds = std::min(currTime - prevTime, 0.05f);   // cap at 50 ms to avoid huge steps after a stall
        prevTime = currTime;

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);   // real pixel size (can differ from window on high-DPI)

        game.update(deltaSeconds);   // move characters, projectiles, spawn waves, check win/lose

        // minimised window: framebuffer is 0x0, skip drawing but keep the game ticking
        if (framebufferWidth == 0 || framebufferHeight == 0) {
            glfwPollEvents();   // still process events so the window can be restored
            continue;
        }

        // Draw order depends on the current screen: background first, then the game on top.
        // beginFrame(..., false) means "don't clear the screen" so the background stays visible.
        if (game.state() == GameState::BgSelect) {
            // background chooser: thumbnails first, then title/names/hints on top (clear=false)
            g_bg.drawPicker(framebufferWidth, framebufferHeight, game.hoveredThumb());
            renderer.beginFrame(framebufferWidth, framebufferHeight, false);
        }
        else if (game.state() == GameState::Cutscene) {
            // cutscene fills the screen; no background image, so let beginFrame clear to black
            renderer.beginFrame(framebufferWidth, framebufferHeight);
        }
        else if (game.state() == GameState::MainMenu) {
            // indices 4 and 2 = bg5 (Stark) and bg3 (Latveria)
            g_bg.drawMenuSplit(framebufferWidth, framebufferHeight, 4, 2);
            renderer.beginFrame(framebufferWidth, framebufferHeight, false);
        }
        else {
            // background image first, then the game drawn flat on top
            // (the board is semi-transparent, see BOARD_ALPHA in UI.cpp)
            g_bg.draw(framebufferWidth, framebufferHeight);
            renderer.beginFrame(framebufferWidth, framebufferHeight, false);
        }
        game.render(renderer);   // draw whatever the current state needs (menu, cutscene, board, UI)

        glfwSwapBuffers(window);   // show the finished frame (double buffering)
        glfwPollEvents();          // process input -> triggers the callbacks above
    }

    // ---------------------------------------------------------------- shutdown
    glfwSetWindowUserPointer(window, nullptr);   // callbacks must not touch game after this
    g_bg.shutdown();   //   free background textures
    renderer.shutdown();      // free shaders and GPU buffers (needs the context still alive)
    Audio::shutdown();        // stop music and release the audio device
    glfwDestroyWindow(window);   // destroy window + its OpenGL context
    glfwTerminate();             // shut GLFW down completely
    return 0;
}