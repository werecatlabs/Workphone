// ============================================================================
// WPSkyAtmosphere.hpp - AAA Procedural Sky and Atmosphere System
// ============================================================================
// Implements the Hillaire/Bruneton physically-based atmosphere model:
//   - Rayleigh scattering (wavelength-dependent)
//   - Mie scattering (forward-scattering)
//   - Ozone absorption
//   - Multiple scattering approximation
//   - Limb-darkened solar disc with analytic circumsolar aureole
//   - Real spherical astronomy for sun/moon positions
//   - Time-of-day colour transitions
//   - Volumetric fog parameters
//
// This is the C++ implementation that mirrors the Claude-of-Duty sky system
// while producing the same artistic and physical output.
// ============================================================================

#ifndef WPSkyAtmosphere_h__
#define WPSkyAtmosphere_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <cstdint>

#include <Workphone/Interface/Procedural/SkyAtmosphereTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPSkyAtmosphere
        {
        public:
            WPSkyAtmosphere();
            ~WPSkyAtmosphere();

            /// Set the current sky state (time of day, weather, location).
            void setState( const SkyState &state );
            const SkyState &getState() const
            {
                return mState;
            }

            /// Recompute all derived sky values for the current state.
            void update();

            /// Access the current results.
            const SkyResults &getResults() const
            {
                return mResults;
            }

            /// Compute sun/moon directions directly without state mutation.
            static void computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear,
                                                   real_Num latitude, Vector3<real_Num> &outSunDir,
                                                   Vector3<real_Num> &outMoonDir );

            /// Compute sky colour at a given view direction (sRGB).
            static Colour computeSkyColour( const Vector3<real_Num> &viewDir,
                                            const Vector3<real_Num> &sunDir, real_Num turbidity,
                                            real_Num sunAltitude );

            /// Compute sun colour with sunset/golden-hour transitions.
            static Colour computeSunColour( real_Num sunAltitude, real_Num intensity );

            /// Compute fog colour matching the time of day.
            static Colour computeFogColour( real_Num sunAltitude );

        private:
            SkyState mState;
            SkyResults mResults;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPSkyAtmosphere_h__
