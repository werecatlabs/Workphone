#pragma once

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @brief Interface representing per-channel audio buffers for an audio bus.
     *
     * This interface provides accessors for the number of channels, silence flags
     * and arrays of per-channel sample buffers in either 32-bit (float) or
     * 64-bit (double) precision. Implementations own the actual buffer memory;
     * callers should not assume ownership unless explicitly documented by the
     * concrete implementation.
     *
     * Typical usage:
     * - Query number of channels via `getNumChannels()`.
     * - Retrieve per-channel buffer pointers via `getChannelBuffers32()` or
     *   `getChannelBuffers64()` depending on the sample precision in use.
     *
     * @note Silence flags are a bitmask where bit i corresponds to channel i
     *       (LSB = channel 0). A bit value of 1 indicates the corresponding
     *       channel is silent/should be treated as silence.
     */
    class WPCore_API IAudioBusBuffers : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Implementations should release any owned resources in their
         * destructor overrides.
         */
        ~IAudioBusBuffers() override;

        /**
         * @brief Get the number of audio channels.
         * @return Number of channels (>= 0).
         */
        virtual s32 getNumChannels() const = 0;

        /**
         * @brief Set the number of audio channels.
         *
         * Implementations may reallocate or adjust internal buffer arrays when
         * this value changes.
         *
         * @param numChannels New number of channels (>= 0).
         */
        virtual void setNumChannels( s32 numChannels ) = 0;

        /**
         * @brief Get the silence bitmask flags.
         *
         * Each bit corresponds to a channel: bit 0 -> channel 0, bit 1 -> channel 1, etc.
         *
         * @return Bitmask where a set bit indicates the channel is silent.
         */
        virtual u64 getSilenceFlags() const = 0;

        /**
         * @brief Set the silence bitmask flags.
         *
         * @param flags Bitmask where a set bit indicates the corresponding channel is silent.
         */
        virtual void setSilenceFlags( u64 flags ) = 0;

        /**
         * @brief Get array of per-channel 32-bit float buffers.
         *
         * The returned pointer is to an array of `f32*` of length equal to
         * `getNumChannels()`. Each `f32*` points to the sample data for that
         * channel. The implementation retains ownership of the memory.
         *
         * @return Pointer to an array of `f32*` channel buffers, or nullptr if none.
         */
        virtual f32 **getChannelBuffers32() const = 0;

        /**
         * @brief Set array of per-channel 32-bit float buffers.
         *
         * Caller should ensure the array length matches `getNumChannels()`.
         * Ownership semantics are implementation-defined; callers should not
         * free buffers unless the implementation's contract allows it.
         *
         * @param value Pointer to an array of `f32*` channel buffers.
         */
        virtual void setChannelBuffers32( f32 **value ) = 0;

        /**
         * @brief Get array of per-channel 64-bit double buffers.
         *
         * The returned pointer is to an array of `f64*` of length equal to
         * `getNumChannels()`. Each `f64*` points to the sample data for that
         * channel. The implementation retains ownership of the memory.
         *
         * @return Pointer to an array of `f64*` channel buffers, or nullptr if none.
         */
        virtual f64 **getChannelBuffers64() const = 0;

        /**
         * @brief Set array of per-channel 64-bit double buffers.
         *
         * Caller should ensure the array length matches `getNumChannels()`.
         * Ownership semantics are implementation-defined; callers should not
         * free buffers unless the implementation's contract allows it.
         *
         * @param value Pointer to an array of `f64*` channel buffers.
         */
        virtual void setChannelBuffers64( f64 **value ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone
