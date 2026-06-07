#pragma once

#include <stdint.h>
#include "config.h"
#include "luts.h"
#include "typedefs.h"
#include "patchs.h"

// ChatGPT suggested I use this to record the maximum amplitude :D
#define LEVEL_MAX 65536u

#define CC_VOICE_ALG 14
#define CC_VOICE_MOD_INDEX 15
#define CC_VOICE_FEEDBACK 16

#define CC_OP0_WAVE 20
#define CC_OP0_LEVEL 21
#define CC_OP0_OCTAVE 22
#define CC_OP0_SEMI 23
#define CC_OP0_ATTACK 24
#define CC_OP0_DECAY 25
#define CC_OP0_SUSTAIN 26
#define CC_OP0_RELEASE 27
#define CC_OP0_FREQ_LFO_RATE 28
#define CC_OP0_FREQ_LFO_DEPTH 29

#define CC_OP1_WAVE 40
#define CC_OP1_LEVEL 41
#define CC_OP1_OCTAVE 42
#define CC_OP1_SEMI 43
#define CC_OP1_ATTACK 44
#define CC_OP1_DECAY 45
#define CC_OP1_SUSTAIN 46
#define CC_OP1_RELEASE 47
#define CC_OP1_FREQ_LFO_RATE 48
#define CC_OP1_FREQ_LFO_DEPTH 49

#define CC_MAX_ENV_MS 2000u
#define CC_MAX_FREQ_LFO_RATE_MILLI_HZ 20000u
#define CC_MAX_FREQ_LFO_DEPTH 4096u

static inline int16_t clamp16(int32_t x) {
    if (x > 32767) {
        return 32767;
    }

    if (x < -32768) {
        return -32768;
    }

    return (int16_t)x;
}

static inline int16_t softclip(int32_t x) {
    if (x > 32767)
        x = 32767;
    else if (x < -32768)
        x = -32768;

    int32_t x2 = (x * x) >> 15;
    int32_t x3 = (x2 * x) >> 15;

    int32_t y = x - (x3 / 3);

    // gain compensation: roughly *1.5
    y = y + (y >> 1);

    if (y > 32767)
        y = 32767;
    else if (y < -32768)
        y = -32768;

    return (int16_t)y;
}

static inline uint32_t phase_inc_to_freq_hz(uint32_t inc) {
    uint32_t hi = inc >> 16;
    uint32_t lo = inc & 0xFFFFu;
    uint32_t hi_mul = hi * SAMP_RATE;
    uint32_t lo_mul = lo * SAMP_RATE;
    uint32_t low = (hi_mul << 16) + lo_mul;
    uint32_t carry = (low < lo_mul) ? 1u : 0u;

    return (hi_mul >> 16) + carry;
}

static inline uint32_t ratio_to_phase_inc(uint32_t num, uint32_t den) {
    uint32_t inc = 0;

    if (den == 0) {
        return 0;
    }

    if (num >= den) {
        return 0xFFFFFFFFu;
    }

    for (uint32_t i = 0; i < 32; i++) {
        num <<= 1;
        inc <<= 1;

        if (num >= den) {
            num -= den;
            inc |= 1u;
        }
    }

    return inc;
}

static inline uint32_t freq_hz_to_phase_inc(uint32_t freq) {
    return ratio_to_phase_inc(freq, SAMP_RATE);
}

static inline uint32_t freq_milli_hz_to_phase_inc(uint32_t milli_hz) {
    return ratio_to_phase_inc(milli_hz, SAMP_RATE * 1000u);
}

static inline uint32_t mul_u32_u16_shr15(uint32_t x, uint16_t y) {
    return (((x >> 16) * (uint32_t)y) << 1) +
           (((x & 0xFFFFu) * (uint32_t)y) >> 15);
}

static inline uint32_t cc_to_level(uint8_t value) {
    return ((uint32_t)value * LEVEL_MAX * 8) / 127u;
}

static inline uint16_t cc_to_sustain(uint8_t value) {
    return (uint16_t)(((uint32_t)value * 65535u) / 127u);
}

static inline uint32_t cc_to_env_ms(uint8_t value) {
    return ((uint32_t)value * CC_MAX_ENV_MS) / 127u;
}

static inline uint32_t cc_to_freq_lfo_rate_phase_inc(uint8_t value) {
    uint32_t milli_hz =
        ((uint32_t)value * CC_MAX_FREQ_LFO_RATE_MILLI_HZ) / 127u;

    return freq_milli_hz_to_phase_inc(milli_hz);
}

