#pragma once

namespace cfg {
	constexpr int   WIN_W = 1280;
	constexpr int   WIN_H = 720;
	constexpr float WORLD_W = 1280.0f;
	constexpr float WORLD_H = 720.0f;

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
	constexpr float GRAVITY = 1500.0f;
	constexpr float JUMP_SPEED = 560.0f;  // peak ~100 px
	constexpr float DUCK_TIME = 0.7f;    // seconds a duck lasts
}

inline float laneBottom(int lane) { return cfg::GRID_TOP - cfg::LANE_H * (lane + 1); }