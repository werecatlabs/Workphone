#pragma once

#include <Workphone/Interface/Sound/IAudioEffect.hpp>

namespace workphone
{
    /**
     * @brief Interface for a delay audio effect.
     *
     * This interface represents an audio delay effect that can be applied to audio channels.
     * Implementations are expected to manage internal delay buffers and processing state.
     *
     * Typical usage:
     * - Query or set the number of channels the effect should process via @c getNumChannels and @c
     * setNumChannels.
     * - The effect implementation should ensure its internal buffers and processing logic are
     * sized/initialized to handle the configured number of channels.
     *
     * @note Implementations should document thread-safety. By default, assume calls to configuration
     * methods occur on the audio thread or are otherwise synchronized by the caller.
     *
     * @ingroup Audio
     */
    class WPCore_API IAudioEffectDelay : public IAudioEffect
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived implementations are correctly destroyed through base pointers.
         */
        ~IAudioEffectDelay() override;

        /**
         * @brief Get the number of audio channels this delay effect processes.
         *
         * For example, a stereo effect would return 2.
         *
         * @return The number of channels currently configured for the effect.
         */
        virtual u32 getNumChannels() const = 0;

        /**
         * @brief Set the number of audio channels the effect should process.
         *
         * Implementations should resize or reallocate internal buffers as necessary
         * to support the requested channel count. Setting this value may invalidate
         * any previously held processing state.
         *
         * @param numChannels The desired number of channels (must be >= 1).
         */
        virtual void setNumChannels( u32 numChannels ) = 0;

        /**
         * @brief Macro used for runtime class registration.
         *
         * Implementations that use the engine's reflection/registration system
         * should leave this macro in place. The exact behaviour depends on the
         * WP_CLASS_REGISTER_DECL definition.
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone
