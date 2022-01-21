#ifndef AUDIO_H
#define AUDIO_H

/* allow audio control */
typedef struct {
    bool is_music;
    uint16_t id;
} sound_id;

sound_id load_sound(char* path);
sound_id load_music(char* path);

void     unload_sound(sound_id sound);

void audio_initialize(void);
void audio_deinitialize(void);

/*this is not a really good option, but it works for simple cases*/
void play_sound(sound_id id);

void play_music(sound_id id, int loops);
void play_sound_on_channel(sound_id id, int channel);

void stop_music(void);
void stop_all_sounds(void);

#endif
