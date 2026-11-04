#include "Game.h"
#include "WaveSystem.h"
#include "Audio.h"
#include <GLFW/glfw3.h>   // key / mouse button codes

#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>

using namespace cfg;

// Reset the WaveSystem globals so the game can be restarted.
void resetWaves()
{
    ::enemies.clear();
    currentWave = 1;
    spawnedInCurrentWave = 0;
    lastSpawnTime = glfwGetTime();
    gameOver = playerWon = false;
}

namespace {

    constexpr float PLAYER_BOLT_SPEED = 420.0f;
    constexpr float ENEMY_BOLT_SPEED = 260.0f;
    constexpr float COIN_HOVER = 0.5f;   // seconds a dropped coin stays put
    constexpr float COIN_FLY = 0.5f;   // seconds to fly to the counter
    constexpr float POPUP_TIME = 1.0f; // seconds a "+coins" reward popup stays on screen

    // [George] sound effects (relative to the working directory, like assets/theme.mp3)
    const char* SFX_COIN = "assets/coin.mp3";     // coin picked up
    const char* SFX_DEATH = "assets/death.mp3";    // any character dies
    const char* VOICE_IRON = "assets/ironman.mp3";  // a Doom enemy dies / you win
    const char* VOICE_DOOM = "assets/doom.mp3";     // one of your heroes dies / you lose
    const char* SFX_REPULSOR = "assets/repulsor.mp3"; // ranged hero fires a rocket

    // Pause-menu buttons (world px, y up). Drawing and click-testing use the same rects.
    struct Button {
        float x, y, w, h;
        bool hit(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    };
    const Button BTN_VOL_DOWN{ 470.0f, 248.0f,  60.0f, 40.0f };
    const Button BTN_VOL_UP{ 750.0f, 248.0f,  60.0f, 40.0f };
    const Button BTN_MUTE{ 530.0f, 190.0f, 220.0f, 40.0f };

    // Main-menu
    // [George] re-laid out to match the menu reference (1600x900 scaled x0.8 to 1280x720, y flipped).
    // These rects are used for BOTH drawing and hit testing.
    const Button BTN_PLAY{ 450.0f, 316.0f, 380.0f, 64.0f };
    const Button BTN_SOUND{ 470.0f, 246.0f, 340.0f, 48.0f };   // [George] opens Sound Settings
    const Button BTN_EXIT{ 470.0f, 176.0f, 340.0f, 48.0f };

    // [George] Sound Settings screen (also uses BTN_VOL_DOWN / BTN_VOL_UP / BTN_MUTE)
    const Button BTN_BACK{ 440.0f, 100.0f, 400.0f, 50.0f };

    // Pause-menu action buttons (sit above the volume section)
    const Button BTN_CONTINUE{ 440.0f, 435.0f, 400.0f, 50.0f };
    const Button BTN_RESTART_P{ 440.0f, 370.0f, 400.0f, 50.0f };
    const Button BTN_MAINMENU_P{ 440.0f, 305.0f, 400.0f, 50.0f };

    // Win/Lose action buttons
    const Button BTN_RESTART_E{ 440.0f, 355.0f, 400.0f, 50.0f };
    const Button BTN_MAINMENU_E{ 440.0f, 290.0f, 400.0f, 50.0f };

    void drawButton(Renderer& r, const Button& b, const std::string& label, bool hovered = false)
    {
        glm::vec3 border = hovered ? glm::vec3{ 0.35f, 0.52f, 0.70f } : glm::vec3{ 0.20f, 0.30f, 0.42f };
        r.drawRect(b.x, b.y, b.w, b.h, border);
        r.drawRect(b.x + 3, b.y + 3, b.w - 6, b.h - 6, { 0.08f, 0.12f, 0.20f });
        float tx = b.x + (b.w - r.textWidth(label, 4.0f)) * 0.5f;
        float ty = b.y + (b.h - 7.0f * 4.0f) * 0.5f;
        r.drawText(label, tx, ty, 4.0f, { 0.85f, 0.92f, 1.00f });
    }

    // [George] Main Menu button: PLAY is the bright-blue primary, the others dark navy.
    // Hovered = brighter fill + cyan glow border.
    void drawMenuButton(Renderer& r, const Button& b, const std::string& label, bool hovered,
                        bool primary, float px)
    {
        if (hovered) {   // soft glow: 3 rings fading outward
            for (int i = 3; i >= 1; --i) {
                float g = 3.0f * i;
                r.drawRect(b.x - g, b.y - g, b.w + 2 * g, b.h + 2 * g, { 0.35f, 0.80f, 1.00f }, 0.10f + 0.06f * (3 - i));
            }
        }
        glm::vec3 rim    = { 0.02f, 0.04f, 0.10f };
        glm::vec3 border = hovered ? glm::vec3{ 0.75f, 0.95f, 1.00f } : glm::vec3{ 0.55f, 0.66f, 0.85f };
        glm::vec3 fillLo = primary ? glm::vec3{ 0.12f, 0.30f, 0.62f } : glm::vec3{ 0.06f, 0.10f, 0.22f };
        glm::vec3 fillHi = primary ? glm::vec3{ 0.25f, 0.50f, 0.85f } : glm::vec3{ 0.12f, 0.18f, 0.34f };
        if (hovered) { fillLo *= 1.35f; fillHi = glm::min(fillHi * 1.35f, glm::vec3(1.0f)); }

        r.drawRect(b.x, b.y, b.w, b.h, rim);                                     // dark outer rim
        r.drawRect(b.x + 2, b.y + 2, b.w - 4, b.h - 4, border);                  // light border
        r.drawRect(b.x + 5, b.y + 5, b.w - 10, b.h - 10, fillLo);                // lower fill
        r.drawRect(b.x + 5, b.y + b.h * 0.5f, b.w - 10, b.h * 0.5f - 5, fillHi); // upper (bevel) fill
        r.drawRect(b.x + 8, b.y + b.h - 9, b.w - 16, 2, { 1.0f, 1.0f, 1.0f }, primary ? 0.35f : 0.15f); // shine

        float tw = r.textWidth(label, px) - px;   // drop the trailing 1-px gap so it centres
        float tx = b.x + (b.w - tw) * 0.5f;
        float ty = b.y + (b.h - 7.0f * px) * 0.5f;
        r.drawText(label, tx + 2, ty - 2, px, { 0.01f, 0.02f, 0.06f }, 0.8f);  // shadow
        r.drawText(label, tx, ty, px, hovered ? glm::vec3{ 1.0f, 1.0f, 1.0f } : glm::vec3{ 0.90f, 0.95f, 1.00f });
    }

    // [George] cheap deterministic 0..1 noise for menu particles / lightning
    float hash01(float n) { float s = std::sin(n * 12.9898f) * 43758.5453f; return s - std::floor(s); }