static inline uint16_t cc_to_freq_lfo_depth(uint8_t value) {
    return (uint16_t)(((uint32_t)value * CC_MAX_FREQ_LFO_DEPTH) / 127u);
}

static inline int8_t cc_to_octave(uint8_t value) {
    return (int8_t)((((uint32_t)value * 4u) / 127u) - 2);
}

static inline int8_t cc_to_semi(uint8_t value) {
    return (int8_t)((((uint32_t)value * 24u) / 127u) - 12);
}

static inline wave_type_t cc_to_wave(uint8_t value) {
    uint8_t wave = ((uint32_t)value * 8u) / 128u;

    if (wave > WAVE_LFSR) {
        wave = WAVE_LFSR;
    }

    return (wave_type_t)wave;
}

static inline channel_alg_t cc_to_alg(uint8_t value) {
    if (value < 43) {
        return ALG_FM;
    }

    if (value < 86) {
        return ALG_FM_FB;
    }

    return ALG_ADD;
}

class ADSR {
private:
    env_state_t state = ENV_IDLE;

    uint32_t level = 0;
    uint32_t sustain = 32768;

    uint32_t attack_ms = 5;
    uint32_t decay_ms = 80;
    uint32_t release_ms = 100;

    uint32_t attack_step = 1;
    uint32_t decay_step = 1;
    uint32_t release_step = 1;

    uint32_t attack_limit = 0;
    uint32_t decay_limit = 32768;

    uint32_t calc_step(uint32_t range, uint32_t ms) {
        if (ms == 0) {
            return range;
        }

        uint32_t samples = (SAMP_RATE * ms) / 1000u;
        if (samples == 0) {
            return range;
        }

        uint32_t step = (uint32_t)((range + samples - 1u) / samples);
        if (step == 0) {
            step = 1;
        }

        return step;
    }

    void refresh_attack_step() {
        attack_step = calc_step(LEVEL_MAX, attack_ms);
        attack_limit = (attack_step < LEVEL_MAX) ? (LEVEL_MAX - attack_step) : 0;
    }

    void refresh_decay_step() {
        decay_step = calc_step(LEVEL_MAX - sustain, decay_ms);
        decay_limit = sustain + decay_step;
    }

    void refresh_release_step() {
        release_step = calc_step(LEVEL_MAX, release_ms);
    }

public:
    ADSR() {
        refresh_attack_step();
        refresh_decay_step();
        refresh_release_step();
    }

    void set_attack(uint32_t ms) {
        attack_ms = ms;
        refresh_attack_step();
    }

    void set_decay(uint32_t ms) {
        decay_ms = ms;
        refresh_decay_step();
    }

    void set_sustain(uint32_t s) {
        if (s > LEVEL_MAX) {
            s = LEVEL_MAX;
        }

        sustain = s;
        refresh_decay_step();
    }

    void set_release(uint32_t ms) {
        release_ms = ms;
        refresh_release_step();
    }

    void set_adsr(uint32_t a, uint32_t d, uint32_t s, uint32_t r) {
        attack_ms = a;
        decay_ms = d;
        sustain = s;
        release_ms = r;

        if (sustain > LEVEL_MAX) {
            sustain = LEVEL_MAX;
        }

        refresh_attack_step();
        refresh_decay_step();
        refresh_release_step();
    }

    void note_on() {
        if (attack_ms == 0) {
            level = LEVEL_MAX;
            state = ENV_DECAY;
        } else {
            state = ENV_ATTACK;
        }
    }

    void note_off() {
        if (state == ENV_IDLE) {
            return;
        }

        state = ENV_RELEASE;
    }

    void reset() {
        level = 0;
        state = ENV_IDLE;
    }

    uint32_t process() {
        if (state == ENV_SUSTAIN) {
            return level;
        }

        if (state == ENV_IDLE) {
            return 0;
        }

        switch (state) {
        case ENV_ATTACK:
            if (level >= attack_limit) {
                level = LEVEL_MAX;
                state = ENV_DECAY;
            } else {
                level += attack_step;
            }
            break;

        case ENV_DECAY:
            if (level <= decay_limit) {
                level = sustain;
                state = ENV_SUSTAIN;
            } else {
                level -= decay_step;
            }
            break;

        case ENV_IDLE:
        case ENV_SUSTAIN:
            break;

        case ENV_RELEASE:
            if (level <= release_step) {
                level = 0;
                state = ENV_IDLE;
            } else {
                level -= release_step;
            }
            break;
        }

        return level;
    }

