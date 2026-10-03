#pragma once

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @brief Interface for a buffer-based audio effect.
     *
     * Implementations process audio using separate input and output float buffers.
     * The host is responsible for providing valid input/output buffers and
     * configuring the buffer size and sample rate before calling `process()`.
     *
     * Notes:
     * - Buffers are owned by the host; implementations MUST NOT free or assume ownership.
     * - Implementations should avoid dynamic memory allocation in `process()` (real-time constraint).
     * - Callers are responsible for synchronization; implementations may assume single-threaded
     *   calls to `process()` unless otherwise documented by the host.
     */
    class WPCore_API IAudioEffect : public ISharedObject
    {
    public:
        /** @brief Virtual destructor. */
        ~IAudioEffect() override;

        /**
         * @brief Process audio from the input buffer to the output buffer.
         *
         * Implementations must read exactly `getNumSamples()` samples from `getInput()`
         * and write exactly `getNumSamples()` samples to `getOutput()`. If `getBypass()`
         * returns true, the implementation should bypass processing (typically by copying
         * input to output). Implementations should not allocate large amounts of memory
         * or perform blocking operations in this method.
         */
        virtual void process() = 0;

        /**
         * @brief Get pointer to the input buffer.
         * @return Pointer to the first sample of the input buffer, or nullptr if not set.
         *
         * The pointer refers to f32 samples (floats). The effect does not take ownership
         * of this pointer — the caller/host remains the owner and must ensure it remains valid.
         */
        virtual f32 *getInput() = 0;

        /**
         * @brief Set the input buffer pointer used by `process()`.
         * @param input Pointer to the first f32 sample of the input buffer.
         *
         * The effect will read from this buffer during `process()`. The caller retains ownership.
         */
        virtual void setInput( f32 *input ) = 0;

        /**
         * @brief Get pointer to the output buffer.
         * @return Pointer to the first sample of the output buffer, or nullptr if not set.
         *
         * The pointer refers to f32 samples (floats). The effect does not take ownership
         * of this pointer — the caller/host remains the owner and must ensure it remains valid.
         */
        virtual f32 *getOutput() = 0;

        /**
         * @brief Set the output buffer pointer used by `process()`.
         * @param output Pointer to the first f32 sample of the output buffer.
         *
         * The effect will write processed samples to this buffer. The caller retains ownership.
         */
        virtual void setOutput( f32 *output ) = 0;

        /**
         * @brief Get the number of samples to process on each `process()` call.
         * @return Number of samples (frames) to process.
         *
         * Clarify with the host whether this count is per-channel or total samples for multichannel
         * data.
         */
        virtual u32 getNumSamples() const = 0;

        /**
         * @brief Set the number of samples to process on each `process()` call.
         * @param numSamples Number of samples (frames).
         */
        virtual void setNumSamples( u32 numSamples ) = 0;

        /**
         * @brief Get whether the effect is bypassed.
         * @return true if bypass is enabled and processing should be skipped.
         *
         * When bypassed, `process()` should pass input to output unchanged (or otherwise avoid
         * modifying audio).
         */
        virtual bool getBypass() const = 0;

        /**
         * @brief Enable or disable bypass.
         * @param bypass true to bypass processing, false to enable processing.
         */
        virtual void setBypass( bool bypass ) = 0;

        /**
         * @brief Get the audio sample rate in Hertz.
         * @return Sample rate (e.g. 44100, 48000).
         *
         * Effects may require the sample rate to compute time-based parameters (delays, filters, etc.).
         */
        virtual u32 getSampleRate() const = 0;

        /**
         * @brief Set the audio sample rate in Hertz.
         * @param sampleRate Sample rate (e.g. 44100, 48000).
         */
        virtual void setSampleRate( u32 sampleRate ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone
