#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/VehicleBody.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Interface/System/ITimer.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, VehicleBody, VehicleComponent<IVehicleBody> );

        VehicleBody::VehicleBody() = default;

        VehicleBody::~VehicleBody() = default;

        void VehicleBody::unload( SmartPtr<ISharedObject> data )
        {
            m_parentVehicle = nullptr;
            VehicleComponent<IVehicleBody>::unload( data );
        }

        Vector3<real_Num> VehicleBody::getVelocity() const
        {
            return m_velocity;
        }

        void VehicleBody::setVelocity( const Vector3<real_Num> &velocity )
        {
            m_velocity = velocity;
        }

        Vector3<real_Num> VehicleBody::getAngularVelocity() const
        {
            return m_angularVelocity;
        }

        void VehicleBody::setAngularVelocity( const Vector3<real_Num> &angularVelocity )
        {
            m_angularVelocity = angularVelocity;
        }

        Vector3<real_Num> VehicleBody::getWorldCenterOfMass() const
        {
            return m_worldCenterOfMass;
        }

        void VehicleBody::setWorldCenterOfMass( const Vector3<real_Num> &force )
        {
            m_worldCenterOfMass = force;
        }

        void VehicleBody::addLocalForceAtPosition( const Vector3<real_Num> &force,
                                                   const Vector3<real_Num> &pos,
                                                   physics::ForceModeEnum forceMode )
        {
            m_parentVehicle->addLocalForce( 0, force, pos );
        }

        void VehicleBody::addLocalForceAtLocalPosition( const Vector3<real_Num> &force,
                                                        const Vector3<real_Num> &pos )
        {
            m_parentVehicle->addLocalForce( 0, force, pos );
        }

        void VehicleBody::addForceAtPosition( const Vector3<real_Num> &force,
                                              const Vector3<real_Num> &pos,
                                              physics::ForceModeEnum forceMode )
        {
            m_parentVehicle->addForce( 0, force, pos );
        }

        void VehicleBody::addForceAtPosition( const Vector3<real_Num> &force,
                                              const Vector3<real_Num> &pos )
        {
            m_parentVehicle->addForce( 0, force, pos );
        }

        bool VehicleBody::castLocalRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data )
        {
            WP_ASSERT( ray.isValid() );
            WP_ASSERT( data );

            auto callback = m_parentVehicle->getVehicleCallback();
            if( callback )
            {
                return callback->castLocalRay( ray, data );
            }

            return false;
        }

        bool VehicleBody::castWorldRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data )
        {
            WP_ASSERT( ray.isValid() );
            WP_ASSERT( data );

            auto callback = m_parentVehicle->getVehicleCallback();
            if( callback )
            {
                return callback->castWorldRay( ray, data );
            }

            return false;
        }

        real_Num VehicleBody::getMass() const
        {
            WP_ASSERT( m_mass > 0 );
            WP_ASSERT( m_mass < static_cast<real_Num>( 1e10 ) );
            return m_mass;
        }

        void VehicleBody::setMass( real_Num mass )
        {
            WP_ASSERT( mass > 0 );
            WP_ASSERT( mass < static_cast<real_Num>( 1e10 ) );

            m_mass = mass;

            WP_ASSERT( m_mass > 0 );
            WP_ASSERT( m_mass < static_cast<real_Num>( 1e10 ) );
        }

        Vector3<real_Num> VehicleBody::getLocalVelocity() const
        {
            return m_localVelocity;
        }

        void VehicleBody::setLocalVelocity( const Vector3<real_Num> &localVelocity )
        {
            m_localVelocity = localVelocity;
        }

        Vector3<real_Num> VehicleBody::getLocalAngularVelocity() const
        {
            return m_localAngularVelocity;
        }

        void VehicleBody::setLocalAngularVelocity( const Vector3<real_Num> &localAngularVelocity )
        {
            m_localAngularVelocity = localAngularVelocity;
        }

        void VehicleBody::addTorque( const Vector3<real_Num> &torque )
        {
            m_parentVehicle->addTorque( 0, torque );
        }

        void VehicleBody::addLocalTorque( const Vector3<real_Num> &localTorque )
        {
            m_parentVehicle->addLocalTorque( 0, localTorque );
        }

        SmartPtr<IVehicle> &VehicleBody::getParentVehicle()
        {
            return m_parentVehicle;
        }

        const SmartPtr<IVehicle> &VehicleBody::getParentVehicle() const
        {
            return m_parentVehicle;
        }

        void VehicleBody::setParentVehicle( SmartPtr<IVehicle> parentVehicle )
        {
            m_parentVehicle = parentVehicle;
        }

        void VehicleBody::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            m_velocity = m_parentVehicle->getLinearVelocity();
            m_angularVelocity = m_parentVehicle->getAngularVelocity();

            m_localVelocity = m_parentVehicle->getLocalLinearVelocity();
            m_localAngularVelocity = m_parentVehicle->getLocalAngularVelocity();

            WP_ASSERT( m_velocity.length() < 1e4 );
            WP_ASSERT( m_angularVelocity.length() < 1e4 );

            auto worldTransform = m_parentVehicle->getWorldTransform();
            auto worldCenterOfMass = worldTransform.transformPoint( m_parentVehicle->getCG() );

            setWorldCenterOfMass( worldCenterOfMass );
        }

        Vector3<real_Num> VehicleBody::getPointVelocity( const Vector3<real_Num> &p )
        {
            return m_parentVehicle->getPointVelocity( p );
        }

        bool VehicleBody::isValid() const
        {
            return m_parentVehicle != nullptr;
        }
    }  // namespace vehicle
}  // namespace workphone
