#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/BladeElementRotor.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <cmath>

namespace workphone
{
    namespace vehicle
    {

        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, BladeElementRotor,
                                   CAircraftAttachment<IVehicleComponent> );

        // ===================================================================
        // Construction
        // ===================================================================

        BladeElementRotor::BladeElementRotor()
        {
            m_segmentLength = m_rotorRadius / static_cast<real_Num>( m_segmentsPerBlade );
        }

        BladeElementRotor::~BladeElementRotor() = default;

        // ===================================================================
        // Lifecycle
        // ===================================================================

        void BladeElementRotor::load( SmartPtr<ISharedObject> data )
        {
            // Recompute the cached segment length whenever parameters are loaded.
            if( m_segmentsPerBlade > 0 )
                m_segmentLength = m_rotorRadius / static_cast<real_Num>( m_segmentsPerBlade );
        }

        void BladeElementRotor::update( const double &t, const double &dt )
        {
            m_totalForce = Vector3<real_Num>::zero();
            m_totalTorque = Vector3<real_Num>::zero();

            if( m_rotorRPM < static_cast<real_Num>( 0.01 ) )
                return;

            // Angular velocity (rad/s): ω = RPM × 2π / 60
            const real_Num omega = m_rotorRPM * Math<real_Num>::two_pi() / static_cast<real_Num>( 60.0 );

            // Body-space axes from the attachment world transform.
            const auto &worldTransform = getWorldTransform();
            const auto  orientation = worldTransform.getOrientation();

            const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();
            const Vector3<real_Num> worldRight = orientation * Vector3<real_Num>::unitX();

            simulateRotor( omega, worldUp, worldRight );

            // Apply accumulated results to the parent physics body.
            auto body = getParent();
            if( body )
            {
                const Vector3<real_Num> hubWorldPos = worldTransform.getPosition();
                body->addForceAtPosition( m_totalForce, hubWorldPos );
                body->addTorque( m_totalTorque );
            }
        }

        // ===================================================================
        // Core blade element integration
        // ===================================================================

        void BladeElementRotor::simulateRotor( real_Num omega, const Vector3<real_Num> &worldUp,
                                               const Vector3<real_Num> &worldRight )
        {
            const auto &worldTransform = getWorldTransform();
            const auto  orientation = worldTransform.getOrientation();
            auto        body = getParent();

            // Refresh cached segment length in case parameters changed at runtime.
            if( m_segmentsPerBlade > 0 )
                m_segmentLength = m_rotorRadius / static_cast<real_Num>( m_segmentsPerBlade );

            const real_Num twoPi = Math<real_Num>::two_pi();
            const real_Num degToRad = Math<real_Num>::deg_to_rad();

            // Shared dynamic pressure factor prefix: 0.5 × � × chord × segLen
            // The per-segment area (chord × segmentLength) is factored out of the
            // lift/drag magnitude expressions and multiplied in once below.
            const real_Num area = m_chordLength * m_segmentLength;
            const real_Num halfRhoArea = static_cast<real_Num>( 0.5 ) * m_airDensity * area;

            // Cyclic pitch amplitude in degrees (physical deflection).
            const real_Num cyclicXDeg = m_cyclicInput.X() * m_cyclicPitchMax;
            const real_Num cyclicYDeg = m_cyclicInput.Y() * m_cyclicPitchMax;

            for( s32 blade = 0; blade < m_bladeCount; ++blade )
            {
                // Azimuth angle of this blade (radians), evenly spaced around the disc.
                const real_Num psi =
                    static_cast<real_Num>( blade ) * twoPi / static_cast<real_Num>( m_bladeCount );

                // Rotation quaternion that places the blade along the rotor X axis
                // at azimuth ψ (rotating around the body-local Y/up axis).
                const Quaternion<real_Num> bladeRot =
                    Quaternion<real_Num>::angleAxis( psi, Vector3<real_Num>::unitY() );

                // First-harmonic cyclic pitch variation:
                //   pitch(ψ) = collective + cyclicX·sin(ψ) + cyclicY·cos(ψ)
                const real_Num pitchDeg = m_collectivePitch +
                                          cyclicXDeg * static_cast<real_Num>( std::sin( psi ) ) +
                                          cyclicYDeg * static_cast<real_Num>( std::cos( psi ) );
                const real_Num alpha = pitchDeg * degToRad; // angle of attack (rad)

                // Section aerodynamic coefficients.
                const real_Num Cl = m_liftSlope * alpha;
                const real_Num Cd = m_dragCoeff + Cl * Cl * static_cast<real_Num>( 0.02 );

                for( s32 seg = 1; seg <= m_segmentsPerBlade; ++seg )
                {
                    // Radial station of this segment.
                    const real_Num radius = static_cast<real_Num>( seg ) * m_segmentLength;

                    // Local position of the segment centre in attachment space —
                    // blade extends along X, rotated to azimuth ψ around Y.
                    const Vector3<real_Num> localPos =
                        bladeRot * Vector3<real_Num>( radius, static_cast<real_Num>( 0.0 ),
                                                      static_cast<real_Num>( 0.0 ) );

                    // World-space position of this segment.
                    const Vector3<real_Num> worldPos =
                        worldTransform.getPosition() + ( orientation * localPos );

                    // Tangential velocity due to rotor rotation:
                    //   v_tan = ω × r  (in the rotor plane, perpendicular to the blade radius)
                    // In the local rotor frame the rotation axis is Y, so the tangential
                    // direction for a segment at (r,0,0) is (0,0,−r·ω) before azimuth rotation.
                    const Vector3<real_Num> localTangential =
                        bladeRot * Vector3<real_Num>( static_cast<real_Num>( 0.0 ),
                                                      static_cast<real_Num>( 0.0 ), -radius * omega );
                    const Vector3<real_Num> tangentialVelWorld = orientation * localTangential;

                    // Body velocity at this world point (accounts for translation + rotation).
                    Vector3<real_Num> bodyPointVel = Vector3<real_Num>::zero();
                    if( body )
                        bodyPointVel = body->getPointVelocity( worldPos );

                    // Total air velocity seen by the section.
                    const Vector3<real_Num> airVelocity = tangentialVelWorld + bodyPointVel;
                    const real_Num          speed = airVelocity.length();

                    if( speed < static_cast<real_Num>( 0.01 ) )
                        continue;

                    const Vector3<real_Num> airflowDir = airVelocity / speed;

                    // Dynamic pressure factor for this section.
                    const real_Num q = halfRhoArea * speed * speed;

                    const real_Num liftMag = q * Cl;
                    const real_Num dragMag = q * Cd;

                    // Lift direction: perpendicular to airflow, in the plane containing
                    // airflow and the blade span (right axis).
                    Vector3<real_Num> liftDir = airflowDir.crossProduct( worldRight ).normaliseCopy();

                    // Drag direction opposes relative airflow.
                    const Vector3<real_Num> dragDir = -airflowDir;

                    const Vector3<real_Num> segForce = liftDir * liftMag + dragDir * dragMag;

                    m_totalForce += segForce;

                    // Torque = r × F, where r is the segment position relative to the hub.
                    // Use the local (attachment-frame) position so the torque is about the hub.
                    const Vector3<real_Num> worldLocalPos = orientation * localPos;
                    m_totalTorque += worldLocalPos.crossProduct( segForce );
                }
            }
        }

        // ===================================================================
        // Output accessors
        // ===================================================================

        Vector3<real_Num> BladeElementRotor::getTotalForce() const
        {
            return m_totalForce;
        }

        Vector3<real_Num> BladeElementRotor::getTotalTorque() const
        {
            return m_totalTorque;
        }

        // ===================================================================
        // Geometry accessors
        // ===================================================================

        s32 BladeElementRotor::getBladeCount() const
        {
            return m_bladeCount;
        }
        void BladeElementRotor::setBladeCount( s32 bladeCount )
        {
            m_bladeCount = bladeCount;
        }

        s32 BladeElementRotor::getSegmentsPerBlade() const
        {
            return m_segmentsPerBlade;
        }
        void BladeElementRotor::setSegmentsPerBlade( s32 segmentsPerBlade )
        {
            m_segmentsPerBlade = segmentsPerBlade;
            if( m_segmentsPerBlade > 0 )
                m_segmentLength = m_rotorRadius / static_cast<real_Num>( m_segmentsPerBlade );
        }

        real_Num BladeElementRotor::getRotorRadius() const
        {
            return m_rotorRadius;
        }
        void BladeElementRotor::setRotorRadius( real_Num rotorRadius )
        {
            m_rotorRadius = rotorRadius;
            if( m_segmentsPerBlade > 0 )
                m_segmentLength = m_rotorRadius / static_cast<real_Num>( m_segmentsPerBlade );
        }

        real_Num BladeElementRotor::getChordLength() const
        {
            return m_chordLength;
        }
        void BladeElementRotor::setChordLength( real_Num chordLength )
        {
            m_chordLength = chordLength;
        }

        // ===================================================================
        // Aerodynamic coefficient accessors
        // ===================================================================

        real_Num BladeElementRotor::getAirDensity() const
        {
            return m_airDensity;
        }
        void BladeElementRotor::setAirDensity( real_Num airDensity )
        {
            m_airDensity = airDensity;
        }

        real_Num BladeElementRotor::getLiftSlope() const
        {
            return m_liftSlope;
        }
        void BladeElementRotor::setLiftSlope( real_Num liftSlope )
        {
            m_liftSlope = liftSlope;
        }

        real_Num BladeElementRotor::getDragCoeff() const
        {
            return m_dragCoeff;
        }
        void BladeElementRotor::setDragCoeff( real_Num dragCoeff )
        {
            m_dragCoeff = dragCoeff;
        }

        // ===================================================================
        // State input accessors
        // ===================================================================

        real_Num BladeElementRotor::getRotorRPM() const
        {
            return m_rotorRPM;
        }
        void BladeElementRotor::setRotorRPM( real_Num rotorRPM )
        {
            m_rotorRPM = rotorRPM;
        }

        real_Num BladeElementRotor::getCollectivePitch() const
        {
            return m_collectivePitch;
        }
        void BladeElementRotor::setCollectivePitch( real_Num collectivePitch )
        {
            m_collectivePitch = collectivePitch;
        }

        Vector2<real_Num> BladeElementRotor::getCyclicInput() const
        {
            return m_cyclicInput;
        }
        void BladeElementRotor::setCyclicInput( const Vector2<real_Num> &cyclicInput )
        {
            m_cyclicInput = cyclicInput;
        }

        real_Num BladeElementRotor::getCyclicPitchMax() const
        {
            return m_cyclicPitchMax;
        }

        void BladeElementRotor::setCyclicPitchMax( real_Num cyclicPitchMax )
        {
            m_cyclicPitchMax = cyclicPitchMax;
        }

    } // namespace vehicle
} // namespace workphone