    // [George] thick line drawn as a run of small squares (Renderer only has axis-aligned rects)
    void drawLine(Renderer& r, float x0, float y0, float x1, float y1, float thick, glm::vec3 c, float a)
    {
        float dx = x1 - x0, dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        int steps = std::max(1, (int)(len / 2.0f));
        for (int i = 0; i <= steps; ++i) {
            float k = (float)i / steps;
            r.drawRect(x0 + dx * k - thick * 0.5f, y0 + dy * k - thick * 0.5f, thick, thick, c, a);
        }
    }

    // [George] draw a unit sprite standing at (cx, bottom) with an on-screen height of hPx.
    // Same centring maths as Game::drawUnit (drawSprite scales pixel-unit vertices twice).
    void drawMenuSprite(Renderer& r, SpriteId id, float cx, float bottom, float hPx, bool flip)
    {
        const SpriteFrame& f = g_sprites[id];
        if (f.h <= 0.0f) return;
        float s = hPx / (f.h * f.h);
        float wScale = f.w * s;
        float wReal = wScale * f.w;
        float x = flip ? cx + wReal * 0.5f - wScale * 0.5f : cx - wReal * 0.5f + wScale * 0.5f;
        r.drawSprite(f, x, bottom, s, s, flip);
    }

    // a = player, b = enemy.  signed gap prevents units from tunnelling through each other.
    bool touching(const Unit& a, const Unit& b)
    {
        if (a.lane != b.lane) return false;
        float gap = b.x - a.x;   // positive when enemy is to the right (normal case)
        float contact = unitHalfWidth(a.def) + unitHalfWidth(b.def);
        return std::abs(gap) <= contact; // [George] was: gap <= contact && gap > -contact (asymmetric — missed enemy-left-of-player case)
    }

} // namespace

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
Game::Game() : rng(std::random_device{}())
{
    srand(static_cast<unsigned int>(time(nullptr)));
    reset();
    st = GameState::MainMenu;   // show main menu before first play
}

void Game::reset()
{
    players.clear();
    enemies.clear();
    projectiles.clear();
    collectibles.clear();
    popups.clear();
    explosions.clear();

    coins = START_COINS;
    armedCard = hoveredCard = hoveredLane = -1;
    hoveredMenuBtn = -1;
    for (bool& s : selectedLanes) s = false;

    baseHp = BASE_MAX_HP;
    resetWaves();
    introFade = 0.0f;   // [George]

    clock = 0.0f;
    st = GameState::Playing;
}

void Game::returnToMainMenu()
{
    reset();
    st = GameState::MainMenu;
}

// [George] PLAY opens the background chooser; picking one starts the cutscene.
// With no background images loaded, PLAY goes straight to the cutscene.
void Game::chooseBackground(int i)
{
    if (bg && i >= 0 && i < bg->count()) bg->current = i;
    hoveredMenuBtn = -1;
    cutscene.start();
    st = GameState::Cutscene;
}

void Game::spawnEnemy(int def, int lane)
{
    Unit e;
    e.def = def;
    e.lane = lane;
    e.x = GRID_RIGHT - unitHalfWidth(def);
    e.hp = g_chars[def].maxHp;
    e.attackTimer = 0.5f;   // starts attacking right after it appears
    enemies.push_back(e);
}

void Game::placePlayer(int def, int lane)
{
    const CharacterDef& d = g_chars[def];
    if (coins < d.cost) return;
    coins -= d.cost;

    Unit p;
    p.def = def;
    p.lane = lane;
    p.x = GRID_LEFT + unitHalfWidth(def);   // leftmost position of the lane
    p.hp = d.maxHp;
    p.attackTimer = 0.2f;
    players.push_back(p);
}

bool Game::isUnlocked(int card) const
{
    return card >= 0 && card < NUM_HERO_TYPES && coins >= g_chars[card].unlockCost;
}

int Game::laneAt(float wx, float wy) const
{
    float gridBottom = laneBottom(NUM_LANES - 1);
    if (wx < GRID_LEFT || wx > GRID_RIGHT || wy < gridBottom || wy >= GRID_TOP) return -1;
    return std::min(std::max((int)((GRID_TOP - wy) / LANE_H), 0), NUM_LANES - 1);
}


// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
void Game::update(float dt)
{
    // [George] intro cutscene plays first; gameplay starts when it ends (or is skipped).
    // reset() runs HERE, not when PLAY is clicked, so the wave timer (glfwGetTime)
    // starts counting after the cutscene instead of during it.
    if (st == GameState::Cutscene) {
        cutscene.update(dt);
        if (cutscene.finished()) {
            reset();             // reset() also sets st = Playing
            introFade = 1.0f;    // [George] start fully dark, then the lights come up
        }
        return;
    }

    if (st != GameState::Playing) return;   // paused / won / lost: everything freezes
    clock += dt;
    if (introFade > 0.0f) introFade = std::max(0.0f, introFade - dt / 1.2f);   // [George] ~1.2 s fade-in

    // a card re-locks when coins drop below its unlockCost, or becomes unaffordable
    if (armedCard >= 0 && (!isUnlocked(armedCard) || coins < g_chars[armedCard].cost))
        armedCard = -1;

    // Wave system drives enemy spawning (assign lane randomly with our rng)
    {
        std::uniform_int_distribution<int> laneDist(0, NUM_LANES - 1);
        size_t prevCount = ::enemies.size();
        updateWaveSystem(glfwGetTime());
        for (size_t i = prevCount; i < ::enemies.size(); ++i) {
            const auto& we = ::enemies[i];
            int def = (we.type == EnemyType::Easy) ? CH_EASY :
                (we.type == EnemyType::Medium) ? CH_MEDIUM : CH_HARD;
            spawnEnemy(def, laneDist(rng));
        }
    }

    updatePlayers(dt);
    updateEnemies(dt);
    updateProjectiles(dt);
    updateExplosions(dt);
    cleanup();

    if (baseHp <= 0) { st = GameState::Lost; Audio::playVoice(VOICE_DOOM, true); return; }   // [George]
    if (::playerWon) { st = GameState::Won;  Audio::playVoice(VOICE_IRON, true); }           // [George]
}


Unit* Game::blockingEnemy(const Unit& p)
{
    Unit* best = nullptr;
    for (Unit& e : enemies)
        if (e.hp > 0 && touching(p, e) && (!best || e.x < best->x)) best = &e;
    return best;
}

Unit* Game::blockingPlayer(const Unit& e)
{
    Unit* best = nullptr;   // the front-most player it touches
    for (Unit& p : players)
        if (p.hp > 0 && touching(p, e) && (!best || p.x > best->x)) best = &p;
    return best;
}

void Game::updatePlayers(float dt)
{
    for (Unit& p : players) {
        const CharacterDef& d = g_chars[p.def];
        p.animTime += dt;

        // jump / duck (defense players ignore both)
        if (d.role != Role::Defense) {
            if (p.jumpY > 0.0f || p.jumpVel > 0.0f) {
                p.jumpY += p.jumpVel * dt;
                p.jumpVel -= GRAVITY * dt;
                if (p.jumpY <= 0.0f) { p.jumpY = 0.0f; p.jumpVel = 0.0f; }
            }
            if (p.duckTimer > 0.0f) p.duckTimer -= dt;
        }

        // movement: stop while touching an enemy; clamp to prevent crossing
        Unit* target = blockingEnemy(p);
        p.fighting = (target != nullptr);
        if (d.speed > 0.0f && !p.fighting) {
            p.x += d.speed * dt;
            p.x = std::min(p.x, GRID_RIGHT - unitHalfWidth(p.def));
        }
        // hard clamp: never let a player overlap an enemy regardless of frame rate
        for (Unit& e : enemies) {
            if (e.lane == p.lane && e.hp > 0) {
                float contact = unitHalfWidth(p.def) + unitHalfWidth(e.def);
                if (e.x > p.x && e.x - p.x < contact) p.x = e.x - contact; // [George] guard: only push player left when enemy is to the right
            }
        }

        // reached the end of the lane: refund 2x the card price and leave the field
        // (defense players never move, so they never get here)
        if (d.role != Role::Defense && d.speed > 0.0f &&
            p.x >= GRID_RIGHT - unitHalfWidth(p.def) - 0.01f) {
            int reward = 2 * d.cost;
            coins += reward;

            RewardPopup pop;
            pop.lane = p.lane;
            pop.x = p.x;
            pop.amount = reward;
            popups.push_back(pop);

            p.hp = 0;   // removed in cleanup(); players never drop collectibles
            p.exited = true;   // [George] left the field, not killed -> no death sound
            continue;
        }

        // attacking starts as soon as the player is placed
        if (p.swingTime > 0.0f) p.swingTime -= dt;
        p.attackTimer -= dt;
        if (d.damage > 0 && p.attackTimer <= 0.0f) {
            p.attackTimer = d.attackCooldown;
            if (d.role == Role::Melee) {
                p.swingTime = 0.18f;
                if (target) target->hp -= d.damage;
            }
            else if (d.role == Role::Ranged) {
                Projectile pr;
                pr.lane = p.lane;
                pr.x = p.x + unitHalfWidth(p.def);
                pr.yOff = GROUND_PAD + p.jumpY + unitHeight(p.def) * 0.80f;   // barrel height
                pr.vx = PLAYER_BOLT_SPEED;
                pr.damage = d.damage;
                projectiles.push_back(pr);
                Audio::playSfx(SFX_REPULSOR);   // [George] ranged shot sound
            }
        }

        // pick up collectibles by walking over them
        if (d.role != Role::Defense) {
            for (Collectible& c : collectibles) {
                if (c.alive && c.lane == p.lane && std::fabs(c.x - p.x) < unitHalfWidth(p.def)) {
                    c.alive = false;
                    coins += COIN_DROP;
                    Audio::playSfx(SFX_COIN);   // [George]
                }
            }
        }
    }

    // age drives the bob animation; coins stay until a player walks over them
    for (Collectible& c : collectibles)
        c.age += dt;

    for (RewardPopup& pop : popups) pop.age += dt;
}



void Game::updateEnemies(float dt)
{
    for (Unit& e : enemies) {
        const CharacterDef& d = g_chars[e.def];
        e.animTime += dt;

        Unit* target = blockingPlayer(e);
        e.fighting = (target != nullptr);
        if (!e.fighting) {
            e.x -= d.speed * dt;
            if (e.x - unitHalfWidth(e.def) <= GRID_LEFT) {   // reached the home base
                --baseHp;
                e.hp = 0;              // mark for removal; cleanup() will do the rest
                e.reachedBase = true;  // ...but it was not defeated, so no coin drop
                continue;
            }
        }
        // hard clamp: never let an enemy cross a player
        for (Unit& p : players) {
            if (p.lane == e.lane && p.hp > 0) {
                float contact = unitHalfWidth(e.def) + unitHalfWidth(p.def);
                if (e.x - p.x < contact) e.x = p.x + contact;
            }
        }

        if (e.swingTime > 0.0f) e.swingTime -= dt;
        e.attackTimer -= dt;
        if (e.attackTimer > 0.0f) continue;
        e.attackTimer = d.attackCooldown;

        if (d.role == Role::EnemyMelee) {
            e.swingTime = 0.2f;
            if (target) target->hp -= d.damage;
        }
        else {
            Projectile pr;
            pr.lane = e.lane;
            pr.x = e.x - unitHalfWidth(e.def);
            pr.vx = -ENEMY_BOLT_SPEED;
            pr.fromEnemy = true;
            pr.high = (d.role == Role::EnemyUpper);
            pr.yOff = pr.high ? GROUND_PAD + 70.0f : GROUND_PAD + 2.0f;
            pr.damage = d.damage;
            projectiles.push_back(pr);
        }
    }
}

void Game::updateProjectiles(float dt)
{
    for (Projectile& pr : projectiles) {
        if (!pr.alive) continue;
        pr.prevX = pr.x;
        pr.x += pr.vx * dt;

        // swept-segment helpers: min/max x the bolt passed through this frame
        float segMin = std::min(pr.prevX, pr.x);
        float segMax = std::max(pr.prevX, pr.x);

        if (!pr.fromEnemy) {
            for (Unit& e : enemies) {
                if (e.lane != pr.lane || e.hp <= 0) continue;
                float bMin = e.x - unitHalfWidth(e.def);
                float bMax = e.x + unitHalfWidth(e.def);
                if (segMax >= bMin && segMin <= bMax) {
                    e.hp -= pr.damage;
                    pr.alive = false;
                    // spawn explosion at impact point (no extra damage)
                    Explosion ex;
                    ex.lane = pr.lane;
                    ex.x = bMin;
                    ex.y = laneBottom(pr.lane) + pr.yOff;
                    explosions.push_back(ex);
                    break;
                }
            }
            if (pr.x > WORLD_W + 60.0f) pr.alive = false;
            continue;
        }

        // enemy projectile: defense player soaks it first
        Unit* hit = nullptr;
        for (Unit& p : players) {
            if (p.lane != pr.lane || p.hp <= 0 || g_chars[p.def].role != Role::Defense) continue;
            float bMin = p.x - unitHalfWidth(p.def);
            float bMax = p.x + unitHalfWidth(p.def);
            if (segMax >= bMin && segMin <= bMax) { hit = &p; break; }
        }
        if (!hit) {
            for (Unit& p : players) {
                if (p.lane != pr.lane || p.hp <= 0) continue;
                float bMin = p.x - unitHalfWidth(p.def);
                float bMax = p.x + unitHalfWidth(p.def);
                if (segMax < bMin || segMin > bMax) continue;
                bool dodged = pr.high ? (p.duckTimer > 0.0f) : (p.jumpY > 12.0f);
                if (dodged) continue;
                hit = &p;
                break;
            }
        }
        if (hit) {
            hit->hp -= pr.damage;
            pr.alive = false;
        }
        if (pr.x < -60.0f) pr.alive = false;
    }
}

void Game::updateExplosions(float dt)
{
    for (Explosion& ex : explosions) ex.age += dt;
}

void Game::cleanup()
{
    // defeated enemies drop a collectible where they fell
    // (enemies that walked into the base are removed without a drop)
    for (const Unit& e : enemies)
        if (e.hp <= 0 && !e.reachedBase) {
            Collectible c;
            c.lane = e.lane;
            c.x = e.x;
            collectibles.push_back(c);
        }

    // Notify wave system of each defeated enemy (match by type only — lanes differ)
    for (const Unit& e : enemies) {
        if (e.hp <= 0) {
            EnemyType et = (e.def == CH_EASY) ? EnemyType::Easy :
                (e.def == CH_MEDIUM) ? EnemyType::Medium :
                EnemyType::Hard;
            for (auto& we : ::enemies) {
                if (we.active && we.type == et) {
                    we.active = false;
                    break;
                }
            }
        }
    }
    removeDefeatedEnemies();

    // [George] death sounds: Iron Man line when a Doom enemy is defeated,
    // Doom line when one of your heroes is killed (voice lines never stack)
    for (const Unit& e : enemies)
        if (e.hp <= 0 && !e.reachedBase) { Audio::playSfx(SFX_DEATH); Audio::playVoice(VOICE_IRON); }
    for (const Unit& p : players)
        if (p.hp <= 0 && !p.exited) { Audio::playSfx(SFX_DEATH); Audio::playVoice(VOICE_DOOM); }

    auto dead = [](const Unit& u) { return u.hp <= 0; };
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), dead), enemies.end());
    players.erase(std::remove_if(players.begin(), players.end(), dead), players.end());
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p) { return !p.alive; }), projectiles.end());
    collectibles.erase(std::remove_if(collectibles.begin(), collectibles.end(),
        [](const Collectible& c) { return !c.alive; }), collectibles.end());
    popups.erase(std::remove_if(popups.begin(), popups.end(),
        [](const RewardPopup& p) { return p.age >= POPUP_TIME; }), popups.end());
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
        [](const Explosion& ex) { return ex.age >= Explosion::DURATION; }), explosions.end());
}



// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void Game::onMouseMove(float wx, float wy)
{
    hoveredLane = laneAt(wx, wy);
    int c = cards.hitTest(wx, wy);
    hoveredCard = (isUnlocked(c) && coins >= g_chars[c].cost) ? c : -1;

    hoveredMenuBtn = -1;
    if (st == GameState::MainMenu) {
        if (BTN_PLAY.hit(wx, wy))       hoveredMenuBtn = 0;
        else if (BTN_SOUND.hit(wx, wy)) hoveredMenuBtn = 1;   // [George]
        else if (BTN_EXIT.hit(wx, wy))  hoveredMenuBtn = 2;
    }
    else if (st == GameState::BgSelect) {                    // [George] hovered thumbnail
        if (bg) hoveredMenuBtn = bg->thumbAt(wx, wy);
    }
    else if (st == GameState::Settings) {                    // [George]
        if (BTN_BACK.hit(wx, wy)) hoveredMenuBtn = 0;
    }
    else if (st == GameState::Paused) {
        if (BTN_CONTINUE.hit(wx, wy))   hoveredMenuBtn = 0;
        else if (BTN_RESTART_P.hit(wx, wy))  hoveredMenuBtn = 1;
        else if (BTN_MAINMENU_P.hit(wx, wy)) hoveredMenuBtn = 2;
    }
    else if (st == GameState::Won || st == GameState::Lost) {
        if (BTN_RESTART_E.hit(wx, wy))  hoveredMenuBtn = 0;
        else if (BTN_MAINMENU_E.hit(wx, wy)) hoveredMenuBtn = 1;
    }
}

