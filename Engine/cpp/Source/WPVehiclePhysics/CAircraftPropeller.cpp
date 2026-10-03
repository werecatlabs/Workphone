#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftPropeller.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include "WPVehiclePhysics/CAircraftEngine.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace vehicle
    {
        CAircraftPropeller::CAircraftPropeller()
        {
            // SmartPtr<CAircraftEngine> pEngine = boost::static_pointer_cast<CAircraftEngine>(m_engine);
            // CAircraftEngine& ThisEngine = *pEngine;

            CAircraftPropeller &thisProp = *this;

            m_thrustLine = -Vector3<real_Num>::UNIT_Z;

            thisProp.m_position.X() = 0.2;
            thisProp.m_position.Y() = 0;
            thisProp.m_position.Z() = 0;
            // ThisEngine.PeakPowerRevs = 12000;
            thisProp.m_propDia = 11 / 39.37;
            thisProp.m_propPitch = 9 / 39.37;
            thisProp.m_propChord = 0.15;
            thisProp.m_propBlades = 2;
            thisProp.m_downThrust = 0;
            thisProp.m_sideThrust = 0;
            m_wProp = 0;

            m_propDia = 0;
            m_cuffChord = 0;
            m_tipChord = 0;
            m_propChord = 0;
            m_propPitch = 0;
            m_propDensity = 0;
            m_geomerticPitch = 0;
            m_propCd0 = 0;
            m_dCdBydCl2 = 0;
            m_dClByAlpha = 0;
            m_propClMax = 0;
            m_propClMin = 0;
            m_propCl = 0;
            m_propCd = 0;
            m_downThrust = 0;
            m_sideThrust = 0;
            m_propTorque = 0;
            m_inputTorque = 0;
            m_reaction = 0;
            m_wProp = 0;
            m_yawPFac = 0;
            m_pitchPFac = 0;
            m_yawDrag = 0;
            m_pitchDrag = 0;
            m_propWash = 0;
            m_propThrustValue = 0;
            m_propArea = 0;
            m_bladeArea = 0;
            m_propMoI = 0;
            m_totMoI = 0;
            m_angMomentum = 0;
            m_propSolRatio = 0;
            m_propDragPowerFactor = 0;
            m_massFlow = 0;
            m_wakeMoI = 0;
            m_wakeRotation = 0;
            m_propCLCoef = 0;
            m_proprThrustKg = 0;
        }

        CAircraftPropeller::~CAircraftPropeller()
        {
        }

        void CAircraftPropeller::load( SmartPtr<ISharedObject> data )
        {
            auto properties = workphone::dynamic_pointer_cast<Properties>( data );
            if( properties )
            {
                properties->getPropertyValue( "FlEqPropDiameter", m_propDia );
                properties->getPropertyValue( "FlEqPropPitch", m_propPitch );
                properties->getPropertyValue( "FlEqPropNumBlades", m_propBlades );
                properties->getPropertyValue( "FlEqPropCuffChord", m_cuffChord );
                properties->getPropertyValue( "FlEqPropTipChord", m_tipChord );
                properties->getPropertyValue( "FlEqPropDensity", m_propDensity );
                properties->getPropertyValue( "FlEqPropDucted", m_ducted );
                properties->getPropertyValue( "FlEqPropFolding", m_folding );
                properties->getPropertyValue( "FlEqPropCd0", m_propCd0 );
                properties->getPropertyValue( "FlEqPropClMin", m_propClMin );
                properties->getPropertyValue( "FlEqPropClMax", m_propClMax );
                properties->getPropertyValue( "FlEqPropCdbydCl2", m_dCdBydCl2 );
                properties->getPropertyValue( "FlEqPropReverseRotn", m_reversed );
                properties->getPropertyValue( "FlEqPropSideThrust", m_sideThrust );
                properties->getPropertyValue( "FlEqPropDownThrust", m_downThrust );
            }

            m_propDia /= static_cast<real_Num>( 39.37 );
            m_propPitch /= static_cast<real_Num>( 39.37 );

            m_cuffChord /= static_cast<real_Num>( 1000.0 );
            m_tipChord /= static_cast<real_Num>( 1000.0 );

            m_sideThrust *= Math<real_Num>::pi() / static_cast<real_Num>( 180.0 );
            m_downThrust *= Math<real_Num>::pi() / static_cast<real_Num>( 180.0 );

            m_dClByAlpha = static_cast<real_Num>( 2.0 ) * Math<real_Num>::pi();

            const double PropDens = 1.4E3; /*density of prop material*/
            const double Cdprop = 0.5;

            auto rFixedWingLL = m_parentAircraft;

            CAircraftPropeller &curProp = *this;

            curProp.m_wProp =
                static_cast<real_Num>( 0.0 ); // set the rotation speed non-zero to ensure running
            curProp.m_propChord =
                ( static_cast<real_Num>( 2.0 ) * curProp.m_tipChord + curProp.m_cuffChord ) /
                static_cast<real_Num>( 3.0 );
            // asssume the effective chord is weighted 2:1 towards the tip
            real_Num pd4 = curProp.m_propDia * curProp.m_propDia * curProp.m_propDia * curProp.m_propDia;
            // calc the 4th power of the diameter
            curProp.m_propDragPowerFactor = curProp.m_propBlades * rFixedWingLL->getAirDensity() *
                                            curProp.m_propChord * Cdprop * pd4 /
                                            static_cast<real_Num>( 128.0 );
            // curProp.m_iProp = real_Num(0.01) * curProp.m_propBlades * PropDens * curProp.m_propChord *
            // curProp.m_propChord * curProp.m_propDia * curProp.m_propDia * curProp.m_propDia; //based
            // on abot 12% mean blade thickness
            curProp.m_propArea = Math<real_Num>::pi() * ( curProp.m_propDia * curProp.m_propDia ) /
                                 static_cast<real_Num>( 4.0 ); // the area of the prop disc
            curProp.m_bladeArea = curProp.m_propBlades * curProp.m_propChord *
                                  static_cast<real_Num>( 0.4 ) *
                                  curProp.m_propDia; // calculate the effective area of the prop blades
            curProp.m_propSolRatio =
                curProp.m_bladeArea / curProp.m_propArea; // the solidity ratio of the prop
            curProp.m_geomerticPitch =
                curProp.m_propPitch /
                ( Math<real_Num>::pi() * static_cast<real_Num>( 0.7 ) *
                  curProp.m_propDia ); // calc the effective pitch angle of the prop

            auto thrustLine = Vector3<real_Num>::zero();
            thrustLine.X() = Math<real_Num>::Cos( m_downThrust ) * Math<real_Num>::Cos( m_sideThrust );
            thrustLine.Y() = Math<real_Num>::Sin( m_sideThrust );
            thrustLine.Z() = Math<real_Num>::Sin( m_downThrust );

            m_thrustLine = thrustLine.normaliseCopy(); // make it a unit vector
        }

        void CAircraftPropeller::update( const double &time, const double &deltaTime )
        {
        }

        Vector3<real_Num> CAircraftPropeller::getThrust() const
        {
            return m_propThrust;
        }

        void CAircraftPropeller::setThrust( const Vector3<real_Num> &thrust )
        {
            m_propThrust = thrust;
        }

        real_Num CAircraftPropeller::getPropwash() const
        {
            return m_propWash;
        }

        void CAircraftPropeller::setPropwash( real_Num propwash )
        {
            m_propWash = propwash;
        }

        real_Num CAircraftPropeller::getThrustValue() const
        {
            WP_ASSERT( Math<real_Num>::isFinite( m_propThrustValue ) );
            return m_propThrustValue;
        }

        void CAircraftPropeller::setThrustValue( real_Num thrustValue )
        {
            WP_ASSERT( Math<real_Num>::isFinite( thrustValue ) );
            WP_ASSERT( Math<real_Num>::isFinite( m_propThrustValue ) );
            m_propThrustValue = thrustValue;
        }

        real_Num CAircraftPropeller::getRotationRate() const
        {
            return m_wProp;
        }

        void CAircraftPropeller::setRotationRate( real_Num rotationRate )
        {
            WP_ASSERT( Math<real_Num>::isFinite( rotationRate ) );
            m_wProp = rotationRate;
        }

        real_Num CAircraftPropeller::getDiameter() const
        {
            return m_propDia;
        }

        void CAircraftPropeller::setDiameter( real_Num diameter )
        {
            m_propDia = diameter;
        }

        real_Num CAircraftPropeller::propwash() const
        {
            return m_propWash;
        }

        void CAircraftPropeller::propwash( const real_Num propwash )
        {
            m_propWash = propwash;
        }

        Vector3<real_Num> CAircraftPropeller::getPosition() const
        {
            return m_position;
        }

        void CAircraftPropeller::setPosition( Vector3<real_Num> position )
        {
            m_position = position;
        }

        Vector3<real_Num> CAircraftPropeller::propThrust() const
        {
            return m_propThrust;
        }

        void CAircraftPropeller::propThrust( Vector3<real_Num> propThrust )
        {
            m_propThrust = propThrust;
        }

        Vector3<real_Num> CAircraftPropeller::angMomVector() const
        {
            return m_angMomVector;
        }

        void CAircraftPropeller::angMomVector( Vector3<real_Num> angMomVector )
        {
            m_angMomVector = angMomVector;
        }

        Vector3<real_Num> CAircraftPropeller::reactionVector() const
        {
            return m_reactionVector;
        }

        void CAircraftPropeller::reactionVector( Vector3<real_Num> reactionVector )
        {
            m_reactionVector = reactionVector;
        }

        Vector3<real_Num> CAircraftPropeller::discFlow() const
        {
            return m_discFlow;
        }

        void CAircraftPropeller::discFlow( Vector3<real_Num> discFlow )
        {
            m_discFlow = discFlow;
        }

        Vector3<real_Num> CAircraftPropeller::wakeRotationVector() const
        {
            return m_wakeRotationVector;
        }

        void CAircraftPropeller::wakeRotationVector( Vector3<real_Num> wakeRotationVector )
        {
            m_wakeRotationVector = wakeRotationVector;
        }

        CAircraftPropeller::FrameOfRef CAircraftPropeller::propFrame() const
        {
            return m_propFrame;
        }

        void CAircraftPropeller::propFrame( FrameOfRef propFrame )
        {
            m_propFrame = propFrame;
        }

        s32 CAircraftPropeller::propBlades() const
        {
            return m_propBlades;
        }

        void CAircraftPropeller::propBlades( s32 propBlades )
        {
            m_propBlades = propBlades;
        }

        real_Num CAircraftPropeller::iProp() const
        {
            return m_iProp;
        }

        void CAircraftPropeller::iProp( real_Num iProp )
        {
            m_iProp = iProp;
        }

        int CAircraftPropeller::throttleChannel() const
        {
            return m_throttleChannel;
        }

        void CAircraftPropeller::throttleChannel( int throttleChannel )
        {
            m_throttleChannel = throttleChannel;
        }

        real_Num CAircraftPropeller::propDia() const
        {
            return m_propDia;
        }

        void CAircraftPropeller::propDia( real_Num propDia )
        {
            m_propDia = propDia;
        }

        real_Num CAircraftPropeller::cuffChord() const
        {
            return m_cuffChord;
        }

        void CAircraftPropeller::cuffChord( real_Num cuffChord )
        {
            m_cuffChord = cuffChord;
        }

        real_Num CAircraftPropeller::tipChord() const
        {
            return m_tipChord;
        }

        void CAircraftPropeller::tipChord( real_Num tipChord )
        {
            m_tipChord = tipChord;
        }

        real_Num CAircraftPropeller::propChord() const
        {
            return m_propChord;
        }

        void CAircraftPropeller::propChord( real_Num propChord )
        {
            m_propChord = propChord;
        }

        real_Num CAircraftPropeller::propPitch() const
        {
            return m_propPitch;
        }

        void CAircraftPropeller::propPitch( real_Num propPitch )
        {
            m_propPitch = propPitch;
        }

        real_Num CAircraftPropeller::propDensity() const
        {
            return m_propDensity;
        }

        void CAircraftPropeller::propDensity( real_Num propDensity )
        {
            m_propDensity = propDensity;
        }

        real_Num CAircraftPropeller::ppAng() const
        {
            return m_geomerticPitch;
        }

        void CAircraftPropeller::ppAng( real_Num ppAng )
        {
            m_geomerticPitch = ppAng;
        }

        real_Num CAircraftPropeller::propCd0() const
        {
            return m_propCd0;
        }

        void CAircraftPropeller::propCd0( real_Num propCd0 )
        {
            m_propCd0 = propCd0;
        }

        real_Num CAircraftPropeller::dCdBydCl3() const
        {
            return m_dCdBydCl2;
        }

        void CAircraftPropeller::dCdBydCl3( real_Num dCdBydCl3 )
        {
            m_dCdBydCl2 = dCdBydCl3;
        }

        real_Num CAircraftPropeller::dClByAlpha1() const
        {
            return m_dClByAlpha;
        }

        void CAircraftPropeller::dClByAlpha1( real_Num dClByAlpha )
        {
            m_dClByAlpha = dClByAlpha;
        }

        real_Num CAircraftPropeller::propClMax() const
        {
            return m_propClMax;
        }

        void CAircraftPropeller::propClMax( real_Num propClMax )
        {
            m_propClMax = propClMax;
        }

        real_Num CAircraftPropeller::propClMin() const
        {
            return m_propClMin;
        }

        void CAircraftPropeller::propClMin( real_Num propClMin )
        {
            m_propClMin = propClMin;
        }

        real_Num CAircraftPropeller::propCl() const
        {
            return m_propCl;
        }

        void CAircraftPropeller::propCl( real_Num propCl )
        {
            m_propCl = propCl;
        }

        real_Num CAircraftPropeller::propCd() const
        {
            return m_propCd;
        }

        void CAircraftPropeller::propCd( real_Num propCd )
        {
            m_propCd = propCd;
        }

        real_Num CAircraftPropeller::downThrust() const
        {
            return m_downThrust;
        }

        void CAircraftPropeller::downThrust( real_Num downThrust )
        {
            m_downThrust = downThrust;
        }

        real_Num CAircraftPropeller::sideThrust() const
        {
            return m_sideThrust;
        }

        void CAircraftPropeller::sideThrust( real_Num sideThrust )
        {
            m_sideThrust = sideThrust;
        }

        real_Num CAircraftPropeller::propTorque() const
        {
            WP_ASSERT( Math<real_Num>::isFinite( m_propTorque ) );
            return m_propTorque;
        }

        void CAircraftPropeller::propTorque( real_Num propTorque )
        {
            WP_ASSERT( Math<real_Num>::isFinite( propTorque ) );
            WP_ASSERT( Math<real_Num>::isFinite( m_propTorque ) );
            m_propTorque = propTorque;
        }

        real_Num CAircraftPropeller::inputTorque() const
        {
            return m_inputTorque;
        }

        void CAircraftPropeller::inputTorque( real_Num inputTorque )
        {
            m_inputTorque = inputTorque;
        }

        real_Num CAircraftPropeller::reaction() const
        {
            return m_reaction;
        }

        void CAircraftPropeller::reaction( real_Num reaction )
        {
            m_reaction = reaction;
        }

        real_Num CAircraftPropeller::wProp() const
        {
            return m_wProp;
        }

        void CAircraftPropeller::wProp( real_Num wProp )
        {
            m_wProp = wProp;
        }

        real_Num CAircraftPropeller::yawPFac() const
        {
            return m_yawPFac;
        }

        void CAircraftPropeller::yawPFac( real_Num yawPFac )
        {
            m_yawPFac = yawPFac;
        }

        real_Num CAircraftPropeller::pitchPFac() const
        {
            return m_pitchPFac;
        }

        void CAircraftPropeller::pitchPFac( real_Num pitchPFac )
        {
            m_pitchPFac = pitchPFac;
        }

        real_Num CAircraftPropeller::yawDrag() const
        {
            return m_yawDrag;
        }

        void CAircraftPropeller::yawDrag( real_Num yawDrag )
        {
            m_yawDrag = yawDrag;
        }

        real_Num CAircraftPropeller::pitchDrag() const
        {
            return m_pitchDrag;
        }

        void CAircraftPropeller::pitchDrag( real_Num pitchDrag )
        {
            m_pitchDrag = pitchDrag;
        }

        real_Num CAircraftPropeller::propWash() const
        {
            return m_propWash;
        }

        void CAircraftPropeller::propWash( real_Num propWash )
        {
            m_propWash = propWash;
        }

        real_Num CAircraftPropeller::propThrustValue() const
        {
            return m_propThrustValue;
        }

        void CAircraftPropeller::propThrustValue( real_Num propThrustValue )
        {
            m_propThrustValue = propThrustValue;
        }

        real_Num CAircraftPropeller::propArea() const
        {
            return m_propArea;
        }

        void CAircraftPropeller::propArea( real_Num propArea )
        {
            m_propArea = propArea;
        }

        real_Num CAircraftPropeller::bladeArea() const
        {
            return m_bladeArea;
        }

        void CAircraftPropeller::bladeArea( real_Num bladeArea )
        {
            m_bladeArea = bladeArea;
        }

        real_Num CAircraftPropeller::propMoI() const
        {
            return m_propMoI;
        }

        void CAircraftPropeller::propMoI( real_Num propMoI )
        {
            m_propMoI = propMoI;
        }

        real_Num CAircraftPropeller::totMoI() const
        {
            return m_totMoI;
        }

        void CAircraftPropeller::totMoI( real_Num totMoI )
        {
            m_totMoI = totMoI;
        }

        real_Num CAircraftPropeller::angMomentum() const
        {
            return m_angMomentum;
        }

        void CAircraftPropeller::angMomentum( real_Num angMomentum )
        {
            m_angMomentum = angMomentum;
        }

        real_Num CAircraftPropeller::propSolRatio() const
        {
            return m_propSolRatio;
        }

        void CAircraftPropeller::propSolRatio( real_Num propSolRatio )
        {
            m_propSolRatio = propSolRatio;
        }

        real_Num CAircraftPropeller::propDragPowerFactor() const
        {
            return m_propDragPowerFactor;
        }

        void CAircraftPropeller::propDragPowerFactor( real_Num propDragPowerFactor )
        {
            m_propDragPowerFactor = propDragPowerFactor;
        }

        real_Num CAircraftPropeller::massFlow() const
        {
            return m_massFlow;
        }

        void CAircraftPropeller::massFlow( real_Num massFlow )
        {
            m_massFlow = massFlow;
        }

        real_Num CAircraftPropeller::wakeMoI() const
        {
            return m_wakeMoI;
        }

        void CAircraftPropeller::wakeMoI( real_Num wakeMoI )
        {
            m_wakeMoI = wakeMoI;
        }

        real_Num CAircraftPropeller::wakeRotation() const
        {
            return m_wakeRotation;
        }

        void CAircraftPropeller::wakeRotation( real_Num wakeRotation )
        {
            m_wakeRotation = wakeRotation;
        }

        real_Num CAircraftPropeller::propClCoef() const
        {
            return m_propCLCoef;
        }

        void CAircraftPropeller::propClCoef( real_Num propClCoef )
        {
            m_propCLCoef = propClCoef;
        }

        real_Num CAircraftPropeller::proprThrustKg() const
        {
            return m_proprThrustKg;
        }

        void CAircraftPropeller::proprThrustKg( real_Num proprThrustKg )
        {
            m_proprThrustKg = proprThrustKg;
        }

        Array<real_Num> CAircraftPropeller::propwashFactor() const
        {
            return m_propwashFactor;
        }

        void CAircraftPropeller::propwashFactor( Array<real_Num> propwashFactor )
        {
            m_propwashFactor = propwashFactor;
        }

        Array<Vector3<real_Num>> CAircraftPropeller::pWash() const
        {
            return m_pWash;
        }

        void CAircraftPropeller::pWash( Array<Vector3<real_Num>> pWash )
        {
            m_pWash = pWash;
        }

        Array<Vector3<real_Num>> CAircraftPropeller::wakeVec() const
        {
            return m_wakeVec;
        }

        void CAircraftPropeller::wakeVec( Array<Vector3<real_Num>> wakeVec )
        {
            m_wakeVec = wakeVec;
        }

        Array<real_Num> CAircraftPropeller::propwashFactorFus() const
        {
            return m_propwashFactorFus;
        }

        void CAircraftPropeller::propwashFactorFus( Array<real_Num> propwashFactorFus )
        {
            m_propwashFactorFus = propwashFactorFus;
        }

        Array<Vector3<real_Num>> CAircraftPropeller::pWashFus() const
        {
            return m_pWashFus;
        }

        void CAircraftPropeller::pWashFus( Array<Vector3<real_Num>> pWashFus )
        {
            m_pWashFus = pWashFus;
        }

        Array<Vector3<real_Num>> CAircraftPropeller::wakeVecFus() const
        {
            return m_wakeVecFus;
        }

        void CAircraftPropeller::wakeVecFus( Array<Vector3<real_Num>> wakeVecFus )
        {
            m_wakeVecFus = wakeVecFus;
        }

        bool CAircraftPropeller::reversed() const
        {
            return m_reversed;
        }

        void CAircraftPropeller::reversed( bool reversed )
        {
            m_reversed = reversed;
        }

        bool CAircraftPropeller::ducted() const
        {
            return m_ducted;
        }

        void CAircraftPropeller::ducted( bool ducted )
        {
            m_ducted = ducted;
        }

        bool CAircraftPropeller::folding() const
        {
            return m_folding;
        }

        void CAircraftPropeller::folding( bool folding )
        {
            m_folding = folding;
        }

        workphone::real_Num CAircraftPropeller::getDownThrust() const
        {
            return m_downThrust;
        }

        void CAircraftPropeller::setDownThrust( real_Num downThrust )
        {
            m_downThrust = downThrust;
        }

        workphone::real_Num CAircraftPropeller::getSideThrust() const
        {
            return m_sideThrust;
        }

        void CAircraftPropeller::setSideThrust( real_Num sideThrust )
        {
            m_sideThrust = sideThrust;
        }

        void CAircraftPropeller::reset()
        {
            m_propChord = ( static_cast<real_Num>( 2.0 ) * m_tipChord + m_cuffChord ) /
                          static_cast<real_Num>( 3.0 );
            // assume the effective chord is weighted 2:1 towards the tip
            m_propMoI = static_cast<real_Num>( 6.0 ) * m_propBlades * m_propDensity *
                        Math<real_Num>::Pow( m_propChord, static_cast<real_Num>( 2.0 ) ) *
                        Math<real_Num>::Pow( m_propDia, static_cast<real_Num>( 3.0 ) );
            // based on abot 12% mean blade thickness Checked as reasonable 11/12/2014
            m_totMoI = m_propMoI; // to prevent a zero total MoI make it equal to the prop MoI here

            m_propArea = Math<real_Num>::pi() * ( m_propDia * m_propDia ) / static_cast<real_Num>( 4.0 );
            // the area of the prop disc

            m_bladeArea = m_propBlades * m_propChord * static_cast<real_Num>( 0.25 ) * m_propDia;
            // calculate the effective area of the prop blades reduced to 50% of the blade length
            m_propSolRatio = m_bladeArea / m_propArea; // the solidity ratio of the prop

            // now calc the effective pitch angle of the prop at 75% of diameter allowing 0.035 radians
            // (2 degrees) for about 2% camber of the blade
            //  and also allowing 12% for the typical extra pitch APC etc seem to have for the geometric
            //  pitch.
            m_geomerticPitch = static_cast<real_Num>( 0.035 ) +
                               static_cast<real_Num>( 1.12 ) * m_propPitch /
                                   ( Math<real_Num>::pi() * static_cast<real_Num>( 0.75 ) * m_propDia );

            // this precalculated value now excludes the Cd as this now varies with Cl
            m_propDragPowerFactor = m_propBlades * m_parentAircraft->getAirDensity() * m_propChord *
                                    Math<real_Num>::Pow( m_propDia, static_cast<real_Num>( 4.0 ) ) /
                                    static_cast<real_Num>( 128.0 );

            m_propFrame.xAxis.X() =
                Math<real_Num>::Cos( m_downThrust ) * Math<real_Num>::Cos( m_sideThrust );
            //
            m_propFrame.xAxis.Y() = Math<real_Num>::Sin( m_sideThrust );
            m_propFrame.xAxis.Z() = Math<real_Num>::Sin( m_downThrust );
            m_propFrame.xAxis = m_propFrame.xAxis.normaliseCopy();

            m_propFrame.yAxis.X() = -Math<real_Num>::Sin( m_sideThrust );
            m_propFrame.yAxis.Y() = Math<real_Num>::Cos( m_sideThrust );
            m_propFrame.yAxis.Z() = static_cast<real_Num>( 0.0 );
            m_propFrame.yAxis = m_propFrame.yAxis.normaliseCopy(); // make it a unit vector

            m_propFrame.zAxis.X() = -Math<real_Num>::Sin( m_downThrust );
            m_propFrame.zAxis.Y() = static_cast<real_Num>( 0.0 );
            m_propFrame.zAxis.Z() = Math<real_Num>::Cos( m_downThrust );
            m_propFrame.zAxis = m_propFrame.zAxis.normaliseCopy(); // make it a unit vector
        }
    } // namespace vehicle
} // namespace workphone
