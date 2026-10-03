// ============================================================================
// WPSkyAtmosphere.cpp - Implementation of the AAA sky atmosphere model
// ============================================================================
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPSkyAtmosphere.hpp"
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            const real_Num PI = 3.14159265358979323846f;
            const real_Num DEG_TO_RAD = PI / 180.0f;
            const real_Num RAD_TO_DEG = 180.0f / PI;

            // Standard Rayleigh scattering coefficients at sea level
            const Vector3<real_Num> BETA_R( 3.8e-6f, 1.35e-5f, 3.31e-5f );
            const Vector3<real_Num> BETA_M( 21e-6f, 21e-6f, 21e-6f );

            real_Num clamp01( real_Num v )
            {
                return v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
            }

            Vector3<real_Num> lerpV( const Vector3<real_Num> &a, const Vector3<real_Num> &b, real_Num t )
            {
                return Vector3<real_Num>( a.x + t * ( b.x - a.x ), a.y + t * ( b.y - a.y ),
                                          a.z + t * ( b.z - a.z ) );
            }
        }  // namespace

        WPSkyAtmosphere::WPSkyAtmosphere()
        {
        }

        WPSkyAtmosphere::~WPSkyAtmosphere()
        {
        }

        // ===================================================================
        // CELESTIAL POSITIONS - real spherical astronomy
        // ===================================================================
        void WPSkyAtmosphere::computeCelestialPositions( real_Num timeOfDay, real_Num dayOfYear,
                                                         real_Num latitude, Vector3<real_Num> &outSunDir,
                                                         Vector3<real_Num> &outMoonDir )
        {
            // Solar declination (simplified astronomical formula)
            real_Num decl =
                23.45f * std::sin( DEG_TO_RAD * ( 360.0f / 365.0f ) * ( dayOfYear - 81.0f ) );

            // Local hour angle - sun at solar noon is overhead (15 deg per hour)
            real_Num hourAngle = ( timeOfDay - 12.0f ) * 15.0f;

            real_Num latRad = DEG_TO_RAD * latitude;
            real_Num declRad = DEG_TO_RAD * decl;
            real_Num hourRad = DEG_TO_RAD * hourAngle;

            // Sun altitude and azimuth
            real_Num sinAlt = std::sin( latRad ) * std::sin( declRad ) +
                              std::cos( latRad ) * std::cos( declRad ) * std::cos( hourRad );
            real_Num altitude = std::asin( clamp01( sinAlt ) );
            real_Num cosAlt = std::cos( altitude );

            real_Num azimuth;
            if( cosAlt > 1e-4f )
            {
                real_Num sinAz = -std::cos( declRad ) * std::sin( hourRad ) / cosAlt;
                real_Num cosAz = ( std::sin( declRad ) - std::sin( altitude ) * std::sin( latRad ) ) /
                                 ( cosAlt * std::cos( latRad ) );
                azimuth = std::atan2( sinAz, cosAz );
            }
            else
            {
                azimuth = 0;
            }

            // Convert to direction vector (Y-up)
            real_Num cosAz = std::cos( azimuth );
            real_Num sinAz2 = std::sin( azimuth );
            outSunDir = Vector3<real_Num>( cosAz * cosAlt, std::sin( altitude ), sinAz2 * cosAlt );

            // Moon direction: 12-hour offset, opposite phase
            real_Num moonHourAngle = ( timeOfDay - 0.5f ) * 15.0f;
            real_Num mHourRad = DEG_TO_RAD * moonHourAngle;
            real_Num mSinAlt = std::sin( latRad ) * std::sin( declRad ) +
                               std::cos( latRad ) * std::cos( declRad ) * std::cos( mHourRad );
            real_Num mAlt = std::asin( clamp01( mSinAlt ) );
            real_Num mCosAlt = std::cos( mAlt );
            real_Num mAzimuth =
                std::atan2( -std::cos( declRad ) * std::sin( mHourRad ) / std::max( mCosAlt, 1e-4f ),
                            ( std::sin( declRad ) - std::sin( mAlt ) * std::sin( latRad ) ) /
                                std::max( mCosAlt * std::cos( latRad ), 1e-4f ) );

            outMoonDir = Vector3<real_Num>( std::cos( mAzimuth ) * mCosAlt, std::sin( mAlt ),
                                            std::sin( mAzimuth ) * mCosAlt );
        }

        // ===================================================================
        // SUN COLOUR - golden hour transitions
        // ===================================================================
        Colour WPSkyAtmosphere::computeSunColour( real_Num sunAltitude, real_Num intensity )
        {
            Vector3<real_Num> orange( 0.95f, 0.55f, 0.25f );
            Vector3<real_Num> yellow( 1.0f, 0.85f, 0.6f );
            Vector3<real_Num> white( 1.0f, 0.97f, 0.92f );
            Vector3<real_Num> twilight( 0.4f, 0.35f, 0.55f );

            // Zenith angle in degrees (0 = zenith, 90 = horizon, >90 = below)
            real_Num zenithAngle = RAD_TO_DEG * ( PI / 2.0f - sunAltitude );

            // Sun attenuation below horizon (atmospheric extinction)
            real_Num sunFactor =
                std::exp( -( 0.0087f + 0.0409f * std::sin( sunAltitude ) ) *
                          std::exp( -std::pow( ( zenithAngle - ( -20.0f ) ) / 9.0f, 2.0f ) ) * 11.0f );

            Vector3<real_Num> col;
            if( sunAltitude < 0.0f )
            {
                // After sunset - twilight
                col = lerpV( twilight, orange, std::max( 0.0f, sunAltitude + 0.4f ) * 2.5f );
            }
            else if( sunAltitude < DEG_TO_RAD * 15.0f )
            {
                // Golden hour: orange to yellow
                real_Num t = sunAltitude / DEG_TO_RAD / 15.0f;
                col = lerpV( orange, yellow, t );
            }
            else
            {
                // Day: yellow to white
                real_Num t = ( sunAltitude - DEG_TO_RAD * 15.0f ) / DEG_TO_RAD * 0.05f;
                col = lerpV( yellow, white, clamp01( t ) );
            }

            col = col * ( sunFactor * intensity );

            return Colour( col.x, col.y, col.z, 1.0f );
        }

        // ===================================================================
        // SKY COLOUR - Rayleigh/Mie scattering at a view direction
        // ===================================================================
        Colour WPSkyAtmosphere::computeSkyColour( const Vector3<real_Num> &viewDir,
                                                  const Vector3<real_Num> &sunDir, real_Num turbidity,
                                                  real_Num sunAltitude )
        {
            Vector3<real_Num> v = viewDir;
            v.normalise();
            Vector3<real_Num> s = sunDir;
            s.normalise();

            real_Num cosTheta = clamp01( v.dotProduct( s ) );
            real_Num zenithAngle = std::acos( clamp01( v.y ) );

            // Total scatter extinction at this angle
            real_Num Fex = std::exp( -( BETA_R.x * 8400.0f + BETA_M.x * 13200.0f ) *
                                     ( 1.0f - 0.7f * std::exp( -zenithAngle / 3.0f ) ) );

            // Rayleigh phase function
            real_Num rayleigh = 0.0596831f * ( 1.0f + cosTheta ) * ( 1.0f + cosTheta );

            // Mie (Henyey-Greenstein) with g=0.8
            real_Num g = 0.8f;
            real_Num gg = g * g;
            real_Num mie =
                ( 1.0f - gg ) / std::pow( 1.0f + gg - 2.0f * g * cosTheta, 1.5f ) * 0.0795774715f;

            // Base sky: blue from Rayleigh
            Vector3<real_Num> sky( 0.4f, 0.6f, 0.95f );
            sky = sky * rayleigh * BETA_R.x * 100.0f;

            // Sun-disc forward Mie peak
            sky = sky + Vector3<real_Num>( 1.0f, 0.95f, 0.8f ) * mie * BETA_M.x * 500.0f;

            // Golden-hour tinting
            real_Num sunHeight = sunDir.y;
            real_Num sunsetFactor = clamp01( ( 0.1f - sunHeight ) / 0.3f );
            sky = sky + Vector3<real_Num>( 1.0f, 0.4f, 0.1f ) * sunsetFactor * Fex * 0.6f;
            sky = sky + Vector3<real_Num>( 0.9f, 0.3f, 0.4f ) * sunsetFactor *
                            std::pow( std::max( 0.0f, cosTheta ), 20.0f ) * 0.3f;

            // Apply turbidity (more haze -> lighter sky)
            sky = sky * ( 1.0f + ( turbidity - 2.0f ) * 0.15f );

            // Clamp and tonemap
            sky = Vector3<real_Num>( std::pow( std::max( 0.0f, sky.x ), 0.45f ),
                                     std::pow( std::max( 0.0f, sky.y ), 0.45f ),
                                     std::pow( std::max( 0.0f, sky.z ), 0.45f ) );

            return Colour( sky.x, sky.y, sky.z, 1.0f );
        }

        // ===================================================================
        // FOG COLOUR
        // ===================================================================
        Colour WPSkyAtmosphere::computeFogColour( real_Num sunAltitude )
        {
            // Fog matches the horizon colour
            if( sunAltitude < DEG_TO_RAD * 5.0f )
            {
                return Colour( 0.45f, 0.35f, 0.30f, 1.0f );
            }
            else if( sunAltitude < DEG_TO_RAD * 25.0f )
            {
                return Colour( 0.75f, 0.65f, 0.55f, 1.0f );
            }
            return Colour( 0.75f, 0.78f, 0.80f, 1.0f );
        }

        // ===================================================================
        // UPDATE - recompute all derived values
        // ===================================================================
        void WPSkyAtmosphere::setState( const SkyState &state )
        {
            mState = state;
            update();
        }

        void WPSkyAtmosphere::update()
        {
            // Compute celestial positions
            computeCelestialPositions( mState.timeOfDay, mState.dayOfYear, mState.latitude,
                                       mResults.sunDir, mResults.moonDir );

            // Sun altitude in radians
            real_Num sunAlt = std::asin( clamp01( mResults.sunDir.y ) );
            mResults.sunAltitude = sunAlt;
            mResults.sunAzimuth = std::atan2( mResults.sunDir.z, mResults.sunDir.x );

            // Key intensity: sun if above horizon, else moon
            real_Num sunFactor = std::max( 0.0f, mResults.sunDir.y );
            real_Num moonFactor = std::max( 0.0f, mResults.moonDir.y );

            // Sun colour and intensity
            Colour sunCol = computeSunColour( sunAlt, mState.sunIntensity );
            mResults.keyColour = sunCol;

            // Moon colour - cooler, lower intensity
            Colour moonCol( 0.4f, 0.5f, 0.7f, 1.0f );
            moonCol.r *= moonFactor * 0.3f;
            moonCol.g *= moonFactor * 0.3f;
            moonCol.b *= moonFactor * 0.3f;

            // Pick the brighter of the two as key light
            real_Num sunKey = sunFactor * mState.sunIntensity;
            real_Num moonKey = moonFactor * 0.3f;
            if( moonKey > sunKey )
            {
                mResults.keyColour = moonCol;
                mResults.keyIntensity = moonKey;
            }
            else
            {
                mResults.keyIntensity = sunKey;
            }

            // Ambient/sky-fill colour: 15% of the beam (whole-sky diffuse)
            mResults.ambientColour = Colour( sunCol.r * 0.15f + 0.1f, sunCol.g * 0.15f + 0.1f,
                                             sunCol.b * 0.15f + 0.12f, 1.0f );

            // Sky colours at zenith and horizon
            mResults.skyColourTop = computeSkyColour( Vector3<real_Num>( 0, 1, 0 ), mResults.sunDir,
                                                      mState.turbidity, sunAlt );
            mResults.skyColourHorizon = computeSkyColour( Vector3<real_Num>( 1, 0, 0 ), mResults.sunDir,
                                                          mState.turbidity, sunAlt );

            // Exposure bias: dark scenes need more exposure to read
            real_Num beamAlive = sunFactor * 2.0f + 0.05f;  // treat moon light as weak
            real_Num altDeg = RAD_TO_DEG * sunAlt;
            mResults.exposureBias = 1.35f * ( 1.0f - clamp01( ( altDeg - 1.0f ) / 12.0f ) ) * beamAlive +
                                    0.55f * ( 1.0f - beamAlive );

            // Indirect scale: less at golden hour, more at night
            mResults.indirectScale = beamAlive > 0.1f ? lerpV( Vector3<real_Num>( 0.45f, 0.45f, 0.45f ),
                                                               Vector3<real_Num>( 1.0f, 1.0f, 1.0f ),
                                                               clamp01( altDeg / 14.0f ) )
                                                            .x
                                                      : 2.2f;

            // Fog
            mResults.fogColour = computeFogColour( sunAlt );
            mResults.fogDensity = ( mState.weather == Weather::Foggy )      ? 0.02f
                                  : ( mState.weather == Weather::Overcast ) ? 0.012f
                                                                            : 0.008f;
        }
    }  // namespace procedural
}  // namespace workphone