void Game::onMouseButton(int button, float wx, float wy)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        // right-click cancels the armed card during play
        if (button == GLFW_MOUSE_BUTTON_RIGHT && st == GameState::Playing)
            armedCard = -1;
        return;
    }

    // [George] clicks are ignored during the cutscene (no accidental card / lane picks)
    if (st == GameState::Cutscene) return;

    // --- Main menu ---
    if (st == GameState::MainMenu) {
        if (BTN_PLAY.hit(wx, wy)) {   // [George] was: st = GameState::Playing
            if (bg && bg->count() > 0) { st = GameState::BgSelect; hoveredMenuBtn = -1; }
            else chooseBackground(-1);
            return;
        }
        if (BTN_SOUND.hit(wx, wy)) { st = GameState::Settings; hoveredMenuBtn = -1; return; }   // [George]
        if (BTN_EXIT.hit(wx, wy)) { exitRequested = true; return; }
        return;
    }

    // --- [George] Background chooser (after the Main Menu) ---
    if (st == GameState::BgSelect) {
        if (bg) {
            int i = bg->thumbAt(wx, wy);
            if (i >= 0) chooseBackground(i);
        }
        return;
    }

    // --- [George] Sound Settings (from the Main Menu) ---
    if (st == GameState::Settings) {
        if (BTN_VOL_DOWN.hit(wx, wy)) { changeVolume(-10); return; }
        if (BTN_VOL_UP.hit(wx, wy)) { changeVolume(+10); return; }
        if (BTN_MUTE.hit(wx, wy)) { toggleMute();      return; }
        if (BTN_BACK.hit(wx, wy)) { st = GameState::MainMenu; hoveredMenuBtn = -1; return; }
        return;
    }

    // --- Pause menu ---
    if (st == GameState::Paused) {
        if (BTN_VOL_DOWN.hit(wx, wy)) { changeVolume(-10);       return; }
        if (BTN_VOL_UP.hit(wx, wy)) { changeVolume(+10);       return; }
        if (BTN_MUTE.hit(wx, wy)) { toggleMute();            return; }
        if (BTN_CONTINUE.hit(wx, wy)) { st = GameState::Playing; return; }
        if (BTN_RESTART_P.hit(wx, wy)) { reset();                 return; }
        if (BTN_MAINMENU_P.hit(wx, wy)) { returnToMainMenu();      return; }
        return;
    }

    // --- Win / Lose screens ---
    if (st == GameState::Won || st == GameState::Lost) {
        if (BTN_RESTART_E.hit(wx, wy)) { reset();            return; }
        if (BTN_MAINMENU_E.hit(wx, wy)) { returnToMainMenu(); return; }
        return;
    }

    // --- Playing ---
    int card = cards.hitTest(wx, wy);
    if (card >= 0) {
        if (isUnlocked(card) && coins >= g_chars[card].cost) armedCard = (armedCard == card) ? -1 : card;
        return;
    }

    int lane = laneAt(wx, wy);
    if (lane < 0) return;
    if (armedCard >= 0) {
        placePlayer(armedCard, lane);
        armedCard = -1;
    }
    else {
        selectedLanes[lane] = !selectedLanes[lane];
    }
}

