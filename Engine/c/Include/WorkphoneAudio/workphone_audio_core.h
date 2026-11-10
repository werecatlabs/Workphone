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
    WP_AUDIO_STALE_HANDLE, WP_AUDIO_EMPTY, WP_AUDIO_DEVICE_ERROR
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
    WP_AUDIO_PAUSED, WP_AUDIO_FINISHED, WP_AUDIO_STOPPED, WP_AUDIO_FAILED } wp_audio_voice_state;
typedef enum wp_audio_completion { WP_AUDIO_COMPLETION_NONE,
    WP_AUDIO_COMPLETION_NATURAL, WP_AUDIO_COMPLETION_STOPPED,
    WP_AUDIO_COMPLETION_RELEASED, WP_AUDIO_COMPLETION_RESTARTED,
    WP_AUDIO_COMPLETION_DEVICE_FAILURE } wp_audio_completion;
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
/* Render-owner device failure. Seals command admission, cancels pending
 * requests with DEVICE_ERROR acknowledgements and terminalizes active runs
 * exactly once. Pins are reclaimed only by control polling/destruction.
 * An in-flight producer publication may finish after sealing; command_poll
 * cancels it after the render owner has finished failure handling. Render and
 * restart/create/submit remain failed until context recreation. Call this on
 * the render owner before it exits; never concurrently with another renderer. */
wp_audio_result wp_audio_context_fail(wp_audio_context *context);
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
/* Exclusive-owner legacy polling for a live handle. Release invalidates this
 * handle; use completion_poll for release notifications that outlive it. */
wp_audio_result wp_audio_voice_poll(wp_audio_context *context, wp_audio_voice_handle handle,
                                   wp_audio_completion *reason);
/* Optional terminal delivery for a concurrent control/render pair. Configure
 * while quiescent, before creating voices/submitting commands. Enabling reserves
 * one of 2048 notification credits for every accepted playback run; create and
 * restart return LIMIT when exhausted. Polling returns credits. Notifications
 * are never overwritten/dropped, even after release/reuse of a voice slot.
 * Each run emits once, including active release/restart; repeated stop/release
 * does not emit again. playback_id starts at 1 and advances on restart. frame is
 * the exclusive output end-frame (commands apply at the next block boundary).
 * Legacy voice_poll is exclusive-owner and independent of this event stream.
 * completion_poll may run concurrently with render on the sole control poller. */
typedef struct wp_audio_completion_event {
    uint32_t size;
    wp_audio_voice_handle voice;
    uint64_t playback_id, frame;
    wp_audio_completion reason;
} wp_audio_completion_event;
wp_audio_result wp_audio_context_notifications(wp_audio_context *context, int enabled);
wp_audio_result wp_audio_completion_poll(wp_audio_context *context, wp_audio_completion_event *event);
/* Writes interleaved stereo float, no limiter. Rejected calls do not advance clock. */
wp_audio_result wp_audio_render(wp_audio_context *context, float *output, uint32_t frames);

/* Optional bounded SPSC command publication: exactly one producer/poller and
 * one render owner. submit/poll, completion_poll and stats_snapshot may run
 * concurrently with render. Other APIs require exclusive context access. Queued start
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
    WP_AUDIO_COMMAND_START, WP_AUDIO_COMMAND_RELEASE,
    WP_AUDIO_COMMAND_MASTER_GAIN /* value = gain [0,1], frame = mute 0/1 */
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
/* Independently sampled atomic telemetry, safe with the command producer and
 * renderer. Rejections count capacity admission failures (not invalid arguments).
 * High-water marks are bounded. Not an atomic transaction across all fields. */
typedef struct wp_audio_context_stats {
    uint32_t size;
    uint64_t commands_accepted, commands_rejected, command_high_water;
    uint64_t completions_published, notification_rejections, notification_high_water;
} wp_audio_context_stats;
wp_audio_result wp_audio_context_stats_snapshot(wp_audio_context *context, wp_audio_context_stats *stats);
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
