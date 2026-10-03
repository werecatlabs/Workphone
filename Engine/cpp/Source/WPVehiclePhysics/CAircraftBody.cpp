#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace vehicle
    {
        CAircraftBody::CAircraftBody() : m_mass( 1.0 )
        {
        }

        CAircraftBody::~CAircraftBody()
        {
        }

        Vector3<real_Num> CAircraftBody::getVelocity() const
        {
            return m_velocity;
        }

        void CAircraftBody::setVelocity( const Vector3<real_Num> &velocity )
        {
            m_velocity = velocity;
        }

        Vector3<real_Num> CAircraftBody::getAngularVelocity() const
        {
            return m_angularVelocity;
        }

        void CAircraftBody::setAngularVelocity( const Vector3<real_Num> &angularVelocity )
        {
            m_angularVelocity = angularVelocity;
        }

        Vector3<real_Num> CAircraftBody::getWorldCenterOfMass() const
        {
            return m_worldCenterOfMass;
        }

        void CAircraftBody::setWorldCenterOfMass( const Vector3<real_Num> &worldCenterOfMass )
        {
            m_worldCenterOfMass = worldCenterOfMass;
        }

        void CAircraftBody::addLocalForceAtPosition( const Vector3<real_Num> &force,
                                                     const Vector3<real_Num> &pos,
                                                     physics::ForceModeEnum   forceMode )
        {
            m_parentAircraft->addLocalForce( 0, force, pos );
        }

        void CAircraftBody::addLocalForceAtLocalPosition( const Vector3<real_Num> &force,
                                                          const Vector3<real_Num> &pos )
        {
            m_parentAircraft->addLocalForce( 0, force, pos );
        }

        void CAircraftBody::addForceAtPosition( const Vector3<real_Num> &force,
                                                const Vector3<real_Num> &pos,
                                                physics::ForceModeEnum   forceMode )
        {
            m_parentAircraft->addForce( 0, force, pos );
        }

        void CAircraftBody::addForceAtPosition( const Vector3<real_Num> &force,
                                                const Vector3<real_Num> &pos )
        {
            m_parentAircraft->addForce( 0, force, pos );
        }

        bool CAircraftBody::castLocalRay( const Ray3<real_Num>           &ray,
                                          SmartPtr<physics::IRaycastHit> &data )
        {
            WP_ASSERT( ray.isValid() );
            WP_ASSERT( data );

            auto callback = m_parentAircraft->getCallback();
            if( callback )
            {
                return callback->castLocalRay( ray, data );
            }

            return false;
        }

        bool CAircraftBody::castWorldRay( const Ray3<real_Num>           &ray,
                                          SmartPtr<physics::IRaycastHit> &data )
        {
            WP_ASSERT( ray.isValid() );
            WP_ASSERT( data );

            auto callback = m_parentAircraft->getCallback();
            if( callback )
            {
                return callback->castWorldRay( ray, data );
            }

            return false;
        }

        real_Num CAircraftBody::getMass() const
        {
            WP_ASSERT( m_mass > 0 );
            WP_ASSERT( m_mass < static_cast<real_Num>( 1e5 ) );
            return m_mass;
        }

        void CAircraftBody::setMass( real_Num mass )
        {
            WP_ASSERT( mass > 0 );
            WP_ASSERT( mass < static_cast<real_Num>( 1e5 ) );

            m_mass = mass;

            WP_ASSERT( m_mass > 0 );
            WP_ASSERT( m_mass < static_cast<real_Num>( 1e5 ) );
        }

        Vector3<real_Num> CAircraftBody::getLocalVelocity() const
        {
            return m_localVelocity;
        }

        void CAircraftBody::setLocalVelocity( const Vector3<real_Num> &localVelocity )
        {
            m_localVelocity = localVelocity;
        }

        Vector3<real_Num> CAircraftBody::getLocalAngularVelocity() const
        {
            return m_localAngularVelocity;
        }

        void CAircraftBody::setLocalAngularVelocity( const Vector3<real_Num> &localAngularVelocity )
        {
            m_localAngularVelocity = localAngularVelocity;
        }

        void CAircraftBody::addTorque( const Vector3<real_Num> &torque )
        {
            m_parentAircraft->addTorque( 0, torque );
        }

        void CAircraftBody::addLocalTorque( const Vector3<real_Num> &torque )
        {
            m_parentAircraft->addLocalTorque( 0, torque );
        }

        SmartPtr<IAircraft> &CAircraftBody::getParentAircraft()
        {
            return m_parentAircraft;
        }

        const SmartPtr<IAircraft> &CAircraftBody::getParentAircraft() const
        {
            return m_parentAircraft;
        }

        void CAircraftBody::setParentAircraft( SmartPtr<IAircraft> parentAircraft )
        {
            m_parentAircraft = parentAircraft;
        }

        void CAircraftBody::update( const double &t, const double &dt )
        {
            m_velocity = m_parentAircraft->getLinearVelocity();
            m_angularVelocity = m_parentAircraft->getAngularVelocity();

            m_localVelocity = m_parentAircraft->getLocalLinearVelocity();
            m_localAngularVelocity = m_parentAircraft->getLocalAngularVelocity();

            WP_ASSERT( m_velocity.length() < 1e4 );
            WP_ASSERT( m_angularVelocity.length() < 1e4 );

            auto aircraftWorldTransform = m_parentAircraft->getWorldTransform();
            auto worldCenterOfMass = aircraftWorldTransform.transformPoint( m_parentAircraft->getCG() );
            setWorldCenterOfMass( worldCenterOfMass );
        }

        Vector3<real_Num> CAircraftBody::getPointVelocity( const Vector3<real_Num> &p )
        {
            return m_parentAircraft->getPointVelocity( p );
        }

        bool CAircraftBody::isValid() const
        {
            return m_parentAircraft != nullptr && m_worldTransform.isValid() &&
                   m_localTransform.isValid();
        }
    } // namespace vehicle
} // namespace workphone