void Game::onKey(int key)
{
    // [George] Space / Enter skips the cutscene; other keys (incl. ESC) do nothing during it
    if (st == GameState::Cutscene) {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER) cutscene.skip();
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        if (st == GameState::Settings || st == GameState::BgSelect) { st = GameState::MainMenu; hoveredMenuBtn = -1; }   // [George]
        else if (st == GameState::Playing) st = GameState::Paused;
        else if (st == GameState::Paused) st = GameState::Playing;
        return;
    }
    // Enter/Space starts the game from the main menu
    if (st == GameState::MainMenu &&
        (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)) {
        // [George] was: st = GameState::Playing;  now opens the background chooser
        if (bg && bg->count() > 0) { st = GameState::BgSelect; hoveredMenuBtn = -1; }
        else chooseBackground(-1);
        return;
    }
    // [George] background chooser: 1-5 picks one, Enter/Space keeps the highlighted one
    if (st == GameState::BgSelect) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && bg && key - GLFW_KEY_1 < bg->count())
            chooseBackground(key - GLFW_KEY_1);
        else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
            chooseBackground(bg ? bg->current : -1);
        else if (bg && key == GLFW_KEY_RIGHT && bg->count() > 0) bg->current = (bg->current + 1) % bg->count();
        else if (bg && key == GLFW_KEY_LEFT  && bg->count() > 0) bg->current = (bg->current + bg->count() - 1) % bg->count();
        return;
    }
    // R restarts from pause / win / lose (not from main menu)
    if (key == GLFW_KEY_R &&
        (st == GameState::Paused || st == GameState::Won || st == GameState::Lost)) {
        reset();
        return;
    }
    // Pause-menu / Sound Settings audio controls
    if (st == GameState::Paused || st == GameState::Settings) {   // [George] + Settings
        if (key == GLFW_KEY_LEFT)  changeVolume(-10);
        else if (key == GLFW_KEY_RIGHT) changeVolume(+10);
        else if (key == GLFW_KEY_M)     toggleMute();
        return;
    }
    if (st != GameState::Playing) return;

    bool jump = (key == GLFW_KEY_W || key == GLFW_KEY_UP);
    bool duck = (key == GLFW_KEY_S || key == GLFW_KEY_DOWN);
    if (!jump && !duck) return;

    for (Unit& p : players) {
        if (!selectedLanes[p.lane] || g_chars[p.def].role == Role::Defense) continue;
        if (jump && p.jumpY <= 0.0f) p.jumpVel = JUMP_SPEED;
        if (duck) p.duckTimer = DUCK_TIME;
    }
}



// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void Game::drawUnit(Renderer& r, const Unit& u, bool enemy, float alpha) const
{
    const CharacterDef& d = g_chars[u.def];

    // pick the animation frame
    SpriteId id = d.frameA;
    bool walking = d.speed > 0.0f && !u.fighting;
    if (d.role == Role::Melee)
        id = (u.swingTime > 0.0f) ? d.frameB : d.frameA;            // B = arm shoots out on a punch
    else if (d.role == Role::EnemyMelee && u.fighting)
        id = (u.swingTime > 0.0f) ? d.frameA : d.frameB;            // weapon strike          
    else if (walking)
        id = ((int)(u.animTime * 5.0f) % 2 == 0) ? d.frameA : d.frameB;   // walk cycle

    const SpriteFrame& f = g_sprites[id];
    float s = unitScale(u.def);
    float sy = (u.duckTimer > 0.0f) ? s * 0.6f : s;
    float y = laneBottom(u.lane) + GROUND_PAD + u.jumpY;

    DrawFx fx;
    fx.red = (1.0f - std::max(0, u.hp) / (float)d.maxHp) * 0.75f;   // redder as health drops
    fx.alpha = alpha;
    // [George] centre the sprite on u.x (the unit's hitbox centre). Renderer::drawSprite
    // puts the sprite's left edge (or right edge when flipped) near x, so players drew
    // to the right of their position and enemies to the left: when they met they ended
    // up crossed, fighting back-to-back.
    float wScale = f.w * s;                 // what drawSprite calls w
    float wReal  = wScale * f.w;            // actual on-screen width
    float drawX  = enemy ? u.x + wReal * 0.5f - wScale * 0.5f
                         : u.x - wReal * 0.5f + wScale * 0.5f;
    r.drawSprite(f, drawX, y, s, sy, enemy, fx);

    if (alpha >= 1.0f)
        drawHealthBar(r, u.x, y + f.h * sy + 6.0f, 56.0f, u.hp, d.maxHp, enemy);
}

