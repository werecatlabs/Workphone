#pragma once
#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Interface/Sound/IAudioEffectVolume.hpp>

namespace workphone
{
    /**
     * @class CAudioEffectVolume
     * @brief Implementation of a volume-based audio effect.
     * 
     * This class provides functionality to adjust the volume of an audio stream
     * by processing input samples and writing the scaled result to an output buffer.
     */
    class WPAudio_API CAudioEffectVolume : public IAudioEffectVolume
    {
    public:
        CAudioEffectVolume();
        ~CAudioEffectVolume() override;

        /**
         * @brief Processes the audio buffers by applying the current volume gain.
         * 
         * If bypass is enabled, the input samples are copied directly to the output.
         */
        void process() override;

        /** @brief Gets the current input audio buffer. */
        f32 *getInput() override;
        /** @brief Sets the input audio buffer to be processed. */
        void setInput( f32 *input ) override;

        /** @brief Gets the current output audio buffer. */
        f32 *getOutput() override;
        /** @brief Sets the output audio buffer where processed samples are written. */
        void setOutput( f32 *output ) override;

        /** @brief Gets the number of samples to process per block. */
        u32 getNumSamples() const override;
        /** @brief Sets the number of samples to process per block. */
        void setNumSamples( u32 numSamples ) override;

        /** @brief Checks if the effect is currently bypassed. */
        bool getBypass() const override;
        /** @brief Enables or disables the bypass mode. */
        void setBypass( bool bypass ) override;

        /** @brief Gets the current volume gain factor. */
        f32 getVolume() const override;
        /** @brief Sets the volume gain factor (typically 0.0f to 1.0f). */
        void setVolume( f32 volume ) override;

        /** @brief Gets the current audio sample rate. */
        u32 getSampleRate() const override;
        /** @brief Sets the audio sample rate. */
        void setSampleRate( u32 sampleRate ) override;

    protected:
        f32 *m_input = nullptr;     ///< Pointer to the input sample buffer.
        f32 *m_output = nullptr;    ///< Pointer to the output sample buffer.
        u32 m_numSamples = 0;       ///< Number of samples per processing block.
        u32 m_sampleRate = 0;       ///< Audio sample rate in Hz.
        f32 m_volume = 0.0f;        ///< Volume gain multiplier.
        bool m_bypass = false;      ///< Bypass flag; if true, processing is skipped.
    };
}  // namespace workphone
