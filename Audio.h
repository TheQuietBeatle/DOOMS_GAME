// Audio.h - Thin wrapper around miniaudio for background music and sound effects.
// One streaming music track (theme) plays on loop; SFX are fire-and-forget;
// voice lines are single-at-a-time with optional force-interrupt.
#pragma once

namespace Audio {
    bool init(const char* musicPath);
    void shutdown();

    void pauseMusic();
    void resumeMusic();
    void restartMusic();
    void switchMusic(const char* path); // stop current theme, play new file on loop
    void setMasterVolume(float v);   // 0.0 – 1.0 (everything: music + sfx)
    float getMasterVolume();
    void setMusicVolume(float v);    // 0.0 – 1.0 (theme music only, default 0.3)
    float getMusicVolume();

    void playSfx(const char* path);  // fire-and-forget: coin pickup, death, shot
    // Character voice line: only one plays at a time. If one is already
    // playing it is skipped, unless force = true (win / lose), which cuts it off.
    void playVoice(const char* path, bool force = false);
}