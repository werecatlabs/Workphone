#ifndef InputController_h__
#define InputController_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API InputController : public ISharedObject
    {
    public:
        /**
             * @brief Lightweight adapter representing an input channel mapped to
             *        an aircraft control axis or button.
             *
             * InputController stores identification metadata (channel id,
             * axis/button names) and a weak/owned reference to the aircraft
             * instance it controls. It exposes simple getters/setters used by
             * the input handling layer to query the current axis value.
             */
        InputController();
        /// Virtual destructor required by ISharedObject
        ~InputController() override;

        /**
             * @brief Get the numeric channel identifier for this controller.
             * @return Channel id (platform/input-system specific).
             */
        s32 getChannelId() const;
        /**
             * @brief Set the numeric channel identifier used by the input system.
             */
        void setChannelId(s32 channelId);

        /**
             * @brief Get the mapped axis name (e.g., "Collective") for this
             *        controller.
             */
        String getAxisName() const;
        /**
             * @brief Assign the axis name for display or mapping purposes.
             */
        void setAxisName(const String &axisName);

        /**
             * @brief Get the mapped button name (if this controller represents
             *        a button input rather than an analog axis).
             */
        String getButtonName() const;
        /**
             * @brief Assign the button name used for mapping or display.
             */
        void setButtonName(const String &buttonName);

        /**
             * @brief Retrieve the current analog axis input value for this
             *        controller. Values are typically in the range [-1, 1].
             * @return The current axis value as a float.
             */
        f32 getAxisInput() const;

        /**
             * @brief Get the aircraft instance associated with this controller.
             * @return SmartPtr to the parent IAircraft (may be null).
             */
        SmartPtr<IAircraft> getParentAircraft() const;
        /**
             * @brief Associate this controller with an aircraft instance.
             * @param parentAircraft SmartPtr to the aircraft to control.
             */
        void setParentAircraft(SmartPtr<IAircraft> parentAircraft);

    protected:
        s32 m_channelId; ///< Numeric channel id from input system
        String m_axisName; ///< Mapped axis identifier for this controller
        String m_buttonName; ///< Mapped button identifier (if applicable)
        SmartPtr<IAircraft> m_parentAircraft; ///< Reference to the aircraft controlled
    };
}

#endif // InputController_h__
