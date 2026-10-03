#ifndef __WPSoundEventParam_h__
#define __WPSoundEventParam_h__

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/SoundEventParam.hpp>

namespace workphone
{
    /**
     * @class WPSoundEventParam
     * @brief Audio backend implementation of sound event parameters.
     *
     * WPSoundEventParam encapsulates a named parameter that can be used to control
     * various aspects of a sound event (e.g., RPM, speed, volume multipliers).
     * This implementation stores the parameter value locally and can be extended
     * to interface with platform-specific audio APIs (XAudio2, WASAPI, CoreAudio).
     */
    class WPSoundEventParam : public SoundEventParam
    {
    public:
        /**
         * @brief Construct a new WPSoundEventParam instance.
         */
        WPSoundEventParam();

        /**
         * @brief Destroy the WPSoundEventParam instance.
         */
        ~WPSoundEventParam() override;

        /**
         * @brief Get the current parameter value.
         * @return The current parameter value (typically in range 0.0 to 1.0 or custom range).
         */
        f32 getValue() const override;

        /**
         * @brief Set the parameter value.
         * @param value The new parameter value.
         */
        void setValue( f32 value ) override;

        /**
         * @brief Get the minimum value for this parameter.
         * @return The minimum value.
         */
        f32 getMinValue() const;

        /**
         * @brief Set the minimum value for this parameter.
         * @param minValue The minimum value.
         */
        void setMinValue( f32 minValue );

        /**
         * @brief Get the maximum value for this parameter.
         * @return The maximum value.
         */
        f32 getMaxValue() const;

        /**
         * @brief Set the maximum value for this parameter.
         * @param maxValue The maximum value.
         */
        void setMaxValue( f32 maxValue );

        /**
         * @brief Set the parameter value range.
         * @param minValue The minimum value.
         * @param maxValue The maximum value.
         */
        void setValueRange( f32 minValue, f32 maxValue );

        WP_CLASS_REGISTER_DECL;

    private:
        /// The current parameter value
        f32 m_value = 0.0f;

        /// The minimum allowed value for this parameter
        f32 m_minValue = 0.0f;

        /// The maximum allowed value for this parameter
        f32 m_maxValue = 1.0f;
    };

}  // namespace workphone

#endif  // __WPSoundEventParam_h__
