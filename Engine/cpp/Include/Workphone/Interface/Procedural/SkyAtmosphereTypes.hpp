#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <cstdint>

namespace workphone
{
    namespace procedural
    {
        using Colour = ColourF;
        enum class WPCore_API Weather : u8
        {
            Clear = 0,
            PartlyCloudy,
            Overcast,
            Foggy,
            Stormy
        };

        /// Real-time state of the sky.
        struct SkyState
        {
            real_Num timeOfDay = 14.0f;    // 0..24 hours
            real_Num dayOfYear = 172.0f;   // 0..365
            real_Num latitude = 32.0f;     // degrees
            real_Num turbidity = 2.0f;     // haze
            real_Num sunIntensity = 1.5f;  // multiplier
            Weather weather = Weather::Clear;
        };

        /// Computed sky parameters at a given time.
        struct SkyResults
        {
            Vector3<real_Num> sunDir;   // unit vector pointing AT the sun
            Vector3<real_Num> moonDir;  // pointing at the moon
            real_Num sunAltitude = 0;   // radians above horizon
            real_Num sunAzimuth = 0;    // radians
            real_Num keyIntensity = 0;  // sun or moon brightness
            Colour keyColour;           // sun/moon colour
            Colour ambientColour;       // hemisphere ambient
            Colour skyColourTop;        // top of sky
            Colour skyColourHorizon;    // horizon colour
            real_Num exposureBias = 1.0f;
            real_Num indirectScale = 1.0f;
            Colour fogColour;
            real_Num fogDensity = 0.008f;
        };

    }  // namespace procedural
}  // namespace workphone