    uint32_t get_level() {
        return level;
    }

    uint8_t is_active() {
        return state != ENV_IDLE;
    }
};

class OP {
private:
    uint8_t note = 0;
    int8_t octave = 0;
    int8_t semi = 0;
    uint32_t freq = 0;
    uint32_t level = LEVEL_MAX;

    uint32_t phase_inc = 0;
    uint32_t phase = 0;
    uint32_t freq_lfo_phase_inc = 0;
    uint32_t freq_lfo_phase = 0;
    uint16_t freq_lfo_depth = 0;
    uint32_t lfsr = 0xCAFEBABE;
    wave_type_t wave_type = WAVE_SINE;

    ADSR env;

    int16_t lfsr_next() {
        uint32_t bit =
            ((lfsr >> 0) ^
            (lfsr >> 1) ^
            (lfsr >> 21) ^
            (lfsr >> 31)) & 1u;

        lfsr = (lfsr >> 1) | (bit << 31);

        return (int16_t)(lfsr >> 16);
    }

    int16_t wave_lookup(uint32_t p) {
        int16_t s = sin_lut[p >> (32 - WAVE_BITS)];

        switch (wave_type) {
        case WAVE_SINE:
            return s;

        case WAVE_HALF_SINE:
            return s > 0 ? s : 0;

        case WAVE_ABS_SINE:
            return s >= 0 ? s : (int16_t)-s;

        case WAVE_CLIP_SINE:
            if (s > 16384) return 16384;
            if (s < -16384) return -16384;
            return s;

        case WAVE_PULSE:
            return (p < 0x80000000u) ? 32767 : -32768;

        case WAVE_SAW:
            return (int16_t)(p >> 16);

        case WAVE_TRI: {
            uint32_t x = p >> 16;
            if (x & 0x8000) {
                return (int16_t)(32767 - ((x & 0x7FFF) << 1));
            } else {
                return (int16_t)(-32768 + (x << 1));
            }
        }

        case WAVE_LFSR:
            return lfsr_next();

        default:
            return s;
        }
    }

    uint32_t current_phase_inc() {
        if (freq_lfo_phase_inc == 0 || freq_lfo_depth == 0) {
            return phase_inc;
        }

        int32_t lfo = sin_lut[freq_lfo_phase >> (32 - WAVE_BITS)];
        uint32_t lfo_abs = (lfo < 0) ? (uint32_t)-lfo : (uint32_t)lfo;
        uint16_t depth_scale =
            (uint16_t)((lfo_abs * (uint32_t)freq_lfo_depth) >> 15);
        uint32_t delta = mul_u32_u16_shr15(phase_inc, depth_scale);

        if (lfo < 0 && delta >= phase_inc) {
            return 0;
        }

        if (lfo < 0) {
            return phase_inc - delta;
        }

        if ((0xFFFFFFFFu - phase_inc) < delta) {
            return 0xFFFFFFFFu;
        }

        return phase_inc + delta;
    }

public:
    void set_note(uint8_t n) {
        note = n;
    }

    uint8_t get_note() {
        return note;
    }

    void set_octave(int8_t oct) {
        octave = oct;
    }

    int8_t get_octave() {
        return octave;
    }

    void set_semi(int8_t s) {
        semi = s;
    }

    int8_t get_semi() {
        return semi;
    }

    // Call this after you set the note, octave or semi.
    void refresh_notes() {
        int16_t _note = (int16_t)note + semi + (octave * 12);

        if (_note > 127) {
            _note = 127;
        } else if (_note < 0) {
            _note = 0;
        }

        phase_inc = midi_phase_inc[_note];
        freq = phase_inc_to_freq_hz(phase_inc);
    }

    // It is not recommended to call this directly
    void set_freq(uint32_t f) {
        freq = f;
        phase_inc = freq_hz_to_phase_inc(f);
    }

    uint16_t get_freq() {
        return freq;
    }

    uint32_t get_phase_inc() {
        return phase_inc;
    }

    void set_freq_lfo(uint32_t rate_phase_inc, uint16_t depth) {
        freq_lfo_phase_inc = rate_phase_inc;
        set_freq_lfo_depth(depth);
    }

