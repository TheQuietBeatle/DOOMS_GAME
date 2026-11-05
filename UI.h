// UI.h - HUD elements: card bar, battlefield grid, coin panel, health bars.
// All drawing uses only Renderer primitives (drawRect/drawSprite/drawText).
// CardBar handles both drawing and hit-testing the hero selection cards.
#pragma once
#include "Renderer.h"
#include <string>

// The bar of player cards at the top-left of the screen.
// DMET502 spec requirement: card UI with locked/unlocked state and hover highlight.
struct CardBar {
    float x0 = 165.0f;   // left edge of the first card
    float y0 = 615.0f;   // bottom edge of the cards
    float w = 85.0f, h = 95.0f, gap = 12.0f;

    // Returns the top-left corner of card i in world coordinates.
    void cardRect(int i, float& x, float& y) const;
    // Returns the card index the point (wx, wy) is over, or -1 if none.
    int  hitTest(float wx, float wy) const;
    // Draws all cards; grayscale = locked (DMET502 spec requirement), tinted = armed.
    void draw(Renderer& r, int coins, int armedCard, int hoveredCard) const;
};

// Draws the lane backgrounds, grid, ground decorations, and hover/selection highlights.
// DMET502 spec requirement: lane hover highlight and lane selection indicator.
void drawBattlefield(Renderer& r, int hoveredLane, const bool* selectedLanes, float time);

// Draws the coin counter panel (top-right of the screen).
void drawCoinPanel(Renderer& r, int coins);

// Draws a colour-coded HP bar centred at (centerX, y); isEnemy = red, player = green.
// DMET502 spec requirement: health bars visible on all units.
void drawHealthBar(Renderer& r, float centerX, float y, float width, int hp, int maxHp, bool isEnemy);

// Draws text centred horizontally at the given world y.
void drawCenteredText(Renderer& r, const std::string& s, float y, float px, glm::vec3 color);
