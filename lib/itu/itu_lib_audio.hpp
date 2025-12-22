#ifndef ITU_LIB_AUDIO_HPP
#define ITU_LIB_AUDIO_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3_mixer/SDL_mixer.h>
#include <itu_common.hpp>
#endif

#define NUM_TRACKS_MAX 32

struct AudioSystemContext
{
    MIX_Mixer* mixer;
    int track_music_ref;
    MIX_Track* tracks[NUM_TRACKS_MAX];
    Uint64     track_time_start[NUM_TRACKS_MAX];
    int tracks_count;
    float gain_music;
    float gain_sfx;
};

void sys_audio_init(int tracks_count);
void sys_audio_play_music(MIX_Audio* audio, Sint64 crossfade_duration_ms);
void sys_audio_stop_music(MIX_Audio* audio, Sint64 fadeout_duration_ms);
void sys_audio_stop_music_immediate(MIX_Audio* audio);
void sys_audio_play_music_immediate(MIX_Audio* audio);
void sys_audio_play_sfx(MIX_Audio* audio);
void sys_audio_set_gain_master(float gain);
void sys_audio_set_gain_music(float gain);
void sys_audio_set_gain_sfx(float gain);

#endif // ITU_LIB_AUDIO_HPP

#ifdef ITU_LIB_AUDIO_IMPLEMENTATION

static AudioSystemContext sys_audio_ctx;

// Internal Helpers
void play_audio_track(int track_idx, MIX_Audio* audio, float gain, SDL_PropertiesID props)
{
    MIX_SetTrackGain(sys_audio_ctx.tracks[track_idx], gain);
    MIX_SetTrackAudio(sys_audio_ctx.tracks[track_idx], audio);
    MIX_PlayTrack(sys_audio_ctx.tracks[track_idx], props);
    sys_audio_ctx.track_time_start[track_idx] = SDL_GetTicks();
}

int find_free_track()
{
    Uint64 oldest_start_time = -1;
    int    oldest_start_idx  = 0;
    for(int i = 0; i < sys_audio_ctx.tracks_count; ++i)
    {
        if(!MIX_TrackPlaying(sys_audio_ctx.tracks[i]))
            return i;
        else if(sys_audio_ctx.track_time_start[i] < oldest_start_time)
        {
            oldest_start_time = sys_audio_ctx.track_time_start[i];
            oldest_start_idx  = i;
        }
    }
    return oldest_start_idx;
}

void sys_audio_init(int tracks_count)
{
    if(tracks_count > NUM_TRACKS_MAX) tracks_count = NUM_TRACKS_MAX;

    MIX_Init(); 
    
    sys_audio_ctx.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if(!sys_audio_ctx.mixer) SDL_Log("Mixer Error: %s", SDL_GetError());

    sys_audio_ctx.track_music_ref = -1;
    sys_audio_ctx.tracks_count = tracks_count;
    
    for(int i = 0; i < tracks_count; ++i)
        sys_audio_ctx.tracks[i] = MIX_CreateTrack(sys_audio_ctx.mixer);

    sys_audio_ctx.gain_music = 1.0f;
    sys_audio_ctx.gain_sfx = 1.0f;
}

void sys_audio_play_music(MIX_Audio* audio, Sint64 crossfade_duration_ms)
{
    if(!audio) return;

    if(sys_audio_ctx.track_music_ref != -1)
    {
        MIX_Track* track_fadeout = sys_audio_ctx.tracks[sys_audio_ctx.track_music_ref];
        Sint64 fadeout_frames = MIX_TrackMSToFrames(track_fadeout, crossfade_duration_ms);
        MIX_StopTrack(track_fadeout, fadeout_frames);
    }
    
    int track_idx = find_free_track();
    MIX_Track* track_fadein = sys_audio_ctx.tracks[track_idx];

    Sint64 fadein_frames = MIX_TrackMSToFrames(track_fadein, crossfade_duration_ms);
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_FRAMES_NUMBER, fadein_frames);
    play_audio_track(track_idx, audio, sys_audio_ctx.gain_music, props);
    sys_audio_ctx.track_music_ref = track_idx;
}

void sys_audio_play_music_immediate(MIX_Audio* audio)
{
    if(!audio) return;
    if(sys_audio_ctx.track_music_ref == -1)
        sys_audio_ctx.track_music_ref = find_free_track();
    play_audio_track(sys_audio_ctx.track_music_ref, audio, sys_audio_ctx.gain_music, 0);
}

void sys_audio_stop_music(MIX_Audio* audio, Sint64 fadeout_duration_ms)
{
    if(sys_audio_ctx.track_music_ref == -1) return;

    MIX_Track* track_fadeout = sys_audio_ctx.tracks[sys_audio_ctx.track_music_ref];
    Sint64 fadeout_frames = MIX_TrackMSToFrames(track_fadeout, fadeout_duration_ms);
    MIX_StopTrack(track_fadeout, fadeout_frames);
    sys_audio_ctx.track_music_ref = -1;
}

void sys_audio_stop_music_immediate(MIX_Audio* audio)
{
    if(sys_audio_ctx.track_music_ref == -1) return;

    MIX_Track* track_fadeout = sys_audio_ctx.tracks[sys_audio_ctx.track_music_ref];
    MIX_StopTrack(track_fadeout, 0);
    sys_audio_ctx.track_music_ref = -1;
}



void sys_audio_play_sfx(MIX_Audio* audio)
{
    if(!audio) return;
    int track_idx = find_free_track();
    play_audio_track(track_idx, audio, sys_audio_ctx.gain_sfx, 0);
}

void sys_audio_play_sfx_gain(MIX_Audio* audio, float volume) 
{
    if(!audio) return;
    int track_idx = find_free_track();
    float new_gain = sys_audio_ctx.gain_sfx * volume;
    play_audio_track(track_idx, audio, new_gain, 0);
}


void sys_audio_set_gain_master(float gain)
{
    MIX_SetMasterGain(sys_audio_ctx.mixer, gain);
}

void sys_audio_set_gain_music(float gain)
{
    sys_audio_ctx.gain_music = gain;
    if(sys_audio_ctx.track_music_ref != -1)
        MIX_SetTrackGain(sys_audio_ctx.tracks[sys_audio_ctx.track_music_ref], gain);
}

void sys_audio_set_gain_sfx(float gain)
{
    sys_audio_ctx.gain_sfx = gain;
    for(int i = 0; i < sys_audio_ctx.tracks_count; ++i)
    {
        if(i == sys_audio_ctx.track_music_ref) continue;
        MIX_SetTrackGain(sys_audio_ctx.tracks[i], gain);
    }
}

void sys_audio_close()
{
    for(int i = 0; i < sys_audio_ctx.tracks_count; ++i) {
        if(sys_audio_ctx.tracks[i]) {
            MIX_DestroyTrack(sys_audio_ctx.tracks[i]);
            sys_audio_ctx.tracks[i] = NULL;
        }
    }
    if (sys_audio_ctx.mixer) {
        MIX_DestroyMixer(sys_audio_ctx.mixer);
        sys_audio_ctx.mixer = NULL;
    }
    MIX_Quit();
}



#endif // ITU_LIB_AUDIO_IMPLEMENTATION
