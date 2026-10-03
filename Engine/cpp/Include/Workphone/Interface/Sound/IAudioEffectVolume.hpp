#pragma once

#include <Workphone/Interface/Sound/IAudioEffect.hpp>

namespace workphone
{
    /**
     * @brief Interface for an audio effect that controls playback volume.
     *
     * This interface represents a simple audio effect which exposes
     * a volume (gain) control. Implementations apply the volume
     * multiplier to audio samples produced or processed by the effect.
     *
     * The meaning of the volume value depends on the concrete implementation,
     * but typical semantics are:
     * - 0.0  => silent
     * - 1.0  => original (unity) gain
     * - >1.0 => amplification
     *
     * @note Callers should consult the concrete effect implementation for
     *       any clamping or additional behaviour (for example, whether values
     *       are clamped to [0,1]).
     *
     * @see IAudioEffect
     */
    class WPCore_API IAudioEffectVolume : public IAudioEffect
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived implementations are correctly destroyed through
         * this interface pointer.
         */
        ~IAudioEffectVolume() override;

        /**
         * @brief Get the current volume (gain) applied by the effect.
         *
         * @return The current volume value as a floating point number.
         *         Typical semantics: 0.0 = silent, 1.0 = unity gain.
         */
        virtual f32 getVolume() const = 0;

        /**
         * @brief Set the volume (gain) for this effect.
         *
         * The exact handling of values outside a particular range is
         * implementation-defined (e.g. clamping, normalization, or allowing
         * amplification above 1.0). Prefer using values in the typical range
         * if portability is required.
         *
         * @param volume The volume/gain multiplier to apply.
         *               Typical values: 0.0 (silent) to 1.0 (original).
         */
        virtual void setVolume( f32 volume ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone
