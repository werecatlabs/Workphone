#ifndef SoundEventParam_h__
#define SoundEventParam_h__

#include <Workphone/Interface/Sound/ISoundEventParam.hpp>

namespace workphone
{

    /**
     * @class SoundEventParam
     * @brief Represents a floating-point parameter associated with a sound event.
     *
     * This class implements the ISoundEventParam interface and is used to store
     * and manipulate numeric parameters that can influence the behavior
     * or characteristics of a sound event (e.g., pitch, cutoff frequency).
     */
    class WPCore_API SoundEventParam : public ISoundEventParam
    {
    public:
        /**
         * @brief Constructs a new SoundEventParam instance.
         */
        SoundEventParam();

        /**
         * @brief Destroys the SoundEventParam instance.
         */
        ~SoundEventParam() override;

        /**
         * @brief Retrieves the current value of the parameter.
         * @return The current parameter value.
         */
        f32 getValue() const override;

        /**
         * @brief Sets the value of the parameter.
         * @param value The new value to set.
         */
        void setValue( f32 value ) override;

    private:
        f32 m_value = 0.0f;  ///< The numeric value of the parameter.
    };

}  // namespace workphone

#endif  // SoundEventParam_h__
