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
    WP_AUDIO_STALE_HANDLE, WP_AUDIO_EMPTY
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

/* Optional bounded SPSC command publication: exactly one producer/poller and
 * one render owner. These two functions may run concurrently with render.
 * All other APIs still require exclusive access to the context. Queued start
 * returns a handle in its acknowledgement; queued release retires clip data on
 * the polling/control thread. Direct creation/release require quiescent render.
 * 128 requests may be outstanding, with eight slots reserved for stop commands.
 * Every accepted request produces one acknowledgement at a block boundary.
 * Poll acknowledgements to replenish capacity. No callbacks or reclamation run
 * in render. There is no implicit growth, blocking or silent command drop. */
typedef enum wp_audio_command_type {
    WP_AUDIO_COMMAND_GAIN, WP_AUDIO_COMMAND_LOOP, WP_AUDIO_COMMAND_PAUSE,
    WP_AUDIO_COMMAND_RESUME, WP_AUDIO_COMMAND_SEEK, WP_AUDIO_COMMAND_RESTART,
    WP_AUDIO_COMMAND_STOP, WP_AUDIO_COMMAND_STOP_ALL,
    WP_AUDIO_COMMAND_START, WP_AUDIO_COMMAND_RELEASE
} wp_audio_command_type;
typedef struct wp_audio_command {
    uint32_t size;
    wp_audio_command_type type;
    wp_audio_voice_handle voice;
    uint64_t request_id, frame;
    float value;
    wp_audio_voice_desc start; /* used only for START; clip pinned on admission */
} wp_audio_command;
typedef struct wp_audio_acknowledgement {
    uint64_t request_id;
    wp_audio_result result;
    wp_audio_voice_handle voice; /* created handle for START, original otherwise */
} wp_audio_acknowledgement;
wp_audio_result wp_audio_command_submit(wp_audio_context *context, const wp_audio_command *command);
wp_audio_result wp_audio_command_poll(wp_audio_context *context, wp_audio_acknowledgement *acknowledgement);
/* Process-wide native-core allocator instrumentation. Excludes platform,
 * adapter and C++ cache allocations. Read while owners are quiescent for an
 * exact leak balance; concurrent fields are independently sampled counters. */
typedef struct wp_audio_memory_stats {
    uint32_t size;
    uint64_t allocations, deallocations, resident_bytes;
} wp_audio_memory_stats;
wp_audio_result wp_audio_memory_snapshot(wp_audio_memory_stats *stats);
#ifdef __cplusplus
}
#endif
#endif
