#ifndef WORKPHONE_AIRCRAFT_CONTROLLER_INPUT_HPP
#define WORKPHONE_AIRCRAFT_CONTROLLER_INPUT_HPP

#include "CarControllerInput.hpp"

namespace workphone::scene::aircraft_input
{
    struct Sample
    {
        bool configured = false, deviceMatched = false;
        f32 throttle = 0, pitch = 0, roll = 0, yaw = 0;
    };

    inline const char *profilePrefix( const String &kind )
    {
        if( kind == "plane" ) return "Plane ";
        if( kind == "helicopter" ) return "Helicopter ";
        if( kind == "drone" ) return "Drone ";
        return nullptr;
    }

    inline bool booleanProperty( const SmartPtr<Properties> &profile, const String &key, bool &value )
    {
        if( !profile->hasProperty( key ) ) return true;
        const auto text = profile->getProperty( key );
        if( text == "true" || text == "1" ) value = true;
        else if( text == "false" || text == "0" ) value = false;
        else return false;
        return true;
    }

    inline bool floatProperty( const SmartPtr<Properties> &profile, const String &key,
                               f32 &value, f32 minimum, f32 maximum )
    {
        if( !profile->hasProperty( key ) ) return controller_input::finiteRange( value, minimum, maximum );
        const auto text = profile->getProperty( key );
        char *end = nullptr;
        const auto number = std::strtod( text.c_str(), &end );
        if( end == text.c_str() || *end != '\0' || !std::isfinite( number ) ||
            number < minimum || number > maximum ) return false;
        value = static_cast<f32>( number );
        return true;
    }

    inline Sample sample( const SmartPtr<Properties> &profile,
                          const Array<SmartPtr<IJoystick>> &devices, const String &kind )
    {
        Sample result;
        const auto prefix = profilePrefix( kind );
        result.configured = profile && prefix;
        if( !result.configured ) return result;
        const auto key = [prefix]( const char *name ) { return String( prefix ) + name; };
        try
        {
            bool enabled = false;
            if( !booleanProperty( profile, key( "Enabled" ), enabled ) || !enabled ) return result;
            s32 index = 0, expectedAxes = 0, expectedButtons = 0, expectedCount = 0;
            s32 pitchAxis = -1, rollAxis = -1, yawAxis = -1, throttleAxis = -1;
            if( !controller_input::integerProperty( profile, "Device Index", index, 0, 63 ) ||
                !controller_input::integerProperty( profile, "Device Axes", expectedAxes, 0, 8 ) ||
                !controller_input::integerProperty( profile, "Device Buttons", expectedButtons, 0, 128 ) ||
                !controller_input::integerProperty( profile, "Device Count", expectedCount, 0, 64 ) ||
                !controller_input::integerProperty( profile, key( "Pitch Axis" ).c_str(), pitchAxis, 0, 7 ) ||
                !controller_input::integerProperty( profile, key( "Roll Axis" ).c_str(), rollAxis, 0, 7 ) ||
                !controller_input::integerProperty( profile, key( "Yaw Axis" ).c_str(), yawAxis, 0, 7 ) ||
                !controller_input::integerProperty( profile, key( "Throttle Axis" ).c_str(), throttleAxis, 0, 7 ) )
                return result;
            const auto name = profile->getProperty( "Device Name" );
            String mode = "01";
            profile->getPropertyValue( key( "Throttle Mode" ), mode );
            f32 deadzone = 0.1f, sensitivity = 1;
            bool invertPitch = false, invertRoll = false, invertYaw = false, invertThrottle = false;
            if( !floatProperty( profile, key( "Deadzone" ), deadzone, 0, 0.95f ) ||
                !floatProperty( profile, key( "Sensitivity" ), sensitivity, 0.1f, 10 ) ||
                !booleanProperty( profile, key( "Invert Pitch" ), invertPitch ) ||
                !booleanProperty( profile, key( "Invert Roll" ), invertRoll ) ||
                !booleanProperty( profile, key( "Invert Yaw" ), invertYaw ) ||
                !booleanProperty( profile, key( "Invert Throttle" ), invertThrottle ) ||
                ( mode != "signed" && mode != "01" ) || name.empty() ||
                devices.size() != static_cast<size_t>( expectedCount ) ||
                static_cast<size_t>( index ) >= devices.size() ) return result;
            const auto &device = devices[index];
            if( !device || device->getName() != name || device->getNumAxes() != expectedAxes ||
                device->getNumButtons() != expectedButtons ) return result;
            for( auto axis : { pitchAxis, rollAxis, yawAxis, throttleAxis } )
                if( axis >= expectedAxes || axis >= IJoystick::MAX_AXES ) return result;
            const auto pitch = controller_input::rawAxis( device, pitchAxis );
            const auto roll = controller_input::rawAxis( device, rollAxis );
            const auto yaw = controller_input::rawAxis( device, yawAxis );
            auto throttle = controller_input::rawAxis( device, throttleAxis );
            if( !std::isfinite( pitch ) || !std::isfinite( roll ) || !std::isfinite( yaw ) ||
                !std::isfinite( throttle ) || device->getNumAxes() != expectedAxes ||
                device->getNumButtons() != expectedButtons ) return result;
            throttle = std::clamp( throttle, -1.0f, 1.0f );
            throttle = mode == "signed" ? ( throttle + 1 ) * 0.5f : std::max( throttle, 0.0f );
            if( invertThrottle ) throttle = 1 - throttle;
            result.deviceMatched = true;
            result.pitch = controller_input::steeringValue( pitch, deadzone, sensitivity, invertPitch );
            result.roll = controller_input::steeringValue( roll, deadzone, sensitivity, invertRoll );
            result.yaw = controller_input::steeringValue( yaw, deadzone, sensitivity, invertYaw );
            result.throttle = controller_input::pedalValue( throttle, deadzone );
        }
        catch( const std::exception & ) {}
        return result;
    }
}
#endif
