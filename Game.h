// Game.h - Central game state: units, projectiles, collectibles, menus, waves.
// Game owns the update/render/input loop; main.cpp calls update+render every frame.
// State machine: MainMenu -> BgSelect -> Cutscene -> Playing -> Paused/Won/Lost.
#pragma once
#include "Renderer.h"
#include "Characters.h"
#include "Config.h"
#include "UI.h"
#include "Cutscene.h"   // intro cutscene (shot sequence before gameplay begins)
#include "Background.h" // background chooser (shown between Main Menu and Cutscene)

#include <random>
#include <vector>

// One character on the battlefield (player or enemy)
struct Unit {
    int   def = 0;            // index into g_chars
    int   lane = 0;
    float x = 0.0f;           // centre x in world px
    int   hp = 0;
    float attackTimer = 0.0f;
    float swingTime = 0.0f;   // > 0 while the strike frame is shown
    float animTime = 0.0f;
    float jumpY = 0.0f, jumpVel = 0.0f;
    float duckTimer = 0.0f;
    bool  fighting = false;   // touching an opponent -> stops moving
    bool  reachedBase = false; // enemy only: walked into the home base (no coin drop)
    bool  exited = false;      // player only: reached the lane end (not a death; no death sound)
};

struct Projectile {
    int   lane = 0;
    float x = 0.0f;
    float prevX = 0.0f;       // x last frame, for swept-segment hit test
    float yOff = 0.0f;        // height above the lane bottom
    float vx = 0.0f;
    bool  fromEnemy = false;
    bool  high = false;       // enemy only: true = duck to dodge, false = jump to dodge
    int   damage = 1;
    bool  alive = true;
};

struct Collectible {
    int   lane = 0;
    float x = 0.0f;
    float age = 0.0f;
    bool  alive = true;
};

// Floating "+100" text shown when a player reaches the end of its lane
struct RewardPopup {
    int   lane = 0;
    float x = 0.0f;
    int   amount = 0;
    float age = 0.0f;
};

// DMET502 spec requirement: distinct win/lose/pause/menu states.
// Settings, BgSelect, and Cutscene were added beyond the base spec.
enum class GameState { MainMenu, Settings, BgSelect, Cutscene, Playing, Paused, Won, Lost };

struct Explosion {
    int   lane = 0;
    float x = 0.0f;
    float y = 0.0f;   // absolute world y at impact
    float age = 0.0f;
    static constexpr float DURATION = 0.30f;
};

class Game {
public:
    Game();

    void reset();
    void update(float dt);
    void render(Renderer& r) const;

    // input (world coordinates, y up)
    void onMouseMove(float wx, float wy);
    void onMouseButton(int button, float wx, float wy);
    void onKey(int key);

    GameState state() const { return st; }
    bool wantsExit() const { return exitRequested; }

    // Background chooser flow: Main Menu -> PLAY -> BgSelect -> Cutscene -> Playing.
    // main.cpp owns the Background object and passes a pointer here.
    void setBackground(Background* b) { bg = b; }
    int  hoveredThumb() const { return st == GameState::BgSelect ? hoveredMenuBtn : -1; }

private:
    void spawnEnemy(int def, int lane);
    void placePlayer(int def, int lane);
    void returnToMainMenu();
    void chooseBackground(int i);   // picks background i, starts the cutscene

    void updatePlayers(float dt);
    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void updateExplosions(float dt);
    void cleanup();

    Unit* blockingEnemy(const Unit& player);
    Unit* blockingPlayer(const Unit& enemy);
    bool  isUnlocked(int card) const;
    int   laneAt(float wx, float wy) const;
    void  drawUnit(Renderer& r, const Unit& u, bool enemy, float alpha = 1.0f) const;
    void  drawMainMenu(Renderer& r) const;   // Main Menu scene (animated, drawing only)

    std::vector<Unit>        players;
    std::vector<Unit>        enemies;
    std::vector<Projectile>  projectiles;
    std::vector<Collectible> collectibles;
    std::vector<RewardPopup> popups;
    std::vector<Explosion>   explosions;

    int  coins = 0;
    int  armedCard = -1;      // card picked, waiting for a lane click
    int  hoveredCard = -1;
    int  hoveredLane = -1;
    int  hoveredMenuBtn = -1; // -1=none; tracks which menu button the mouse is over
    bool selectedLanes[cfg::NUM_LANES] = {};

    int   baseHp = 0;

    int   musicVolPct = 30;   // 0-100 in steps of 10; must match Audio's default volume (0.3)
    bool  musicMuted = false;

    float     clock = 0.0f;
    GameState st = GameState::Playing;
    std::mt19937 rng;
    CardBar   cards;
    Cutscene  cutscene;         // intro cutscene (drives Game.cpp update/render while Cutscene state)
    Background* bg = nullptr;  // owned by main.cpp; null if no images loaded
    float     introFade = 0.0f; // 1 = black overlay; fades to 0 after cutscene (lights-up effect)
    bool      exitRequested = false;

    void applyVolume();
    void changeVolume(int deltaPct);
    void toggleMute();
};
