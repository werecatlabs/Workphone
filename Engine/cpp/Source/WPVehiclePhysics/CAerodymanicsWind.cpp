#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAerodymanicsWind.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    namespace vehicle
    {
        const real_dNum CAerodymanicsWind::KvonKarmen = 0.41; // VonKarmen's constant

        CAerodymanicsWind::CAerodymanicsWind()
        {
            initWeather();
        }

        CAerodymanicsWind::~CAerodymanicsWind()
        {
        }

        void CAerodymanicsWind::setWind( real_Num Speed, real_Num Direction, real_Num Turb,
                                         real_Num GndHt, real_Num DirOff, real_Num Rough ) /* export */
        {
            m_meanWindSpeed = Speed;
            m_meanWindDirection = Direction;
            m_turbulence = static_cast<real_Num>( 0.3 ) * Turb;
            // note the multiplication by a scaling factor here to make the user turb. adjustment have
            // desired range
            m_groundHeight = GndHt;
            m_directionOffset = DirOff;
            m_roughness = Rough;
            m_convXFactor = -Math<real_Num>::Cos( ( Direction - m_directionOffset ) /
                                                  static_cast<real_Num>( 57.296 ) );
            // holds the fraction of the current wind in the Conventional vector X direction
            m_convYFactor = -Math<real_Num>::Sin( ( Direction - m_directionOffset ) /
                                                  static_cast<real_Num>( 57.296 ) );
            // holds the fraction of the current wind in the Conventional vector Y direction
            m_convCrossX =
                -Math<real_Num>::Cos( ( static_cast<real_Num>( 90 ) + Direction - m_directionOffset ) /
                                      static_cast<real_Num>( 57.296 ) );
            // these Cross wind components acct in a direction 90 degrees to the right of the mean wind
            m_convCrossY =
                -Math<real_Num>::Sin( ( static_cast<real_Num>( 90 ) + Direction - m_directionOffset ) /
                                      static_cast<real_Num>( 57.296 ) );
            m_uStar = ( m_meanWindSpeed * KvonKarmen ) / Math<real_Num>::Ln( 10.0f / m_roughness );
            // calculate UStar

            // set the angular frequency for the windspeed spectral components
            m_omega[1] = 1 * ( 2 * Math<real_Num>::pi() / 60 );     // 1 cycle/ minute  component
            m_omega[2] = 1.2 * ( 2 * Math<real_Num>::pi() / 60 );   // 1.2 cycle/minute
            m_omega[3] = 1.5 * ( 2 * Math<real_Num>::pi() / 60 );   // 1.5 cycle/minute
            m_omega[4] = 1.8 * ( 2 * Math<real_Num>::pi() / 60 );   // 1.8 cycle/minute
            m_omega[5] = 2.2 * ( 2 * Math<real_Num>::pi() / 60 );   // 2.2 cycle/minute
            m_omega[6] = 2.7 * ( 2 * Math<real_Num>::pi() / 60 );   // 2.7 cycle/minute
            m_omega[7] = 3.3 * ( 2 * Math<real_Num>::pi() / 60 );   // 3.3 cycle/minute
            m_omega[8] = 3.9 * ( 2 * Math<real_Num>::pi() / 60 );   // 3.9 cycle/minute
            m_omega[9] = 4.7 * ( 2 * Math<real_Num>::pi() / 60 );   // 4.7 cycle/minute
            m_omega[10] = 5.6 * ( 2 * Math<real_Num>::pi() / 60 );  // 5.6 cycle/minute
            m_omega[11] = 6.8 * ( 2 * Math<real_Num>::pi() / 60 );  // 6.8 cycle/ minute  component
            m_omega[12] = 8.2 * ( 2 * Math<real_Num>::pi() / 60 );  // 8.2 cycle/minute
            m_omega[13] = 10.0 * ( 2 * Math<real_Num>::pi() / 60 ); // 10 cycle/minute
            m_omega[14] = 12.0 * ( 2 * Math<real_Num>::pi() / 60 ); // 12 cycle/minute
            m_omega[15] = 15.0 * ( 2 * Math<real_Num>::pi() / 60 ); // 15 cycle/minute

            // set the amplitudes for those spectral components
            m_atude[1] = static_cast<real_Num>( 0.20 );
            m_atude[2] = static_cast<real_Num>( 0.30 );
            m_atude[3] = static_cast<real_Num>( 0.50 );
            m_atude[4] = static_cast<real_Num>( 0.70 );
            m_atude[5] = static_cast<real_Num>( 0.95 );
            m_atude[6] = static_cast<real_Num>( 0.95 );
            m_atude[7] = static_cast<real_Num>( 0.70 );
            m_atude[8] = static_cast<real_Num>( 0.50 );
            m_atude[9] = static_cast<real_Num>( 0.30 );
            m_atude[10] = static_cast<real_Num>( 0.20 );
            m_atude[11] = static_cast<real_Num>( 0.15 );
            m_atude[12] = static_cast<real_Num>( 0.12 );
            m_atude[13] = static_cast<real_Num>( 0.11 );
            m_atude[14] = static_cast<real_Num>( 0.9 );
            m_atude[15] = static_cast<real_Num>( 0.8 );
        }

        void CAerodymanicsWind::calcTurbulence( real_Num Time )
        {
            s32      F;
            real_Num Phi1, Phi2;
            if( Math<real_Num>::Abs( Time - m_turbTime ) > static_cast<real_Num>( 0.1 ) )
            // check if at least 0.5 seconds has elapsed since last turbulence calc done
            {
                m_turbTime = Time;
                m_kTurb = 0; // zero the turbulene values ready to sum the components
                m_kCrossWind = 0;

                for( F = 1; F != 15; F++ ) // for each of the frequencies in the spectrum
                {
                    Phi1 = ( Time - m_phaseTime1 ) * m_omega[F];
                    // add in the random offsets so as to unsync the sine waves
                    Phi2 = ( Time - m_phaseTime2 ) * m_omega[F];
                    m_kTurb = m_kTurb + m_atude[F] * Math<real_Num>::Sin( Phi1 );
                    // sum the 10 frequency components in the wind spectrum
                    m_kCrossWind = m_kCrossWind + m_atude[F] * Math<real_Num>::Sin( Phi2 );
                    // add the components with a phase shift
                } // end of the For F loop

                m_kCrossWind = static_cast<real_Num>( 0.5 ) * m_kCrossWind * m_turbulence;
                // apply the turbulance strength factor to the cross wind element and factor 0.5 to limit
                // direction shift
                m_kTurb = m_kTurb * m_turbulence;                 // apply the gain factor turbulence
                m_kTurb = m_kTurb + static_cast<real_Num>( 1.0 ); // add in 1 to make the mean value = 1
            }
        }

        Vector3<real_Num> CAerodymanicsWind::getWind( real_Num Height, real_Num Time )
        // allows dll to get the current wind. Returns a ConventionalFrameVector
        {
            real_Num ScaleHt;
            m_modelHeight = Height;
            calcTurbulence( Time );

            // change to using log profile Vz = (m_uStar/KvonKarmen) * Ln(Z/RoughnessLength)
            ScaleHt = ( m_modelHeight - m_groundHeight ) / m_roughness;
            // the height as a multiple of the roughness length

            real_Num LogWind; // the MeanWind at the models height

            if( ScaleHt > static_cast<real_Num>( 1.0 ) )
                LogWind = ( m_uStar / KvonKarmen ) * Math<real_Num>::Ln( ScaleHt );
            else
                LogWind = static_cast<real_Num>( 0.0 ); // calculate the Mean wind at this height

            Vector3<real_Num> result;
            result.X() = LogWind * ( m_kTurb * m_convXFactor + m_kCrossWind * m_convCrossX );
            // apply the turbulance and direction fractions for
            result.Z() = LogWind * ( m_kTurb * m_convYFactor + m_kCrossWind * m_convCrossY );
            result.Y() = 0; // make vertical component zero
            return result;
        }

        Vector3<real_Num> CAerodymanicsWind::getWindY( real_Num Height, real_Num Time )
        // allows the heli dll to get the current wind in a Saracen frame vector  (Y vertical)

        {
            Vector3<real_Num> result;
            Vector3<real_Num> TempConWind;
            TempConWind = getWind( Height, Time ); // use the getWind routine to do the wind calc
            // result = VecToSaracen(TempConWind); //convert the resulting to a Saracen vector for the
            // heli sim
            return result;
        }

        // note to save this passing the time into this procedure we use the CurrentWindSpeed as
        // calculated at the last getWind call for the model

        void CAerodymanicsWind::passWind( real_Num Height, real_Num &WindX, real_Num &WindY,
                                          real_Num &WindZ )
        /* export */ // note this sends a Saracen vector out the hard way!
        {
            real_Num LogWind; // the MeanWind at the specified height

            real_Num          ScaleHt;
            Vector3<real_Num> ConWind; // Conventional frame representation of this wind

            Vector3<real_Num> SarWind; // saracen frame representation of this wind
            m_sockHeight = Height;     // pass height to my debug global
            ScaleHt = ( Height - m_groundHeight ) / m_roughness;
            // the height as a multiple of the roughness length

            if( ScaleHt > 1 )
            {
                LogWind = ( m_uStar / KvonKarmen ) * Math<real_Num>::Ln( ScaleHt );
            }
            else
            {
                LogWind = 0; // calculate the Mean wind at this height
            }

            ConWind.X() = LogWind * ( m_kTurb * m_convXFactor + m_kCrossWind * m_convCrossX );
            // apply the turbulance and direction fractions for
            ConWind.Y() = LogWind * ( m_kTurb * m_convYFactor + m_kCrossWind * m_convCrossY );
            ConWind.Z() = 0; // make vertical component zero
            // SarWind = VecToSaracen(ConWind);  //convert to a temp. Saracen vector
            m_sockWind = SarWind; // pass the wind vec to my global bebug variable
            WindX = SarWind.X();  // pass the Saracen version of the wind out via the arguments
            WindY = SarWind.Y();
            WindZ = SarWind.Z();
        } /*default*/

        void CAerodymanicsWind::initWeather()
        {
            // initialization: todo
            // Randomize;
            // m_phaseTime1 = Random(5000); // pick a random start time for our turbulance
            // m_phaseTime2 = Random(6000);
            setWind( 0, 0, 0, 0, 0, 0.1 );
        }

        real_Num CAerodymanicsWind::getAirDensity() const
        {
            return static_cast<real_Num>( 1.225 );
        }

        void CAerodymanicsWind::setAirDensity( real_Num airDensity )
        {
        }

        void CAerodymanicsWind::setState( State state )
        {
        }
    } // namespace vehicle
} // namespace workphone
