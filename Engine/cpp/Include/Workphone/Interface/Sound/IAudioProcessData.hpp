#pragma once

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @brief Interface describing a single audio processing buffer/block of data.
     *
     * IAudioProcessData provides metadata and accessors for audio processing routines.
     * Implementations supply information about the processing mode, the sample format,
     * buffer sizes and the per-bus input/output buffers used during a processing callback.
     *
     * This interface intentionally remains lightweight and does not assume a specific
     * low-level audio API — it simply exposes the data required by higher-level audio
     * processing systems.
     */
    class WPCore_API IAudioProcessData : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures correct cleanup for derived implementations.
         */
        ~IAudioProcessData() override;

        /**
         * @brief Get the current audio processing mode.
         *
         * The processing mode is an implementation-defined integer that describes how
         * the audio should be processed (for example: realtime vs. offline, or other
         * mode flags understood by the audio subsystem).
         *
         * @return An integer representing the process mode.
         */
        virtual s32 getProcessMode() const = 0;

        /**
         * @brief Set the audio processing mode.
         *
         * @param processMode Implementation-defined mode flags describing how audio
         *                    should be processed for the current callback.
         */
        virtual void setProcessMode( s32 processMode ) = 0;

        /**
         * @brief Get the symbolic sample size.
         *
         * The symbolic sample size indicates the sample format in an abstract form
         * (for example: 16-bit integer, 24-bit, 32-bit float, etc.). The exact
         * enumeration values are defined by the audio subsystem that implements this
         * interface.
         *
         * @return An integer representing the symbolic sample size/format.
         */
        virtual s32 getSymbolicSampleSize() const = 0;

        /**
         * @brief Set the symbolic sample size/format.
         *
         * @param symbolicSampleSize Implementation-defined code describing the sample
         *                           representation used by the buffers.
         */
        virtual void setSymbolicSampleSize( s32 symbolicSampleSize ) = 0;

        /**
         * @brief Get the number of samples (frames) in each buffer.
         *
         * This value represents the number of audio frames (samples per channel)
         * that are available in the input and output buffers for the current
         * processing block.
         *
         * @return Number of samples (frames) per buffer.
         */
        virtual s32 getNumSamples() const = 0;

        /**
         * @brief Set the number of samples (frames) per buffer for the processing block.
         *
         * @param numSamples Number of samples (frames) in each buffer.
         */
        virtual void setNumSamples( s32 numSamples ) = 0;

        /**
         * @brief Get the number of input buses/channels groups.
         *
         * Input buses represent logical input groups (for example: stereo bus, mono bus).
         *
         * @return Number of input buses.
         */
        virtual s32 getNumInputs() const = 0;

        /**
         * @brief Set the number of input buses.
         *
         * @param numInputs Number of input buses present for the processing block.
         */
        virtual void setNumInputs( s32 numInputs ) = 0;

        /**
         * @brief Get the number of output buses/channels groups.
         *
         * Output buses represent logical output groups that will be written to by
         * the processing routine.
         *
         * @return Number of output buses.
         */
        virtual s32 getNumOutputs() const = 0;

        /**
         * @brief Set the number of output buses.
         *
         * @param numOutputs Number of output buses present for the processing block.
         */
        virtual void setNumOutputs( s32 numOutputs ) = 0;

        /**
         * @brief Retrieve the array of input bus buffers.
         *
         * Each element in the returned array references an `IAudioBusBuffers` instance
         * that contains the raw audio buffer(s) for a single input bus. The array
         * length should match `getNumInputs()`. Individual bus buffer semantics
         * (interleaved vs non-interleaved, channel ordering, etc.) are implementation-specific.
         *
         * @return Array of smart pointers to input `IAudioBusBuffers`.
         */
        virtual Array<SmartPtr<IAudioBusBuffers>> getInputs() const = 0;

        /**
         * @brief Retrieve the array of output bus buffers.
         *
         * Each element in the returned array references an `IAudioBusBuffers` instance
         * that the processing routine should write to for a single output bus. The
         * array length should match `getNumOutputs()`. Implementations decide buffer layout.
         *
         * @return Array of smart pointers to output `IAudioBusBuffers`.
         */
        virtual Array<SmartPtr<IAudioBusBuffers>> getOutputs() const = 0;

        /**
         * @brief Macro used to register the class with the engine's runtime/reflection system.
         *
         * Keeps a consistent declaration across interface types for RTTI, factory or
         * serialization systems used by the project.
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone
