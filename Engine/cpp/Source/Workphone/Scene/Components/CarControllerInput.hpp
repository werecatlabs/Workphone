#ifndef WORKPHONE_CAR_CONTROLLER_INPUT_HPP
#define WORKPHONE_CAR_CONTROLLER_INPUT_HPP

// Private input-profile sampling shared with deterministic contracts. Profiles
// are immutable snapshots published through IApplicationManager::playerSettings.
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Input/Joystick.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace workphone::scene::controller_input
{
    struct Keyboard
    {
        bool throttle = false, brake = false, left = false, right = false;
        bool active() const { return throttle || brake || left || right; }
    };

    struct Sample
    {
        bool configured = false, deviceMatched = false;
        f32 throttle = 0, brake = 0, steering = 0;
    };

    inline bool finiteRange( f32 value, f32 minimum, f32 maximum )
    {
        return std::isfinite( value ) && value >= minimum && value <= maximum;
    }

    inline f32 steeringValue( f32 value, f32 deadzone, f32 sensitivity, bool invert )
    {
        value = std::clamp( value, -1.0f, 1.0f );
        const auto magnitude = std::max( 0.0f, ( std::abs( value ) - deadzone ) / ( 1 - deadzone ) );
        const auto result = std::copysign( std::min( 1.0f, magnitude * sensitivity ), value );
        return invert ? -result : result;
    }

    inline f32 pedalValue( f32 value, f32 deadzone )
    {
        return std::clamp( ( value - deadzone ) / ( 1 - deadzone ), 0.0f, 1.0f );
    }

    inline f32 rawAxis( const SmartPtr<IJoystick> &joystick, s32 axis )
    {
        if( auto concrete = dynamic_cast<const Joystick *>( joystick.get() ) )
            return concrete->getRawAxis( axis );
        // Alternate backends that only implement IJoystick keep their existing
        // processed-axis contract until they expose their own raw device state.
        return joystick->getAxis( axis );
    }

    inline bool integerProperty( const SmartPtr<Properties> &profile, const char *key,
                                 s32 &value, s32 minimum, s32 maximum )
    {
        if( !profile->hasProperty( key ) ) return value >= minimum && value <= maximum;
        const auto text = profile->getProperty( key );
        char *end = nullptr;
        const auto number = std::strtod( text.c_str(), &end );
        if( end == text.c_str() || *end != '\0' || !std::isfinite( number ) ||
            number != std::trunc( number ) || number < minimum || number > maximum ) return false;
        value = static_cast<s32>( number );
        return true;
    }

    inline Sample sample( const SmartPtr<Properties> &profile,
                          const Array<SmartPtr<IJoystick>> &devices, Keyboard keyboard = {} )
    {
        Sample result;
        result.configured = profile != nullptr;
        if( !profile ) return result;
        // Keyboard remains available even with an invalid or disconnected
        // controller profile. Simultaneous opposing keys cancel steering.
        if( keyboard.active() )
        {
            result.throttle = keyboard.throttle ? 1.0f : 0.0f;
            result.brake = keyboard.brake ? 1.0f : 0.0f;
            result.steering = f32( keyboard.right ) - f32( keyboard.left );
            return result;
        }
        try
        {
            bool enabled = false;
            profile->getPropertyValue( "Enabled", enabled );
            if( !enabled ) return result;  // An explicit disabled profile is keyboard-only.

            s32 index = 0, expectedAxes = 0, expectedButtons = 0, expectedCount = 0;
            s32 steeringAxis = 0, throttleAxis = 5, brakeAxis = 2;
            String name, mode = "separate01";
            bool invertSteering = false, invertThrottle = false, invertBrake = false;
            f32 deadzone = 0.1f, sensitivity = 1.0f, pedalDeadzone = 0.02f;
            const auto validIntegers =
                integerProperty( profile, "Device Index", index, 0, 63 ) &&
                integerProperty( profile, "Device Axes", expectedAxes, 0, 8 ) &&
                integerProperty( profile, "Device Buttons", expectedButtons, 0, 128 ) &&
                integerProperty( profile, "Device Count", expectedCount, 0, 64 ) &&
                integerProperty( profile, "Steering Axis", steeringAxis, -1, 7 ) &&
                integerProperty( profile, "Throttle Axis", throttleAxis, -1, 7 ) &&
                integerProperty( profile, "Brake Axis", brakeAxis, -1, 7 );
            profile->getPropertyValue( "Device Name", name );
            profile->getPropertyValue( "Pedal Mode", mode );
            profile->getPropertyValue( "Invert Steering", invertSteering );
            profile->getPropertyValue( "Invert Throttle", invertThrottle );
            profile->getPropertyValue( "Invert Brake", invertBrake );
            profile->getPropertyValue( "Steering Deadzone", deadzone );
            profile->getPropertyValue( "Steering Sensitivity", sensitivity );
            profile->getPropertyValue( "Pedal Deadzone", pedalDeadzone );

            if( !validIntegers || index < 0 || index > 63 || expectedCount < 0 || expectedCount > 64 ||
                devices.size() != static_cast<size_t>( expectedCount ) ||
                static_cast<size_t>( index ) >= devices.size() || name.empty() ||
                expectedAxes < 0 || expectedAxes > 8 || expectedButtons < 0 || expectedButtons > 128 ||
                !finiteRange( deadzone, 0, 0.95f ) || !finiteRange( pedalDeadzone, 0, 0.95f ) ||
                !finiteRange( sensitivity, 0.1f, 10.0f ) ||
                ( mode != "separate01" && mode != "separateSigned" && mode != "combined" ) )
                return result;

            const auto &device = devices[index];
            if( !device || device->getName() != name || device->getNumAxes() != expectedAxes ||
                device->getNumButtons() != expectedButtons ) return result;
            const auto validAxis = [expectedAxes]( s32 axis ) {
                return axis >= 0 && axis < expectedAxes && axis < IJoystick::MAX_AXES;
            };
            if( !validAxis( steeringAxis ) || !validAxis( throttleAxis ) ||
                ( mode != "combined" && !validAxis( brakeAxis ) ) ) return result;

            auto steering = rawAxis( device, steeringAxis );
            auto throttle = rawAxis( device, throttleAxis );
            auto brake = mode == "combined" ? 0.0f : rawAxis( device, brakeAxis );
            if( !std::isfinite( steering ) || !std::isfinite( throttle ) || !std::isfinite( brake ) )
                return result;
            if( device->getNumAxes() != expectedAxes || device->getNumButtons() != expectedButtons )
                return result;
            steering = steeringValue( steering, deadzone, sensitivity, invertSteering );
            throttle = std::clamp( throttle, -1.0f, 1.0f );
            brake = std::clamp( brake, -1.0f, 1.0f );
            if( mode == "combined" )
            {
                if( invertThrottle ) throttle = -throttle;
                brake = std::max( -throttle, 0.0f );
                throttle = std::max( throttle, 0.0f );
            }
            else
            {
                if( mode == "separateSigned" )
                {
                    throttle = ( throttle + 1 ) * 0.5f;
                    brake = ( brake + 1 ) * 0.5f;
                }
                else
                {
                    throttle = std::max( throttle, 0.0f );
                    brake = std::max( brake, 0.0f );
                }
                if( invertThrottle ) throttle = 1 - throttle;
                if( invertBrake ) brake = 1 - brake;
            }
            result.deviceMatched = true;
            result.steering = steering;
            result.throttle = pedalValue( throttle, pedalDeadzone );
            result.brake = pedalValue( brake, pedalDeadzone );
        }
        catch( const std::exception & )
        {
            // A malformed published profile must never retain the last throttle.
        }
        return result;
    }
}
#endif
