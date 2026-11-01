#define MINIAUDIO_IMPLEMENTATION     // ONLY here, nowhere else
#include "miniaudio.h"
#include "Audio.h"

static ma_engine engine;
static ma_sound  music;
static bool      ready = false;       // engine started
static bool      musicReady = false;  // theme file loaded
static ma_sound  voice;               // [George] current voice line
static bool      voiceReady = false;
static float     masterVol = 1.0f;
static float     musicVol = 0.3f;     // [George] theme starts at 30% (matches Game::musicVolPct = 30)

static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

bool Audio::init(const char* musicPath) {
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) return false;
    ready = true;

    if (ma_sound_init_from_file(&engine, musicPath, MA_SOUND_FLAG_STREAM,
        NULL, NULL, &music) != MA_SUCCESS)
        return false;
    musicReady = true;

    ma_sound_set_looping(&music, MA_TRUE);
    ma_sound_set_volume(&music, musicVol);
    ma_sound_start(&music);
    return true;
}

void Audio::shutdown() {
    if (!ready) return;
    if (musicReady) ma_sound_uninit(&music);
    if (voiceReady) ma_sound_uninit(&voice);
    ma_engine_uninit(&engine);
    ready = musicReady = voiceReady = false;
}

void Audio::pauseMusic() { if (musicReady) ma_sound_stop(&music); }
void Audio::resumeMusic() { if (musicReady) ma_sound_start(&music); }
void Audio::restartMusic() { if (musicReady) { ma_sound_seek_to_pcm_frame(&music, 0); ma_sound_start(&music); } }

void Audio::setMasterVolume(float v) {
    masterVol = clamp01(v);
    if (ready) ma_engine_set_volume(&engine, masterVol);
}
float Audio::getMasterVolume() { return masterVol; }

void Audio::setMusicVolume(float v) {
    musicVol = clamp01(v);
    if (musicReady) ma_sound_set_volume(&music, musicVol);
}
float Audio::getMusicVolume() { return musicVol; }

void Audio::playSfx(const char* path) {
    if (ready) ma_engine_play_sound(&engine, path, NULL);
}

void Audio::playVoice(const char* path, bool force) {
    if (!ready) return;
    if (voiceReady) {
        if (!force && ma_sound_is_playing(&voice)) return;   // don't stack voice lines
        ma_sound_uninit(&voice);
        voiceReady = false;
    }
    if (ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &voice) == MA_SUCCESS) {
        voiceReady = true;
        ma_sound_start(&voice);
    }
}
