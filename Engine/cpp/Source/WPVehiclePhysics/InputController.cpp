#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/InputController.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    InputController::InputController() : m_channelId(0)
    {
    }

    InputController::~InputController()
    {
    }

    s32 InputController::getChannelId() const
    {
        return m_channelId;
    }

    void InputController::setChannelId(s32 channelId)
    {
        m_channelId = channelId;
    }

    String InputController::getAxisName() const
    {
        return m_axisName;
    }

    void InputController::setAxisName(const String &axisName)
    {
        m_axisName = axisName;
    }

    String InputController::getButtonName() const
    {
        return m_buttonName;
    }

    void InputController::setButtonName(const String &buttonName)
    {
        m_buttonName = buttonName;
    }

    f32 InputController::getAxisInput() const
    {
        if(m_axisName == "Throttle")
        {
            return m_parentAircraft->getChannel(CAircraft::m_thrChannel);
        }
        if(m_axisName == "Roll")
        {
            return m_parentAircraft->getChannel(CAircraft::m_ailChannel) * 3.0f;
        }
        if(m_axisName == "Pitch")
        {
            return m_parentAircraft->getChannel(CAircraft::m_eleChannel);
        }
        if(m_axisName == "Yaw")
        {
            return m_parentAircraft->getChannel(CAircraft::m_yawChannel);
        }

        return 0.0f;
    }

    SmartPtr<IAircraft> InputController::getParentAircraft() const
    {
        return m_parentAircraft;
    }

    void InputController::setParentAircraft(SmartPtr<IAircraft> parentAircraft)
    {
        m_parentAircraft = parentAircraft;
    }
}
