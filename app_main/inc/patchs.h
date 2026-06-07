#ifndef PATCHS_H
#define PATCHS_H

#include <stdint.h>
#include "typedefs.h"

typedef struct {
    wave_type_t wave;

    uint32_t attack;
    uint32_t decay;
    uint16_t sustain;
    uint32_t release;

    int8_t octave;
    int8_t semi;

    uint32_t level;

    uint32_t freq_lfo_rate_milli_hz;
    uint16_t freq_lfo_depth;

    uint32_t amp_lfo_rate_milli_hz;
    uint16_t amp_lfo_depth;
} op_patch_t;

typedef struct {
    channel_alg_t alg;

    uint32_t mod_index;
    uint8_t feedback;

    op_patch_t op[2];
} patch_t;

#define PATCH_BANK_SIZE 8

#define OP_PATCH(wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_) \
    { wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, 0, 0, 0, 0 }

#define OP_PATCH_LFO(wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, freq_lfo_rate_milli_hz_, freq_lfo_depth_) \
    { wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, freq_lfo_rate_milli_hz_, freq_lfo_depth_, 0, 0 }

#define OP_PATCH_AMP_LFO(wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, amp_lfo_rate_milli_hz_, amp_lfo_depth_) \
    { wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, 0, 0, amp_lfo_rate_milli_hz_, amp_lfo_depth_ }

#define OP_PATCH_LFOS(wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, freq_lfo_rate_milli_hz_, freq_lfo_depth_, amp_lfo_rate_milli_hz_, amp_lfo_depth_) \
    { wave_, attack_, decay_, sustain_, release_, octave_, semi_, level_, freq_lfo_rate_milli_hz_, freq_lfo_depth_, amp_lfo_rate_milli_hz_, amp_lfo_depth_ }

#define PATCH_FM(mod_index_, feedback_, mod_, car_) \
    { ALG_FM, mod_index_, feedback_, { mod_, car_ } }

#define PATCH_FM_FB(mod_index_, feedback_, mod_, car_) \
    { ALG_FM_FB, mod_index_, feedback_, { mod_, car_ } }

#define PATCH_ADD(mod_, car_) \
    { ALG_ADD, 0, 0, { mod_, car_ } }

static const patch_t patch_bank[PATCH_BANK_SIZE] = {
    // 1 Acoustic Grand Piano
    PATCH_FM(
        65536,
        0,
        OP_PATCH(WAVE_HALF_SINE, 0, 130, 0, 80, 0, 0, 65536),
        OP_PATCH(WAVE_SINE, 0, 220, 0, 130, 0, 0, 65535)
    ),

    // 2 Bright Acoustic Piano
    PATCH_FM(
        26000,
        0,
        OP_PATCH(WAVE_HALF_SINE, 0, 95, 0, 70, 0, 0, 65536),
        OP_PATCH(WAVE_SINE, 0, 180, 0, 110, 0, 0, 65535)
    ),

    // 3 Electric Grand Piano
    PATCH_FM(
        14000,
        0,
        OP_PATCH(WAVE_SINE, 0, 180, 9000, 140, 1, 0, 65536),
        OP_PATCH(WAVE_HALF_SINE, 2, 260, 18000, 170, 0, 0, 62000)
    ),

    // 4 Honky-tonk Piano
    PATCH_ADD(
        OP_PATCH(WAVE_SINE, 0, 220, 0, 120, 0, -1, 65536),
        OP_PATCH(WAVE_SINE, 0, 230, 0, 120, 0, 1, 52000)
    ),

    // 5 Electric Piano 1
    PATCH_FM(
        11000,
        0,
        OP_PATCH(WAVE_SINE, 0, 240, 12000, 180, 1, 0, 65536),
        OP_PATCH(WAVE_SINE, 4, 300, 26000, 220, 0, 0, 61000)
    ),

    // 6 Electric Piano 2
    PATCH_FM_FB(
        24000,
        1,
        OP_PATCH(WAVE_SINE, 0, 180, 8000, 150, 1, 0, 65536),
        OP_PATCH(WAVE_ABS_SINE, 3, 320, 24000, 240, 0, 0, 60000)
    ),

    // 7 Harpsichord
    PATCH_FM(
        32000,
        0,
        OP_PATCH(WAVE_SINE, 0, 90, 0, 45, 2, 0, 65536),
        OP_PATCH(WAVE_SAW, 0, 120, 0, 55, 0, 0, 60000)
    ),

    // 8 Clavi
    PATCH_FM_FB(
        28000,
        2,
        OP_PATCH(WAVE_PULSE, 0, 80, 0, 45, 1, 0, 65536),
        OP_PATCH(WAVE_CLIP_SINE, 0, 130, 6000, 60, 0, 0, 62000)
    ),
};

#endif
