#pragma once
#include "Renderer.h"
#include "Characters.h"
#include "Config.h"
#include "UI.h"
#include "Cutscene.h"   // [George] intro cutscene
#include "Background.h" // [George] background chooser

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
    bool  exited = false;      // [George] player only: reached the lane end (not a death)
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

enum class GameState { MainMenu, Settings, BgSelect, Cutscene, Playing, Paused, Won, Lost };   // [George] + Settings, BgSelect, Cutscene

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

    // [George] background chooser (Main Menu -> PLAY -> choose background -> cutscene -> game)
    void setBackground(Background* b) { bg = b; }
    int  hoveredThumb() const { return st == GameState::BgSelect ? hoveredMenuBtn : -1; }

private:
    void spawnEnemy(int def, int lane);
    void placePlayer(int def, int lane);
    void returnToMainMenu();
    void chooseBackground(int i);   // [George] pick a background, then play the cutscene

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
    void  drawMainMenu(Renderer& r) const;   // [George] Main Menu scene (drawing only)

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

    int   musicVolPct = 30;   // [George] 0-100 in steps of 10 (must match Audio's default 0.3)
    bool  musicMuted = false;

    float     clock = 0.0f;
    GameState st = GameState::Playing;
    std::mt19937 rng;
    CardBar   cards;
    Cutscene  cutscene;       // [George] intro cutscene
    Background* bg = nullptr; // [George] owned by main.cpp
    float     introFade = 0.0f; // [George] 1 = black, fades to 0 after the cutscene (lights come up)
    bool      exitRequested = false;

    void applyVolume();
    void changeVolume(int deltaPct);
    void toggleMute();
};