    void set_freq_lfo_rate(uint32_t rate_phase_inc) {
        freq_lfo_phase_inc = rate_phase_inc;
    }

    uint32_t get_freq_lfo_rate() {
        return freq_lfo_phase_inc;
    }

    void set_freq_lfo_depth(uint16_t depth) {
        if (depth > CC_MAX_FREQ_LFO_DEPTH) {
            depth = CC_MAX_FREQ_LFO_DEPTH;
        }

        freq_lfo_depth = depth;
    }

    uint16_t get_freq_lfo_depth() {
        return freq_lfo_depth;
    }

    void set_phase(uint32_t p) {
        phase = p;
    }

    uint32_t get_phase() {
        return phase;
    }

    void set_wave_type(wave_type_t type) {
        wave_type = type;
    }

    wave_type_t get_wave_type() {
        return wave_type;
    }

    void set_level(uint32_t l) {
        if (l > LEVEL_MAX) {
            l = LEVEL_MAX;
        }

        level = l;
    }

    void note_on() {
        env.note_on();
        phase = 0;
        freq_lfo_phase = 0;
    }

    void note_off() {
        env.note_off();
    }

    void set_adsr(uint32_t a, uint32_t d, uint16_t s, uint32_t r) {
        env.set_adsr(a, d, s, r);
    }

    void set_attack(uint32_t ms) {
        env.set_attack(ms);
    }

    void set_decay(uint32_t ms) {
        env.set_decay(ms);
    }

    void set_sustain(uint32_t s) {
        env.set_sustain(s);
    }

    void set_release(uint32_t ms) {
        env.set_release(ms);
    }

    int16_t process(int32_t pm) {
        uint32_t e = env.process();
        int32_t r = ((int32_t)wave_lookup(phase + pm) * (int32_t)e) >> 16;
        r = (r * level) >> 16;
        phase += current_phase_inc();
        freq_lfo_phase += freq_lfo_phase_inc;
        return r;
    }

    uint8_t is_active() {
        return env.is_active();
    }
};

class VOICE {
private:
    OP op[2];

    channel_alg_t alg = ALG_FM;

    uint32_t mod_index = 32768;

    uint8_t feedback = 0;

    int16_t mod_prev = 0;
    int16_t mod_last = 0;

    uint8_t active = 0;

    uint8_t velocity = 127;
    uint32_t velocity_gain = LEVEL_MAX;

    uint8_t note = 0;

public:
    OP *get_mod() {
        return &op[0];
    }

    OP *get_car() {
        return &op[1];
    }

    void set_alg(channel_alg_t a) {
        alg = a;
        mod_prev = 0;
        mod_last = 0;
    }

    channel_alg_t get_alg() {
        return alg;
    }

    void set_mod_index(uint32_t idx) {
        mod_index = idx;
    }

    uint32_t get_mod_index() {
        return mod_index;
    }

    void set_feedback(uint8_t fb) {
        if (fb > 7) {
            fb = 7;
        }

        feedback = fb;
    }

    uint8_t get_feedback() {
        return feedback;
    }

    void note_on(uint8_t n, uint8_t vel) {
        note = n;
        velocity = vel;
        velocity_gain = ((uint32_t)vel * LEVEL_MAX) / 127u;

        op[0].set_note(n);
        op[1].set_note(n);
        op[0].refresh_notes();
        op[1].refresh_notes();

        op[0].note_on();
        op[1].note_on();

        active = true;
    }

    void note_off() {
        op[0].note_off();
        op[1].note_off();
    }

    uint8_t get_note() {
        return note;
    }

    uint8_t is_active() {
        if (!active) {
            return 0;
        }

        if (!op[0].is_active() && !op[1].is_active()) {
            active = 0;
            return 0;
        }

        return 1;
    }

    void apply_patch(const patch_t *p)
    {
        set_alg(p->alg);

        mod_index = p->mod_index;
        set_feedback(p->feedback);

        for (uint32_t i = 0; i < 2; i++) {
            op[i].set_wave_type(
                p->op[i].wave
            );

            op[i].set_octave(
                p->op[i].octave
            );

            op[i].set_semi(
                p->op[i].semi
            );

            op[i].set_adsr(
                p->op[i].attack,
                p->op[i].decay,
                p->op[i].sustain,
                p->op[i].release
            );

            op[i].set_level(
                p->op[i].level
            );

            op[i].set_freq_lfo(
                freq_milli_hz_to_phase_inc(p->op[i].freq_lfo_rate_milli_hz),
                p->op[i].freq_lfo_depth
            );

        }
    }