void Game::render(Renderer& r) const
{
    if (st == GameState::Cutscene) { cutscene.draw(r); return; }   // [George] intro cutscene

    // [George] background chooser: the thumbnails are drawn by Background (main.cpp);
    // here we only add the title, the names and the hints on top.
    if (st == GameState::BgSelect) {
        drawCenteredText(r, "CHOOSE YOUR BATTLEGROUND", 560, 6.0f, { 0.85f, 0.35f, 0.15f });
        if (bg) {
            for (int i = 0; i < bg->count(); ++i) {
                float x, y, w, h;
                bg->thumbRect(i, x, y, w, h);
                std::string nm = bg->name(i);
                if (nm.empty()) nm = "BACKGROUND " + std::to_string(i + 1);
                std::string label = std::to_string(i + 1) + "  " + nm;
                float px = 2.5f;
                while (px > 1.0f && r.textWidth(label, px) > w + 20.0f) px -= 0.5f;
                glm::vec3 col = (i == bg->current) ? glm::vec3{ 0.45f, 0.90f, 1.00f }
                              : (i == hoveredMenuBtn) ? glm::vec3{ 1.0f, 1.0f, 1.0f }
                                                      : glm::vec3{ 0.70f, 0.78f, 0.88f };
                r.drawText(label, x + (w - r.textWidth(label, px)) * 0.5f, y - 30.0f, px, col);
            }
        }
        drawCenteredText(r, "CLICK A BACKGROUND OR PRESS 1-5", 200, 3.0f, { 0.75f, 0.85f, 0.95f });
        drawCenteredText(r, "LEFT/RIGHT CHANGE   ENTER CONTINUE   ESC BACK", 160, 2.0f, { 0.55f, 0.65f, 0.75f });
        return;
    }

    if (st == GameState::MainMenu) { drawMainMenu(r); return; }   // [George] full-scene main menu

    drawBattlefield(r, st == GameState::Playing ? hoveredLane : -1, selectedLanes, clock);

    // ghost preview of where the picked card will appear
    if (st == GameState::Playing && armedCard >= 0 && hoveredLane >= 0) {
        Unit ghost;
        ghost.def = armedCard;
        ghost.lane = hoveredLane;
        ghost.x = GRID_LEFT + unitHalfWidth(armedCard);
        ghost.hp = g_chars[armedCard].maxHp;
        drawUnit(r, ghost, false, 0.45f);
    }

    for (const Collectible& c : collectibles) {
        float x = c.x;
        float y = laneBottom(c.lane) + GROUND_PAD + 6.0f + std::sin(c.age * 8.0f) * 3.0f;
        float fcs = 1.5f / g_sprites[SPR_COIN].h;
        r.drawSprite(g_sprites[SPR_COIN], x, y, fcs, fcs, false);
    }

    for (const Unit& p : players) drawUnit(r, p, false);
    for (const Unit& e : enemies) drawUnit(r, e, true);

    // "+100" popups at the lane end: rise and fade out
    for (const RewardPopup& pop : popups) {
        float t = std::min(std::max(pop.age / POPUP_TIME, 0.0f), 1.0f);
        std::string txt = "+" + std::to_string(pop.amount);
        float tx = std::min(pop.x, GRID_RIGHT - r.textWidth(txt, 3.0f)) - r.textWidth(txt, 3.0f) * 0.5f;
        float ty = laneBottom(pop.lane) + LANE_H * 0.55f + t * 30.0f;
        r.drawText(txt, tx, ty, 3.0f, { 1.00f, 0.85f, 0.25f }, 1.0f - t);
    }

    for (const Projectile& p : projectiles) {
        float y = laneBottom(p.lane) + p.yOff;
        if (!p.fromEnemy) {
            // rocket flying right – ~1.8× larger than original
            float flick = (std::fmod(p.x, 20.0f) < 10.0f) ? 7.0f : 0.0f;
            r.drawRect(p.x - 43 - flick, y - 3, 13 + flick, 7, { 1.00f, 0.75f, 0.20f }); // flame
            r.drawRect(p.x - 29, y - 7, 40, 14, { 0.76f, 0.26f, 0.17f }); // body
            r.drawRect(p.x - 29, y - 12, 11, 24, { 0.35f, 0.38f, 0.42f }); // fins
            r.drawRect(p.x + 11, y - 5, 9, 10, { 0.85f, 0.84f, 0.76f }); // nose
            r.drawRect(p.x + 20, y - 2, 5, 4, { 0.85f, 0.84f, 0.76f }); // tip
        }
        else if (p.high) {
            r.drawRect(p.x - 7, y - 7, 14, 14, { 0.20f, 0.60f, 0.15f });
            r.drawRect(p.x - 4, y - 4, 8, 8, { 0.60f, 1.00f, 0.40f });
        }
        else {
            r.drawRect(p.x - 10, y, 20, 6, { 0.20f, 0.60f, 0.15f });
            r.drawRect(p.x - 6, y + 2, 12, 2, { 0.60f, 1.00f, 0.40f });
        }
    }

    // Rocket explosion effects
    for (const Explosion& ex : explosions) {
        float t = ex.age / Explosion::DURATION;   // 0 = fresh, 1 = gone
        float invt = 1.0f - t;

        // Central flash: bright yellow-white square, shrinks and fades
        float flashS = 18.0f * invt;
        r.drawRect(ex.x - flashS, ex.y - flashS, flashS * 2.0f, flashS * 2.0f,
            { 1.0f, 0.95f, 0.55f }, invt);

        // Four cardinal sparks flying outward (orange)
        float ringD = 34.0f * t;
        float sqS = 7.0f * invt;
        glm::vec3 orange{ 1.0f, 0.50f, 0.05f };
        r.drawRect(ex.x + ringD - sqS * 0.5f, ex.y - sqS * 0.5f, sqS, sqS, orange, invt);
        r.drawRect(ex.x - ringD - sqS * 0.5f, ex.y - sqS * 0.5f, sqS, sqS, orange, invt);
        r.drawRect(ex.x - sqS * 0.5f, ex.y + ringD - sqS * 0.5f, sqS, sqS, orange, invt);
        r.drawRect(ex.x - sqS * 0.5f, ex.y - ringD - sqS * 0.5f, sqS, sqS, orange, invt);

        // Four diagonal debris pieces (red, slightly smaller, faster)
        float diagD = 24.0f * t;
        float spkS = 5.0f * invt;
        glm::vec3 red{ 1.0f, 0.22f, 0.0f };
        r.drawRect(ex.x + diagD - spkS * 0.5f, ex.y + diagD - spkS * 0.5f, spkS, spkS, red, invt * 0.85f);
        r.drawRect(ex.x - diagD - spkS * 0.5f, ex.y + diagD - spkS * 0.5f, spkS, spkS, red, invt * 0.85f);
        r.drawRect(ex.x + diagD - spkS * 0.5f, ex.y - diagD - spkS * 0.5f, spkS, spkS, red, invt * 0.85f);
        r.drawRect(ex.x - diagD - spkS * 0.5f, ex.y - diagD - spkS * 0.5f, spkS, spkS, red, invt * 0.85f);

        // Two horizontal smoke trails (dark, expand sideways, fade)
        float smokeW = 20.0f * t;
        float smokeH = 4.0f * invt;
        r.drawRect(ex.x + 4.0f * t, ex.y - smokeH * 0.5f, smokeW, smokeH, { 0.35f, 0.35f, 0.35f }, invt * 0.45f);
        r.drawRect(ex.x - 4.0f * t - smokeW, ex.y - smokeH * 0.5f, smokeW, smokeH, { 0.35f, 0.35f, 0.35f }, invt * 0.45f);
    }

    // Base HP bar (centred on the home-base column)
    {
        float baseMidY = (GRID_TOP + laneBottom(NUM_LANES - 1)) * 0.5f;
        drawHealthBar(r, 70.0f, baseMidY + 20.0f, 110.0f, baseHp, BASE_MAX_HP, false);
        r.drawText("BASE", 70.0f - r.textWidth("BASE", 2.5f) * 0.5f, baseMidY + 36.0f, 2.5f, { 0.80f, 0.85f, 0.90f });
    }

    // HUD (hidden on main menu and sound settings)
    if (st != GameState::MainMenu && st != GameState::Settings) {   // [George]
        drawCoinPanel(r, coins);
        cards.draw(r, coins, armedCard, hoveredCard);

        std::string wave = "WAVE " + std::to_string(currentWave) + "/" + std::to_string(WAVE_TOTAL);
        r.drawText(wave, WORLD_W - r.textWidth(wave, 4.0f) - 30.0f, 660.0f, 4.0f, { 0.90f, 0.95f, 1.00f });
    }
    if (st == GameState::Playing) {
        r.drawText("CLICK CARD THEN LANE.  CLICK LANE TO SELECT.", 490, 650, 2.0f, { 0.60f, 0.70f, 0.80f });
        r.drawText("W JUMP  S DUCK  ESC PAUSE  RIGHT CLICK CANCEL", 490, 630, 2.0f, { 0.60f, 0.70f, 0.80f });
    }

    // overlays
    if (st != GameState::Playing) {
        r.drawRect(0, 0, WORLD_W, WORLD_H, { 0.0f, 0.0f, 0.0f }, 0.6f);

        // [George] MainMenu is drawn by drawMainMenu() (early return above)
        if (st == GameState::Settings) {   // [George]
            drawCenteredText(r, "SOUND SETTINGS", 440, 7.0f, { 0.85f, 0.35f, 0.15f });
            drawButton(r, BTN_VOL_DOWN, "-");
            drawButton(r, BTN_VOL_UP, "+");
            std::string vol = musicMuted ? "MUSIC OFF" : "MUSIC " + std::to_string(musicVolPct) + "%";
            drawCenteredText(r, vol, 254, 4.0f, { 0.75f, 0.85f, 0.95f });
            drawButton(r, BTN_MUTE, musicMuted ? "UNMUTE" : "MUTE");
            drawCenteredText(r, "LEFT/RIGHT VOLUME   M MUTE   ESC BACK", 165, 2.0f, { 0.55f, 0.65f, 0.75f });
            drawButton(r, BTN_BACK, "BACK", hoveredMenuBtn == 0);
        }
        else if (st == GameState::Paused) {
            drawCenteredText(r, "PAUSED", 500, 10.0f, { 1, 1, 1 });
            drawButton(r, BTN_CONTINUE, "CONTINUE", hoveredMenuBtn == 0);
            drawButton(r, BTN_RESTART_P, "RESTART", hoveredMenuBtn == 1);
            drawButton(r, BTN_MAINMENU_P, "MAIN MENU", hoveredMenuBtn == 2);
            // Volume control
            drawButton(r, BTN_VOL_DOWN, "-");
            drawButton(r, BTN_VOL_UP, "+");
            std::string vol = musicMuted ? "MUSIC OFF" : "MUSIC " + std::to_string(musicVolPct) + "%";
            drawCenteredText(r, vol, 254, 4.0f, { 0.75f, 0.85f, 0.95f });
            drawButton(r, BTN_MUTE, musicMuted ? "UNMUTE" : "MUTE");
            drawCenteredText(r, "LEFT/RIGHT VOLUME   M MUTE", 160, 2.0f, { 0.55f, 0.65f, 0.75f });
        }
        else if (st == GameState::Won) {
            drawCenteredText(r, "VICTORY!", 460, 10.0f, { 0.4f, 1.0f, 0.5f });
            drawButton(r, BTN_RESTART_E, "RESTART", hoveredMenuBtn == 0);
            drawButton(r, BTN_MAINMENU_E, "MAIN MENU", hoveredMenuBtn == 1);
        }
        else {   // Lost
            drawCenteredText(r, "BASE OVERRUN", 460, 10.0f, { 1.0f, 0.35f, 0.3f });
            drawButton(r, BTN_RESTART_E, "RESTART", hoveredMenuBtn == 0);
            drawButton(r, BTN_MAINMENU_E, "MAIN MENU", hoveredMenuBtn == 1);
        }
    }

    // [George] lights come up after the cutscene (black overlay fading out)
    if (introFade > 0.0f)
        r.drawRect(0, 0, WORLD_W, WORLD_H, { 0.0f, 0.0f, 0.0f }, introFade);
}

