#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Sound/IAudioEffectDelay.hpp>

namespace workphone
{
    /**
     * @class CAudioEffectDelay
     * @brief Concrete implementation of a delay audio effect.
     * 
     * This class provides the functionality to process audio samples and apply a 
     * delay effect based on the configured sample rate and delay time.
     */
    class CAudioEffectDelay : public IAudioEffectDelay
    {
    public:
        /** @brief Default constructor. */
        CAudioEffectDelay();
        /** @brief Destructor. */
        ~CAudioEffectDelay() override;

        /** 
         * @brief Processes the current audio buffer. 
         * 
         * Applies the delay effect to the input buffer and writes the result to the output buffer.
         */
        void process() override;

        /** @brief Gets the pointer to the input audio buffer. @return Pointer to the input buffer. */
        f32 *getInput() override;
        /** @brief Sets the pointer to the input audio buffer. @param input Pointer to the input buffer. */
        void setInput( f32 *input ) override;

        /** @brief Gets the pointer to the output audio buffer. @return Pointer to the output buffer. */
        f32 *getOutput() override;
        /** @brief Sets the pointer to the output audio buffer. @param output Pointer to the output buffer. */
        void setOutput( f32 *output ) override;

        /** @brief Gets the number of samples in the current buffer. @return The number of samples. */
        u32 getNumSamples() const override;
        /** @brief Sets the number of samples in the current buffer. @param numSamples The number of samples. */
        void setNumSamples( u32 numSamples ) override;

        /** @brief Checks if the effect is currently bypassed. @return True if bypassed, false otherwise. */
        bool getBypass() const override;
        /** @brief Sets whether the effect should be bypassed. @param bypass True to bypass, false to enable. */
        void setBypass( bool bypass ) override;

        /** @brief Gets the number of audio channels. @return The number of channels. */
        u32 getNumChannels() const override;
        /** @brief Sets the number of audio channels. @param numChannels The number of channels. */
        void setNumChannels( u32 numChannels ) override;

        /** @brief Gets the current audio sample rate. @return The sample rate in Hz. */
        u32 getSampleRate() const override;
        /** @brief Sets the audio sample rate. @param sampleRate The sample rate in Hz. */
        void setSampleRate( u32 sampleRate ) override;

    private:
        f32 *m_input = nullptr;
        f32 *m_output = nullptr;
        u32 m_numSamples = 0;
        u32 m_numChannels = 0;
        f32 m_delay = 0.0f;
        f32 **m_buffer = nullptr;
        s32 m_bufferPosition = 0;
        u32 m_sampleRate = 0;
        bool m_bBypass = false;
    };
}  // namespace workphone
