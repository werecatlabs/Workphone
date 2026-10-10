#include "workphone_audio_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <stdatomic.h>
#endif

#define AUDIO_BYTES_LIMIT (64u * 1024u * 1024u)
static uint16_t le16(const unsigned char *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int finite_range(float v, float lo, float hi) { return isfinite(v) && v >= lo && v <= hi; }

wp_audio_result wp_audio_wav_inspect(const void *bytes, size_t length, wp_audio_wav_info *info) {
    const unsigned char *p = (const unsigned char *)bytes;
    size_t at = 12, end;
    int fmt = 0, data = 0;
    wp_audio_wav_info result;
    if (!p || !info || info->size != sizeof(*info)) return WP_AUDIO_INVALID_ARGUMENT;
    memset(&result, 0, sizeof(result)); result.size = sizeof(result);
    if (length > AUDIO_BYTES_LIMIT) return WP_AUDIO_LIMIT;
    if (length < 12 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return WP_AUDIO_INVALID_FORMAT;
    if (le32(p + 4) < 4 || (uint64_t)le32(p + 4) + 8 != length) return WP_AUDIO_INVALID_FORMAT;
    end = length;
    while (at < end) {
        uint32_t n;
        size_t payload;
        if (end - at < 8) return WP_AUDIO_INVALID_FORMAT;
        n = le32(p + at + 4); payload = at + 8;
        if ((uint64_t)n + (n & 1u) > end - payload) return WP_AUDIO_INVALID_FORMAT;
        if (!memcmp(p + at, "fmt ", 4)) {
            uint32_t expected;
            if (fmt++ || n < 16 || n == 17) return WP_AUDIO_INVALID_FORMAT;
            if (n > 18) return WP_AUDIO_UNSUPPORTED;
            if (n == 18 && le16(p + payload + 16) != 0) return WP_AUDIO_UNSUPPORTED;
            result.format_tag = le16(p + payload);
            result.channels = le16(p + payload + 2);
            result.sample_rate = le32(p + payload + 4);
            result.block_align = le16(p + payload + 12);
            result.bits_per_sample = le16(p + payload + 14);
            if (result.channels < 1 || result.channels > 2 || result.sample_rate < 8000 ||
                result.sample_rate > 192000) return WP_AUDIO_UNSUPPORTED;
            if (!((result.format_tag == 1 && (result.bits_per_sample == 8 || result.bits_per_sample == 16 ||
                 result.bits_per_sample == 24 || result.bits_per_sample == 32)) ||
                 (result.format_tag == 3 && result.bits_per_sample == 32))) return WP_AUDIO_UNSUPPORTED;
            expected = (uint32_t)result.channels * (result.bits_per_sample / 8);
            if (result.block_align != expected || le32(p + payload + 8) != result.sample_rate * expected)
                return WP_AUDIO_INVALID_FORMAT;
        } else if (!memcmp(p + at, "data", 4)) {
            if (data++ || !n) return WP_AUDIO_INVALID_FORMAT;
            result.data_offset = payload; result.data_bytes = n;
        }
        at = payload + n + (n & 1u);
    }
    if (!fmt || !data || result.data_bytes % result.block_align) return WP_AUDIO_INVALID_FORMAT;
    if (result.format_tag == 3) {
        size_t i;
        for (i = result.data_offset; i < result.data_offset + result.data_bytes; i += 4) {
            uint32_t bits = le32(p + i);
            float value;
            memcpy(&value, &bits, sizeof(value));
            if (!isfinite(value)) return WP_AUDIO_INVALID_FORMAT;
        }
    }
    *info = result;
    return WP_AUDIO_OK;
}

#ifdef _MSC_VER
typedef volatile long audio_ref;
static void ref_init(audio_ref *ref) { *ref = 1; }
static void ref_retain(audio_ref *ref) { _InterlockedIncrement(ref); }
static int ref_release(audio_ref *ref) { return _InterlockedDecrement(ref) == 0; }
#else
typedef atomic_uint audio_ref;
static void ref_init(audio_ref *ref) { atomic_init(ref, 1); }
static void ref_retain(audio_ref *ref) { atomic_fetch_add(ref, 1); }
static int ref_release(audio_ref *ref) { return atomic_fetch_sub(ref, 1) == 1; }
#endif
struct wp_audio_clip { float *samples; uint64_t frames; uint32_t rate, channels; audio_ref references; };
wp_audio_result wp_audio_clip_decode(const void *bytes, size_t length, wp_audio_clip **clip) {
    wp_audio_wav_info info;
    wp_audio_clip *c;
    size_t i, count, stride;
    const unsigned char *p;
    wp_audio_result result;
    if (!clip) return WP_AUDIO_INVALID_ARGUMENT;
    *clip = NULL; memset(&info, 0, sizeof(info)); info.size = sizeof(info);
    result = wp_audio_wav_inspect(bytes, length, &info);
    if (result != WP_AUDIO_OK) return result;
    stride = info.bits_per_sample / 8; count = info.data_bytes / stride;
    if (count > AUDIO_BYTES_LIMIT / sizeof(float)) return WP_AUDIO_LIMIT;
    c = (wp_audio_clip *)calloc(1, sizeof(*c));
    if (!c) return WP_AUDIO_OUT_OF_MEMORY;
    ref_init(&c->references);
    c->samples = (float *)malloc(count * sizeof(float));
    if (!c->samples) { free(c); return WP_AUDIO_OUT_OF_MEMORY; }
    c->frames = info.data_bytes / info.block_align; c->rate = info.sample_rate; c->channels = info.channels;
    p = (const unsigned char *)bytes + info.data_offset;
    for (i = 0; i < count; ++i, p += stride) {
        float v;
        if (info.format_tag == 3) {
            uint32_t bits = le32(p); memcpy(&v, &bits, sizeof(v));
            if (!isfinite(v)) { wp_audio_clip_destroy(c); return WP_AUDIO_INVALID_FORMAT; }
        } else if (stride == 1) v = ((float)p[0] - 128.0f) / 128.0f;
        else {
            uint32_t raw = stride == 2 ? le16(p) : stride == 3 ?
                ((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)) : le32(p);
            double signed_value = (double)raw;
            double range = stride == 2 ? 65536.0 : stride == 3 ? 16777216.0 : 4294967296.0;
            if (signed_value >= range / 2) signed_value -= range;
            v = (float)(signed_value / (range / 2));
        }
        c->samples[i] = v;
    }
    *clip = c; return WP_AUDIO_OK;
}
void wp_audio_clip_destroy(wp_audio_clip *c) { if (c && ref_release(&c->references)) { free(c->samples); free(c); } }
uint64_t wp_audio_clip_frames(const wp_audio_clip *c) { return c ? c->frames : 0; }

typedef struct audio_voice {
    const wp_audio_clip *clip;
    uint32_t generation;
    uint64_t start;
    double cursor;
    float gain, left, right, pitch;
    int used, loop;
    wp_audio_voice_state state, resume_state;
    wp_audio_completion completion;
} audio_voice;
struct wp_audio_context {
    uint64_t identity, clock;
    uint32_t rate, capacity, max_block;
    float gain;
    int mute;
    audio_voice *voices;
};
#ifdef _MSC_VER
static volatile __int64 next_context = 0;
static uint64_t context_identity(void) { return (uint64_t)_InterlockedIncrement64(&next_context); }
#else
static atomic_uint_fast64_t next_context = 1;
static uint64_t context_identity(void) { return atomic_fetch_add(&next_context, 1); }
#endif
wp_audio_result wp_audio_context_create(const wp_audio_context_desc *d, wp_audio_context **context) {
    wp_audio_context *c;
    if (!context) return WP_AUDIO_INVALID_ARGUMENT;
    *context = NULL;
    if (!d || d->size != sizeof(*d) || d->sample_rate < 8000 || d->sample_rate > 192000 ||
        !d->voice_capacity || d->voice_capacity > 1024 || !d->max_block_frames || d->max_block_frames > 8192)
        return WP_AUDIO_INVALID_ARGUMENT;
    c = (wp_audio_context *)calloc(1, sizeof(*c));
    if (!c) return WP_AUDIO_OUT_OF_MEMORY;
    c->voices = (audio_voice *)calloc(d->voice_capacity, sizeof(audio_voice));
    if (!c->voices) { free(c); return WP_AUDIO_OUT_OF_MEMORY; }
    c->identity = context_identity(); c->rate = d->sample_rate;
    c->capacity = d->voice_capacity; c->max_block = d->max_block_frames; c->gain = 1;
    *context = c; return WP_AUDIO_OK;
}
void wp_audio_context_destroy(wp_audio_context *c) {
    if (c) {
        uint32_t i;
        for (i = 0; i < c->capacity; ++i)
            if (c->voices[i].used) wp_audio_clip_destroy((wp_audio_clip *)c->voices[i].clip);
        free(c->voices); free(c);
    }
}
uint64_t wp_audio_context_clock(const wp_audio_context *c) { return c ? c->clock : 0; }
wp_audio_result wp_audio_context_gain(wp_audio_context *c, float gain, int mute) {
    if (!c || !finite_range(gain, 0, 1)) return WP_AUDIO_INVALID_ARGUMENT;
    c->gain = gain; c->mute = !!mute; return WP_AUDIO_OK;
}
static audio_voice *voice(wp_audio_context *c, wp_audio_voice_handle h) {
    if (!c || h.context != c->identity || h.slot >= c->capacity || !c->voices[h.slot].used ||
        c->voices[h.slot].generation != h.generation) return NULL;
    return c->voices + h.slot;
}
wp_audio_result wp_audio_voice_create(wp_audio_context *c, const wp_audio_voice_desc *d, wp_audio_voice_handle *h) {
    uint32_t i;
    if (!c || !d || !h || d->size != sizeof(*d) || !d->clip || !finite_range(d->gain, 0, 1) ||
        !finite_range(d->pan, -1, 1) || !finite_range(d->pitch, 0.125f, 8)) return WP_AUDIO_INVALID_ARGUMENT;
    memset(h, 0, sizeof(*h));
    for (i = 0; i < c->capacity; ++i) {
        audio_voice *v = c->voices + i;
        uint32_t generation;
        if (v->used || v->generation == UINT32_MAX) continue;
        generation = v->generation + 1; memset(v, 0, sizeof(*v)); v->generation = generation;
        v->used = 1; v->clip = d->clip; v->start = d->start_frame; v->gain = d->gain;
        ref_retain(&((wp_audio_clip *)d->clip)->references);
        /* Balance pan: center preserves authored level; hard pan mutes opposite side. */
        v->left = d->pan > 0 ? 1 - d->pan : 1; v->right = d->pan < 0 ? 1 + d->pan : 1;
        v->pitch = d->pitch; v->loop = !!d->loop; v->state = WP_AUDIO_PENDING;
        h->context = c->identity; h->slot = i; h->generation = generation;
        return WP_AUDIO_OK;
    }
    return WP_AUDIO_LIMIT;
}
wp_audio_result wp_audio_voice_release(wp_audio_context *c, wp_audio_voice_handle h) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    wp_audio_clip_destroy((wp_audio_clip *)v->clip);
    v->used = 0; v->clip = NULL; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_pause(wp_audio_context *c, wp_audio_voice_handle h, int pause) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (pause && (v->state == WP_AUDIO_PLAYING || v->state == WP_AUDIO_PENDING)) {
        v->resume_state = v->state; v->state = WP_AUDIO_PAUSED;
    } else if (!pause && v->state == WP_AUDIO_PAUSED) v->state = v->resume_state;
    return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_stop(wp_audio_context *c, wp_audio_voice_handle h) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (v->state != WP_AUDIO_STOPPED && v->state != WP_AUDIO_FINISHED) v->completion = WP_AUDIO_COMPLETION_STOPPED;
    v->state = WP_AUDIO_STOPPED; v->cursor = 0; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_restart(wp_audio_context *c, wp_audio_voice_handle h) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    v->cursor = 0; v->state = WP_AUDIO_PENDING; v->start = c->clock;
    v->completion = WP_AUDIO_COMPLETION_NONE; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_seek(wp_audio_context *c, wp_audio_voice_handle h, uint64_t frame) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (frame > v->clip->frames) return WP_AUDIO_INVALID_ARGUMENT;
    v->cursor = (double)frame; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_gain(wp_audio_context *c, wp_audio_voice_handle h, float gain) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (!finite_range(gain, 0, 1)) return WP_AUDIO_INVALID_ARGUMENT;
    v->gain = gain; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_loop(wp_audio_context *c, wp_audio_voice_handle h, int loop) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    v->loop = !!loop; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_status(wp_audio_context *c, wp_audio_voice_handle h, wp_audio_voice_state *state, uint64_t *cursor) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (!state || !cursor) return WP_AUDIO_INVALID_ARGUMENT;
    *state = v->state; *cursor = (uint64_t)v->cursor; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_voice_poll(wp_audio_context *c, wp_audio_voice_handle h, wp_audio_completion *reason) {
    audio_voice *v = voice(c, h); if (!v) return WP_AUDIO_STALE_HANDLE;
    if (!reason) return WP_AUDIO_INVALID_ARGUMENT;
    *reason = v->completion; v->completion = WP_AUDIO_COMPLETION_NONE; return WP_AUDIO_OK;
}
wp_audio_result wp_audio_render(wp_audio_context *c, float *out, uint32_t frames) {
    uint32_t i, f;
    if (!c || !out || frames > c->max_block || UINT64_MAX - c->clock < frames) return WP_AUDIO_INVALID_ARGUMENT;
    memset(out, 0, (size_t)frames * 2 * sizeof(float));
    for (i = 0; i < c->capacity; ++i) {
        audio_voice *v = c->voices + i;
        const wp_audio_clip *clip = v->clip;
        if (!v->used || (v->state != WP_AUDIO_PENDING && v->state != WP_AUDIO_PLAYING)) continue;
        for (f = 0; f < frames; ++f) {
            uint64_t a, b;
            float fraction, gain;
            uint32_t channel;
            if (c->clock + f < v->start) continue;
            v->state = WP_AUDIO_PLAYING;
            if (v->cursor >= (double)clip->frames) {
                if (v->loop) v->cursor = fmod(v->cursor, (double)clip->frames);
                else { v->state = WP_AUDIO_FINISHED; v->completion = WP_AUDIO_COMPLETION_NATURAL; break; }
            }
            a = (uint64_t)v->cursor; b = a + 1;
            if (b >= clip->frames) b = v->loop ? 0 : a;
            fraction = (float)(v->cursor - (double)a); gain = c->mute ? 0 : c->gain * v->gain;
            for (channel = 0; channel < 2; ++channel) {
                uint32_t source = clip->channels == 1 ? 0 : channel;
                float x = clip->samples[a * clip->channels + source];
                float y = clip->samples[b * clip->channels + source];
                out[(size_t)f * 2 + channel] += (x + (y - x) * fraction) * gain * (channel ? v->right : v->left);
            }
            v->cursor += (double)clip->rate / c->rate * v->pitch;
            if (v->loop && v->cursor >= (double)clip->frames)
                v->cursor = fmod(v->cursor, (double)clip->frames);
            if (!v->loop && v->cursor >= (double)clip->frames) {
                v->cursor = (double)clip->frames; v->state = WP_AUDIO_FINISHED;
                v->completion = WP_AUDIO_COMPLETION_NATURAL; break;
            }
        }
    }
    c->clock += frames; return WP_AUDIO_OK;
}
