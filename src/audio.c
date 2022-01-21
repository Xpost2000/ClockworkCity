#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "common.h"
#include "audio.h"

/*
  id system is bad right now
*/

#define AUDIO_MAX_SOUNDS (4096)
#define AUDIO_MAX_MUSIC  (512)

local uint16_t sound_chunk_count = 0;
local Mix_Chunk* sound_chunks[AUDIO_MAX_SOUNDS] = {};
local uint16_t music_count = 0;
local Mix_Music* music_buffers[AUDIO_MAX_MUSIC] = {};

void audio_initialize(void) {
    assert(Mix_OpenAudio(44100, AUDIO_S16SYS, 8, 2048) == 0 && "audio bork?");
}

void audio_deinitialize(void) {
    Mix_CloseAudio();
}

sound_id load_sound(char* path) {
    assert(sound_chunk_count < AUDIO_MAX_SOUNDS && "too many sounds?");
    uint16_t next_id = sound_chunk_count++;
    sound_chunks[next_id] = Mix_LoadWAV(path);

    return (sound_id) {
        .id = next_id
    };
}

sound_id load_music(char* path) {
    assert(music_count < AUDIO_MAX_MUSIC && "too much music");
    uint16_t next_id = music_count++;
    music_buffers[next_id] = Mix_LoadMUS(path);

    return (sound_id) {
        .id = next_id,
        .is_music = true,
    };
}

void unload_sound(sound_id sound) {
    unimplemented();
}

void play_sound(sound_id id) {
    if (id.is_music) {
        play_music(id, 1);
    } else {
        play_sound_on_channel(id, -1);
    }
}

void play_sound_on_channel(sound_id id, int channel) {
    not_really_important_assert(!id.is_music && "no functionality to play music like this.");

    Mix_Chunk* sound_chunk = sound_chunks[id.id];
    Mix_PlayChannel(channel, sound_chunk, 0);
}

void play_music(sound_id id, int loops) {
    assert(id.is_music);

    Mix_Music* music_buffer = music_buffers[id.id];
    Mix_PlayMusic(music_buffer, loops);
}

void stop_music(void) {
    Mix_HaltMusic();
}

void stop_all_sounds(void) {
    Mix_HaltChannel(-1);
}



