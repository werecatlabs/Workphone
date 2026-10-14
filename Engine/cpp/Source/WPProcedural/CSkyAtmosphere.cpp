#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CSkyAtmosphere.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CSkyAtmosphere, ISkyAtmosphere );

    void CSkyAtmosphere::setState( const SkyState &state )
    {
        mImplementation.setState( state );
    }

    const SkyState &CSkyAtmosphere::getState() const
    {
        return mImplementation.getState();
    }

    void CSkyAtmosphere::update()
    {
        mImplementation.update();
    }

    const SkyResults &CSkyAtmosphere::getResults() const
    {
        return mImplementation.getResults();
    }

    void CSkyAtmosphere::computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear,
                                                    real_Num latitude, Vector3<real_Num> &outSunDir,
                                                    Vector3<real_Num> &outMoonDir )
    {
        WPSkyAtmosphere::computeCelestialPositions( timeOfDay, dayOfYear, latitude, outSunDir,
                                                    outMoonDir );
    }

    Colour CSkyAtmosphere::computeSkyColour( const Vector3<real_Num> &viewDir,
                                             const Vector3<real_Num> &sunDir, real_Num turbidity,
                                             real_Num sunAltitude )
    {
        return WPSkyAtmosphere::computeSkyColour( viewDir, sunDir, turbidity, sunAltitude );
    }

    Colour CSkyAtmosphere::computeSunColour( real_Num sunAltitude, real_Num intensity )
    {
        return WPSkyAtmosphere::computeSunColour( sunAltitude, intensity );
    }

    Colour CSkyAtmosphere::computeFogColour( real_Num sunAltitude )
    {
        return WPSkyAtmosphere::computeFogColour( sunAltitude );
    }
}  // namespace workphone::procedural
