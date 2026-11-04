#pragma once
// ---------------------------------------------------------------------------
// Intro cutscene: "The Reactor Heist"  (~18 seconds)
//
//  Shot 1  0-3s    New York at peace, Stark Tower + pulsing Arc Reactor
//  Shot 2  3-6s    Sky turns green, lightning, castle appears, Doom floats in
//  Shot 3  6-9s    Doombots march in, Doom demands the reactor
//  Shot 4  9-12s   Iron Man flies down and lands in front of the tower
//  Shot 5  12-15s  Repulsor beam vs. Doom bolt clash, screen shake
//  Shot 6  15-18s  Doombots spread into lanes, grid appears -> gameplay
//
// Uses only Renderer::drawRect / drawSprite / drawText and the existing
// sprites in g_sprites[], so the characters match the gameplay models.
// ---------------------------------------------------------------------------
#include "Renderer.h"

class Cutscene {
public:
    static constexpr float DURATION = 18.0f;

    void start();                    // reset to t = 0
    void update(float dt);           // advance time
    void draw(Renderer& r) const;    // draw the current frame
    void skip();                     // jump to the end (Space / Enter)
    bool finished() const { return done; }

private:
    float t = 0.0f;
    bool  done = false;
};