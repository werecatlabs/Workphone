#ifndef WORKPHONE_AUDIO_CORE_H
#define WORKPHONE_AUDIO_CORE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Additive, SDK-free API. All calls on one context must be serialized by its
 * control owner. render does not allocate, lock, perform IO or call user code.
 * Clips may be shared by contexts. Voices pin clips; clip_destroy relinquishes
 * the caller's reference. Release/destroy run on control owners, never render. */
typedef enum wp_audio_result {
    WP_AUDIO_OK, WP_AUDIO_INVALID_ARGUMENT, WP_AUDIO_INVALID_FORMAT,
    WP_AUDIO_UNSUPPORTED, WP_AUDIO_LIMIT, WP_AUDIO_OUT_OF_MEMORY,
    WP_AUDIO_STALE_HANDLE
} wp_audio_result;
typedef struct wp_audio_wav_info {
    uint32_t size; /* sizeof(wp_audio_wav_info) */
    uint32_t sample_rate;
    uint16_t channels, format_tag, bits_per_sample, block_align;
    size_t data_offset, data_bytes;
} wp_audio_wav_info;
/* Strict RIFF PCM 8/16/24/32 or IEEE float32, mono/stereo, <=64 MiB.
 * Validates the entire container including trailing chunks and odd padding. */
wp_audio_result wp_audio_wav_inspect(const void *bytes, size_t length, wp_audio_wav_info *info);
typedef struct wp_audio_clip wp_audio_clip;
wp_audio_result wp_audio_clip_decode(const void *bytes, size_t length, wp_audio_clip **clip);
void wp_audio_clip_destroy(wp_audio_clip *clip);
uint64_t wp_audio_clip_frames(const wp_audio_clip *clip);

typedef struct wp_audio_context wp_audio_context;
typedef struct wp_audio_voice_handle { uint64_t context; uint32_t slot, generation; } wp_audio_voice_handle;
typedef enum wp_audio_voice_state { WP_AUDIO_PENDING, WP_AUDIO_PLAYING,
    WP_AUDIO_PAUSED, WP_AUDIO_FINISHED, WP_AUDIO_STOPPED } wp_audio_voice_state;
typedef enum wp_audio_completion { WP_AUDIO_COMPLETION_NONE,
    WP_AUDIO_COMPLETION_NATURAL, WP_AUDIO_COMPLETION_STOPPED } wp_audio_completion;
typedef struct wp_audio_context_desc {
    uint32_t size, sample_rate, voice_capacity, max_block_frames;
} wp_audio_context_desc;
typedef struct wp_audio_voice_desc {
    uint32_t size;
    const wp_audio_clip *clip;
    uint64_t start_frame; /* absolute output clock; late requests start next render */
    float gain, pan, pitch; /* [0,1], [-1,1], [0.125,8] */
    int loop;
} wp_audio_voice_desc;
wp_audio_result wp_audio_context_create(const wp_audio_context_desc *desc, wp_audio_context **context);
void wp_audio_context_destroy(wp_audio_context *context);
uint64_t wp_audio_context_clock(const wp_audio_context *context);
wp_audio_result wp_audio_context_gain(wp_audio_context *context, float gain, int mute);
wp_audio_result wp_audio_voice_create(wp_audio_context *context, const wp_audio_voice_desc *desc,
                                     wp_audio_voice_handle *handle);
wp_audio_result wp_audio_voice_release(wp_audio_context *context, wp_audio_voice_handle handle);
wp_audio_result wp_audio_voice_pause(wp_audio_context *context, wp_audio_voice_handle handle, int pause);
wp_audio_result wp_audio_voice_stop(wp_audio_context *context, wp_audio_voice_handle handle);
wp_audio_result wp_audio_voice_restart(wp_audio_context *context, wp_audio_voice_handle handle);
wp_audio_result wp_audio_voice_seek(wp_audio_context *context, wp_audio_voice_handle handle, uint64_t frame);
wp_audio_result wp_audio_voice_gain(wp_audio_context *context, wp_audio_voice_handle handle, float gain);
wp_audio_result wp_audio_voice_loop(wp_audio_context *context, wp_audio_voice_handle handle, int loop);
wp_audio_result wp_audio_voice_status(wp_audio_context *context, wp_audio_voice_handle handle,
                                     wp_audio_voice_state *state, uint64_t *cursor);
/* Consume one terminal reason on control thread. Release does not emit an event. */
wp_audio_result wp_audio_voice_poll(wp_audio_context *context, wp_audio_voice_handle handle,
                                   wp_audio_completion *reason);
/* Writes interleaved stereo float, no limiter. Rejected calls do not advance clock. */
wp_audio_result wp_audio_render(wp_audio_context *context, float *output, uint32_t frames);
#ifdef __cplusplus
}
#endif
#endif
