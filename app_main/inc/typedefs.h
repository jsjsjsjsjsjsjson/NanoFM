#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef enum {
    WAVE_SINE,
    WAVE_HALF_SINE,
    WAVE_ABS_SINE,
    WAVE_CLIP_SINE,
    WAVE_PULSE,
    WAVE_TRI,
    WAVE_SAW,
    WAVE_LFSR,
} wave_type_t;

typedef enum {
    ENV_IDLE,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE,
} env_state_t;

typedef enum {
    ALG_FM,    // M -> C -> OUT
    ALG_FM_FB, // M (Feedback) -> C -> OUT
    ALG_ADD,   // M + C -> OUT
} channel_alg_t;

#endif