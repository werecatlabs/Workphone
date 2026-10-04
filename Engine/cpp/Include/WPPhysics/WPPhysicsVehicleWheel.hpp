#ifndef WPPHYSICSVEHICLEWHEEL_HPP
#define WPPHYSICSVEHICLEWHEEL_HPP

#include <Workphone/Interface/Physics/IPhysicsVehicleWheel3.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    class WPPhysicsVehicleWheel final : public IPhysicsVehicleWheel3
    {
    public:
        real_Num getRadius() const override
        {
            return m_radius;
        }

        void setRadius( real_Num radius ) override
        {
            if( std::isfinite( static_cast<double>( radius ) ) )
            {
                m_radius = std::max( radius, static_cast<real_Num>( 0.001 ) );
            }
        }

        real_Num getWidth() const override
        {
            return m_width;
        }

        void setWidth( real_Num width ) override
        {
            if( std::isfinite( static_cast<double>( width ) ) )
            {
                m_width = std::max( width, static_cast<real_Num>( 0.001 ) );
            }
        }

        real_Num getMaxSuspensionTravelCm() const override
        {
            return m_maxSuspensionTravelCm;
        }

        void setMaxSuspensionTravelCm( real_Num value ) override
        {
            if( std::isfinite( static_cast<double>( value ) ) )
            {
                m_maxSuspensionTravelCm = std::max( value, static_cast<real_Num>( 0.0 ) );
            }
        }

        real_Num getMaxSuspensionForce() const override
        {
            return m_maxSuspensionForce;
        }

        void setMaxSuspensionForce( real_Num value ) override
        {
            if( std::isfinite( static_cast<double>( value ) ) )
            {
                m_maxSuspensionForce = std::max( value, static_cast<real_Num>( 0.0 ) );
            }
        }

        real_Num getSuspensionStiffness() const override
        {
            return m_suspensionStiffness;
        }

        void setSuspensionStiffness( real_Num value ) override
        {
            if( std::isfinite( static_cast<double>( value ) ) )
            {
                m_suspensionStiffness = std::max( value, static_cast<real_Num>( 0.0 ) );
            }
        }

        real_Num getSuspensionDamping() const override
        {
            return m_suspensionDamping;
        }

        void setSuspensionDamping( real_Num value ) override
        {
            if( std::isfinite( static_cast<double>( value ) ) )
            {
                m_suspensionDamping = std::max( value, static_cast<real_Num>( 0.0 ) );
            }
        }

        real_Num getFrictionSlip() const override
        {
            return m_frictionSlip;
        }

        void setFrictionSlip( real_Num value ) override
        {
            if( std::isfinite( static_cast<double>( value ) ) )
            {
                m_frictionSlip = std::max( value, static_cast<real_Num>( 0.0 ) );
            }
        }

        real_Num getSteering() const override
        {
            return m_steering;
        }

        void setSteering( real_Num value ) override
        {
            m_steering =
                std::isfinite( static_cast<double>( value ) ) ? value : static_cast<real_Num>( 0.0 );
        }

        real_Num getEngineForce() const override
        {
            return m_engineForce;
        }

        void setEngineForce( real_Num value ) override
        {
            m_engineForce =
                std::isfinite( static_cast<double>( value ) ) ? value : static_cast<real_Num>( 0.0 );
        }

        real_Num getBrake() const override
        {
            return m_brake;
        }

        void setBrake( real_Num value ) override
        {
            m_brake = std::isfinite( static_cast<double>( value ) )
                        ? std::max( value, static_cast<real_Num>( 0.0 ) )
                        : static_cast<real_Num>( 0.0 );
        }

        bool isInContact() const override
        {
            return m_inContact;
        }

        void setInContact( bool inContact ) const
        {
            m_inContact = inContact;
        }

    private:
        real_Num     m_radius = static_cast<real_Num>( 0.35 );
        real_Num     m_width = static_cast<real_Num>( 0.25 );
        real_Num     m_maxSuspensionTravelCm = static_cast<real_Num>( 20.0 );
        real_Num     m_maxSuspensionForce = static_cast<real_Num>( 6000.0 );
        real_Num     m_suspensionStiffness = static_cast<real_Num>( 35.0 );
        real_Num     m_suspensionDamping = static_cast<real_Num>( 4.5 );
        real_Num     m_frictionSlip = static_cast<real_Num>( 1.0 );
        real_Num     m_steering = static_cast<real_Num>( 0.0 );
        real_Num     m_engineForce = static_cast<real_Num>( 0.0 );
        real_Num     m_brake = static_cast<real_Num>( 0.0 );
        mutable bool m_inContact = false;
    };
}

#endif // WPPHYSICSVEHICLEWHEEL_HPP