    void set_op_wave(uint32_t idx, wave_type_t wave) {
        if (idx < 2) {
            op[idx].set_wave_type(wave);
        }
    }

    void set_op_level(uint32_t idx, uint32_t level) {
        if (idx < 2) {
            op[idx].set_level(level);
        }
    }

    void set_op_octave(uint32_t idx, int8_t octave) {
        if (idx < 2) {
            op[idx].set_octave(octave);
            op[idx].refresh_notes();
        }
    }

    void set_op_semi(uint32_t idx, int8_t semi) {
        if (idx < 2) {
            op[idx].set_semi(semi);
            op[idx].refresh_notes();
        }
    }

    void set_op_attack(uint32_t idx, uint32_t ms) {
        if (idx < 2) {
            op[idx].set_attack(ms);
        }
    }

    void set_op_decay(uint32_t idx, uint32_t ms) {
        if (idx < 2) {
            op[idx].set_decay(ms);
        }
    }

    void set_op_sustain(uint32_t idx, uint32_t sustain) {
        if (idx < 2) {
            op[idx].set_sustain(sustain);
        }
    }

    void set_op_release(uint32_t idx, uint32_t ms) {
        if (idx < 2) {
            op[idx].set_release(ms);
        }
    }

    void set_op_freq_lfo_rate(uint32_t idx, uint32_t rate_phase_inc) {
        if (idx < 2) {
            op[idx].set_freq_lfo_rate(rate_phase_inc);
        }
    }

    void set_op_freq_lfo_depth(uint32_t idx, uint16_t depth) {
        if (idx < 2) {
            op[idx].set_freq_lfo_depth(depth);
        }
    }

