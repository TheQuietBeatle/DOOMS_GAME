// Config.h - Compile-time constants for the entire game.
// All world-space sizes, lane geometry, and tuning values live here so
// they can be changed in one place without touching game logic.
#pragma once

namespace cfg {
	constexpr int   WIN_W = 1280;
	constexpr int   WIN_H = 720;
	constexpr float WORLD_W = 1280.0f;
	constexpr float WORLD_H = 720.0f;

	// NOTE: value is 5 here but the game uses indices 0-3; WaveSystem.h comments say "0 to 3".
	// laneBottom(NUM_LANES - 1) is the bottom lane, so only 4 lanes are active.
	constexpr int   NUM_LANES = 5;
	constexpr int   BASE_MAX_HP = 1;
	constexpr float LANE_H = 110.0f;
	constexpr float GRID_TOP = 600.0f;   // top edge of lane 0
	constexpr float GRID_LEFT = 140.0f;   // home base ends here
	constexpr float GRID_RIGHT = 1260.0f;  // enemies spawn here
	constexpr int   GRID_COLS = 10;
	constexpr float GROUND_PAD = 8.0f;     // feet offset from lane bottom

	constexpr int   START_COINS = 150;     // = 3 melee players
	constexpr int   COIN_DROP = 50;      // value of one collectible
	constexpr float GRAVITY = 850.0f;
	constexpr float JUMP_SPEED = 420.0f;  
	constexpr float DUCK_TIME = 1.2f;    // seconds a duck lasts
}

// y-coordinate of the bottom edge of a lane (world coords, y up).
// Lane 0 is the top lane; lane NUM_LANES-1 is the bottom lane.
// Called by: Game.cpp (unit placement, projectile height), UI.cpp (battlefield drawing).
inline float laneBottom(int lane) { return cfg::GRID_TOP - cfg::LANE_H * (lane + 1); }