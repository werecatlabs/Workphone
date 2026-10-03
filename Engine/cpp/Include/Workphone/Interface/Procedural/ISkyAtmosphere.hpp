#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/SkyAtmosphereTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API ISkyAtmosphere : public ISharedObject
    {
    public:
        ~ISkyAtmosphere() override;
        virtual void setState( const SkyState &state ) = 0;
        virtual const SkyState &getState() const = 0;
        virtual void update() = 0;
        virtual const SkyResults &getResults() const = 0;
        virtual void computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear,
                                                real_Num latitude, Vector3<real_Num> &outSunDir,
                                                Vector3<real_Num> &outMoonDir ) = 0;
        virtual Colour computeSkyColour( const Vector3<real_Num> &viewDir,
                                         const Vector3<real_Num> &sunDir, real_Num turbidity,
                                         real_Num sunAltitude ) = 0;
        virtual Colour computeSunColour( real_Num sunAltitude, real_Num intensity ) = 0;
        virtual Colour computeFogColour( real_Num sunAltitude ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