    void control_change(uint8_t cc, uint8_t value) {
        switch (cc) {
        case CC_VOICE_ALG:
            set_alg(cc_to_alg(value));
            break;

        case CC_VOICE_MOD_INDEX:
            set_mod_index(cc_to_level(value));
            break;

        case CC_VOICE_FEEDBACK:
            set_feedback(((uint32_t)value * 7u) / 127u);
            break;

        case CC_OP0_WAVE:
        case CC_OP1_WAVE:
            set_op_wave((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_wave(value));
            break;

        case CC_OP0_LEVEL:
        case CC_OP1_LEVEL:
            set_op_level((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_level(value));
            break;

        case CC_OP0_OCTAVE:
        case CC_OP1_OCTAVE:
            set_op_octave((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_octave(value));
            break;

        case CC_OP0_SEMI:
        case CC_OP1_SEMI:
            set_op_semi((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_semi(value));
            break;

        case CC_OP0_ATTACK:
        case CC_OP1_ATTACK:
            set_op_attack((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_env_ms(value));
            break;

        case CC_OP0_DECAY:
        case CC_OP1_DECAY:
            set_op_decay((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_env_ms(value));
            break;

        case CC_OP0_SUSTAIN:
        case CC_OP1_SUSTAIN:
            set_op_sustain((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_sustain(value));
            break;

        case CC_OP0_RELEASE:
        case CC_OP1_RELEASE:
            set_op_release((cc >= CC_OP1_WAVE) ? 1u : 0u, cc_to_env_ms(value));
            break;

        case CC_OP0_FREQ_LFO_RATE:
        case CC_OP1_FREQ_LFO_RATE:
            set_op_freq_lfo_rate(
                (cc >= CC_OP1_WAVE) ? 1u : 0u,
                cc_to_freq_lfo_rate_phase_inc(value)
            );
            break;

        case CC_OP0_FREQ_LFO_DEPTH:
        case CC_OP1_FREQ_LFO_DEPTH:
            set_op_freq_lfo_depth(
                (cc >= CC_OP1_WAVE) ? 1u : 0u,
                cc_to_freq_lfo_depth(value)
            );
            break;

        default:
            break;
        }
    }

    int16_t process() {
        if (!active) {
            return 0;
        }

        int32_t out = 0;

        switch (alg) {
        case ALG_FM: {
            int16_t mod = op[0].process(0);

            int32_t pm =
                ((int32_t)mod * (int32_t)mod_index);

            out = op[1].process(pm);
        }
        break;

        case ALG_FM_FB: {
            int32_t fb = 0;

            if (feedback) {
                fb =
                    ((int32_t)mod_prev +
                     (int32_t)mod_last) *
                    ((int32_t)feedback << 12);
            }

            int16_t mod = op[0].process(fb);

            mod_prev = mod_last;
            mod_last = mod;

            int32_t pm =
                ((int32_t)mod * (int32_t)mod_index);

            out = op[1].process(pm);
        }
        break;

        case ALG_ADD: {
            int32_t m = op[0].process(0);
            int32_t c = op[1].process(0);

            out = (m + c) >> 1;
        }
        break;
        }

        out = (out * (int32_t)velocity_gain) >> 16;
        return clamp16(out);
    }
};

#define MIDI_CHANNEL_COUNT 2
#define VOICES_PER_MIDI_CH 8

class MIDI_CHANNEL {
private:
    VOICE voice[VOICES_PER_MIDI_CH];

    patch_t patch;
    uint8_t program = 0;
    uint8_t volume = 127;
    uint8_t pan = 64;

    uint32_t channel_gain = LEVEL_MAX;

    VOICE *alloc_voice(uint8_t note) {
        for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
            if (voice[i].is_active() && voice[i].get_note() == note) {
                return &voice[i];
            }
        }

        for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
            if (!voice[i].is_active()) {
                return &voice[i];
            }
        }

        return &voice[0];
    }

public:
    MIDI_CHANNEL() {
        apply_patch(&patch_bank[0]);
    }

    void apply_patch(const patch_t *p) {
        patch = *p;

        for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
            voice[i].apply_patch(&patch);
        }
    }

    void set_program(uint8_t p) {
        if (p >= PATCH_BANK_SIZE) {
            p = PATCH_BANK_SIZE - 1;
        }

        program = p;
        apply_patch(&patch_bank[program]);
    }

    uint8_t get_program() {
        return program;
    }

    void set_volume(uint8_t v) {
        volume = v;
        channel_gain = ((uint32_t)v * LEVEL_MAX) / 127u;
    }

    uint8_t get_volume() {
        return volume;
    }

    void note_on(uint8_t note, uint8_t vel) {
        VOICE *v = alloc_voice(note);

        v->note_on(note, vel);
    }

    void note_off(uint8_t note) {
        for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
            if (voice[i].is_active() && voice[i].get_note() == note) {
                voice[i].note_off();
            }
        }
    }

    int16_t process() {
        int32_t mix = 0;

        for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
            mix += voice[i].process();
        }

        mix >>= 3;
        mix = (mix * (int32_t)channel_gain) >> 16;
        return mix;
    }

    void control_change(uint8_t cc, uint8_t value) {
        switch (cc) {
        case 7: // Channel Volume
            set_volume(value);
            break;

        default:
            for (uint32_t i = 0; i < VOICES_PER_MIDI_CH; i++) {
                voice[i].control_change(cc, value);
            }
            break;
        }
    }
};

class SYNTH {
private:
    MIDI_CHANNEL midi_ch[MIDI_CHANNEL_COUNT];

public:
    SYNTH() {}

    void note_on(uint8_t ch, uint8_t note, uint8_t vel) {
        if (ch >= MIDI_CHANNEL_COUNT) {
            return;
        }

        if (vel == 0) {
            note_off(ch, note);
            return;
        }

        midi_ch[ch].note_on(note, vel);
    }

    void note_off(uint8_t ch, uint8_t note) {
        if (ch >= MIDI_CHANNEL_COUNT) {
            return;
        }

        midi_ch[ch].note_off(note);
    }

    void program_change(uint8_t ch, uint8_t program) {
        if (ch >= MIDI_CHANNEL_COUNT) {
            return;
        }

        midi_ch[ch].set_program(program);
    }

    void control_change(uint8_t ch, uint8_t cc, uint8_t value) {
        if (ch >= MIDI_CHANNEL_COUNT) {
            return;
        }

        midi_ch[ch].control_change(cc, value);
    }

    int16_t process() {
        int32_t mix = 0;

        for (uint32_t i = 0; i < MIDI_CHANNEL_COUNT; i++) {
            mix += midi_ch[i].process();
        }

        // mix >>= 1;

        return mix;
    }

    uint32_t process_block(int16_t *buf, uint32_t num_samples) {
        uint32_t count = 0;

        for (uint32_t i = 0; i < num_samples; i++) {
            buf[i] = process();
            count++;
        }

        return count;
    }
};