void Game::applyVolume()
{
    // Engine volume scales everything: theme music AND every playSfx() sound
    Audio::setMasterVolume(musicMuted ? 0.0f : musicVolPct * 0.01f);
}

void Game::changeVolume(int deltaPct)
{
    musicVolPct = std::min(std::max(musicVolPct + deltaPct, 0), 100);
    musicMuted = false;          // touching the volume always unmutes
    applyVolume();
}

void Game::toggleMute()
{
    musicMuted = !musicMuted;    // musicVolPct is kept, so unmuting restores it
    applyVolume();
}

// ---------------------------------------------------------------------------
// [George] Main Menu scene (matches the menu reference image).
// The split Stark/Doom background image is drawn first by Background::drawMenuSplit (main.cpp).
// Only drawing lives here; buttons/hover/clicks are unchanged (BTN_*, hoveredMenuBtn).
// ---------------------------------------------------------------------------
void Game::drawMainMenu(Renderer& r) const
{
    const float t = (float)glfwGetTime();   // update() is frozen outside Playing, so use wall time
    const float midX = WORLD_W * 0.5f;
    const float groundTop = 124.0f;

    // --- darken the sky towards the top so the title reads -----------------
    for (int i = 0; i < 12; ++i) {
        float y = WORLD_H - (i + 1) * 30.0f;
        r.drawRect(0, y, WORLD_W, 30.0f, { 0.0f, 0.0f, 0.0f }, 0.40f * (1.0f - i / 12.0f));
    }
    // faint glow along the seam
    r.drawRect(midX - 30, groundTop, 60, WORLD_H - groundTop, { 0.4f, 0.9f, 1.0f }, 0.05f);
    r.drawRect(midX - 10, groundTop, 20, WORLD_H - groundTop, { 0.6f, 0.95f, 1.0f }, 0.06f);

    // --- floating particles: blue (Stark) left, green (Doom) right ----------
    for (int side = 0; side < 2; ++side) {
        glm::vec3 col = side == 0 ? glm::vec3{ 0.45f, 0.75f, 1.00f } : glm::vec3{ 0.45f, 1.00f, 0.45f };
        for (int i = 0; i < 45; ++i) {
            float n = (float)(i + side * 100);
            float x = side * midX + 8.0f + hash01(n) * (midX - 16.0f);
            float span = WORLD_H - groundTop;
            float y = groundTop + std::fmod(hash01(n + 7.1f) * span + t * (6.0f + hash01(n + 3.3f) * 14.0f), span);
            x += std::sin(t * 0.8f + n) * 4.0f;
            float sz = 2.0f + std::floor(hash01(n + 1.7f) * 3.0f);
            float a = 0.35f + 0.55f * (0.5f + 0.5f * std::sin(t * 2.5f + n * 1.3f));
            r.drawRect(x, y, sz, sz, col, a);
        }
    }

    // --- lightning bolt down the centre seam (flickers slightly) ------------
    {
        const float baseX[] = { 8, -18, 14, -26, 20, -12, 24, -20, 10, 0 };
        const int   N = 10;
        int seed = (int)(t * 9.0f);
        float flick = 0.70f + 0.30f * hash01((float)seed);
        float bx[N], by[N];
        for (int i = 0; i < N; ++i) {
            by[i] = WORLD_H - i * (WORLD_H - groundTop) / (N - 1);
            bx[i] = midX + baseX[i] + (hash01(seed * 31.0f + i) - 0.5f) * 8.0f;
        }
        auto bolt = [&](float x0, float y0, float x1, float y1, float scale) {
            drawLine(r, x0, y0, x1, y1, 10.0f * scale, { 0.30f, 0.85f, 1.00f }, 0.10f * flick);
            drawLine(r, x0, y0, x1, y1, 5.0f * scale, { 0.55f, 0.95f, 1.00f }, 0.35f * flick);
            drawLine(r, x0, y0, x1, y1, 2.0f * scale, { 1.00f, 1.00f, 1.00f }, 0.95f * flick);
        };
        for (int i = 0; i + 1 < N; ++i) bolt(bx[i], by[i], bx[i + 1], by[i + 1], 1.0f);
        // three thin side branches
        bolt(bx[2], by[2], bx[2] - 60, by[2] - 40, 0.6f);
        bolt(bx[4], by[4], bx[4] - 85, by[4] - 22, 0.6f);
        bolt(bx[7], by[7], bx[7] + 55, by[7] - 35, 0.6f);
    }

    // --- ground strip: blue neon grid (left) / green stone bricks (right) ---
    {
        // left: dark floor (slightly lighter towards the top) with a glowing grid
        for (int i = 0; i < 6; ++i) {
            float hgt = groundTop / 6.0f;
            float y = groundTop - (i + 1) * hgt;
            r.drawRect(0, y, midX, hgt, { 0.05f - 0.006f * i, 0.10f - 0.012f * i, 0.24f - 0.025f * i });
        }
        glm::vec3 grid{ 0.25f, 0.60f, 1.00f };
        const float hy[] = { 90.0f, 52.0f, 12.0f };
        for (float y : hy) r.drawRect(0, y, midX, 2.0f, grid, 0.55f);
        for (int i = 0; i <= 11; ++i) {   // slanted lines leaning like "/" (cheap perspective)
            float xt = i * 58.0f + 12.0f;
            drawLine(r, xt, groundTop, xt - 70.0f - i * 4.0f, 0.0f, 2.0f, grid, 0.50f);
        }
        r.drawRect(0, groundTop - 3, midX, 3, { 0.45f, 0.80f, 1.00f });        // bright edge
        r.drawRect(0, groundTop, midX, 4, { 0.35f, 0.75f, 1.00f }, 0.25f);    // edge glow

        // right: stone bricks (covers any grid line that spilled over the seam)
        r.drawRect(midX, 0, midX, groundTop, { 0.03f, 0.07f, 0.03f });         // mortar
        const float bh = 30.0f, bw = 76.0f, gap = 3.0f;
        for (int row = 0; row < 5; ++row) {
            float y = groundTop - (row + 1) * bh;
            float off = (row % 2) ? bw * 0.5f : 0.0f;
            for (float x = midX - off; x < WORLD_W; x += bw) {
                float x0 = std::max(x + gap, midX), x1 = std::min(x + bw - gap, WORLD_W);
                if (x1 <= x0) continue;
                float y0 = std::max(y + gap, 0.0f);
                float top = y + bh - gap;
                float shade = 0.85f + 0.3f * hash01(row * 17.0f + x);
                r.drawRect(x0, y0, x1 - x0, top - y0, glm::vec3{ 0.07f, 0.15f, 0.06f } * shade);
                r.drawRect(x0, top - 3, x1 - x0, 3, glm::vec3{ 0.11f, 0.24f, 0.09f } * shade);   // lit top edge
            }
        }
        r.drawRect(midX, groundTop - 3, midX, 3, { 0.35f, 0.85f, 0.30f });     // bright edge
        r.drawRect(midX, groundTop, midX, 4, { 0.35f, 0.85f, 0.30f }, 0.25f);  // edge glow
    }

    // --- characters on the ground (real units only), gentle idle animation --
    {
        struct MenuUnit { int def; float cx; float h; bool enemy; float phase; };
        const MenuUnit units[] = {
            { CH_DEFENSE, 95.0f,   185.0f, false, 0.0f },   // players face right
            { CH_MELEE,   262.0f,  142.0f, false, 0.4f },
            { CH_RANGED,  395.0f,   90.0f, false, 0.8f },
            { CH_EASY,    920.0f,  150.0f, true,  0.2f },   // enemies face left
            { CH_HARD,    1055.0f, 170.0f, true,  0.6f },
            { CH_MEDIUM,  1195.0f, 120.0f, true,  1.0f },
        };
        for (const MenuUnit& u : units) {
            const CharacterDef& d = g_chars[u.def];
            bool useB = std::fmod(t * 1.6f + u.phase * 2.0f, 2.0f) >= 1.0f;   // A/B swap ~every 0.6 s
            SpriteId id = useB ? d.frameB : d.frameA;
            float bob = std::max(0.0f, std::sin(t * 2.2f + u.phase * 3.0f)) * 3.0f;
            const SpriteFrame& f = g_sprites[id];
            float wReal = f.h > 0.0f ? u.h * f.w / f.h : 0.0f;
            r.drawRect(u.cx - wReal * 0.40f, groundTop - 6.0f, wReal * 0.80f, 6.0f, { 0.0f, 0.0f, 0.0f }, 0.45f); // shadow
            drawMenuSprite(r, id, u.cx, groundTop - 4.0f + bob, u.h, u.enemy);
        }
    }

    // --- title ---------------------------------------------------------------
    {
        const float px = 14.0f;
        const std::string a = "DOOMS ", b = "GAME";
        float total = r.textWidth(a + b, px) - px;
        float x0 = midX - total * 0.5f;
        float y = 556.0f;
        r.drawTextStyled(a, x0, y, px, { 0.60f, 0.98f, 0.42f }, { 0.13f, 0.52f, 0.12f },
                         { 0.02f, 0.08f, 0.03f }, 4.0f, 6.0f, 6.0f);
        r.drawTextStyled(b, x0 + r.textWidth(a, px), y, px, { 0.80f, 0.97f, 1.00f }, { 0.15f, 0.45f, 0.92f },
                         { 0.02f, 0.04f, 0.14f }, 4.0f, 6.0f, 6.0f);

        const std::string sub = "IRON MAN  VS  DR DOOM";
        const float sp = 4.5f;
        float sw = r.textWidth(sub, sp) - sp;
        r.drawTextStyled(sub, midX - sw * 0.5f, 484.0f, sp, { 1.00f, 0.82f, 0.35f }, { 0.88f, 0.45f, 0.10f },
                         { 0.10f, 0.05f, 0.01f }, 1.5f, 2.0f, 2.0f);
    }

    // --- button panel + buttons (rects = BTN_PLAY / BTN_SOUND / BTN_EXIT) -----
    r.drawRect(440.0f, 160.0f, 400.0f, 240.0f, { 0.02f, 0.03f, 0.08f }, 0.62f);
    r.drawRect(440.0f, 398.0f, 400.0f, 2.0f, { 0.45f, 0.60f, 0.85f }, 0.25f);
    drawMenuButton(r, BTN_PLAY,  "PLAY",           hoveredMenuBtn == 0, true,  5.0f);
    drawMenuButton(r, BTN_SOUND, "SOUND SETTINGS", hoveredMenuBtn == 1, false, 3.5f);
    drawMenuButton(r, BTN_EXIT,  "EXIT",           hoveredMenuBtn == 2, false, 3.5f);

    // --- bottom text ----------------------------------------------------------
    {
        const std::string s = "DEFEND STARK TOWER";
        float w = r.textWidth(s, 3.0f) - 3.0f;
        r.drawText(s, midX - w * 0.5f + 2, 24.0f, 3.0f, { 0.0f, 0.02f, 0.08f }, 0.8f);
        r.drawText(s, midX - w * 0.5f, 26.0f, 3.0f, { 0.55f, 0.80f, 1.00f });
        // only keys that work on this screen (onKey: ENTER / SPACE start; ESC does nothing on MainMenu)
        r.drawText("ENTER / SPACE TO PLAY", 18.0f, 8.0f, 2.0f, { 0.62f, 0.66f, 0.72f }, 0.85f);
    }
}
