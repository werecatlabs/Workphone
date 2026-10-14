#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <WPProcedural/WPSkyAtmosphere.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CSkyAtmosphere : public ISkyAtmosphere
    {
    public:
        void setState( const SkyState &state ) override;
        const SkyState &getState() const override;
        void update() override;
        const SkyResults &getResults() const override;
        void computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear, real_Num latitude,
                                        Vector3<real_Num> &outSunDir,
                                        Vector3<real_Num> &outMoonDir ) override;
        Colour computeSkyColour( const Vector3<real_Num> &viewDir, const Vector3<real_Num> &sunDir,
                                 real_Num turbidity, real_Num sunAltitude ) override;
        Colour computeSunColour( real_Num sunAltitude, real_Num intensity ) override;
        Colour computeFogColour( real_Num sunAltitude ) override;
        WP_CLASS_REGISTER_DECL;

    private:
        WPSkyAtmosphere mImplementation;
    };
}  // namespace workphone::procedural
