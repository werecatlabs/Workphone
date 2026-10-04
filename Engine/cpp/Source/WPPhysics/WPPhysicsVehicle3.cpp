#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysicsVehicle3.hpp"

namespace workphone
{
    namespace physics
    {

        CPhysicsVehicle3::CPhysicsVehicle3( SmartPtr<IRigidBody3> chassis ) :
            m_chassis( std::move( chassis ) )
        {
            setLoadingState( LoadingState::Loaded );
        }

        CPhysicsVehicle3::~CPhysicsVehicle3()
        {
            unload( nullptr );
        }

        void CPhysicsVehicle3::unload( SmartPtr<ISharedObject> )
        {
            m_wheels.clear();
            m_vehicleInput = nullptr;
            m_chassis = nullptr;
            setLoadingState( LoadingState::Unloaded );
        }

        workphone::physics::IPhysicsVehicleWheel3 *CPhysicsVehicle3::addWheel()
        {
            if( !m_chassis || m_finalized )
            {
                WP_LOG_WARNING(
                    "CPhysicsVehicle3::addWheel: wheels cannot be added after finalization." );
                return nullptr;
            }

            auto wheel = workphone::make_ptr<CPhysicsVehicleWheel>();
            wheel->setLoadingState( LoadingState::Loaded );
            auto result = wheel.get();
            m_wheels.push_back( wheel );
            return result;
        }

        workphone::physics::IPhysicsVehicleWheel3 *CPhysicsVehicle3::getWheel( u32 wheelIndex ) const
        {
            return wheelIndex < m_wheels.size() ? m_wheels[wheelIndex].get() : nullptr;
        }

        u32 CPhysicsVehicle3::getNumWheels() const
        {
            return static_cast<u32>( m_wheels.size() );
        }

        void CPhysicsVehicle3::finalize()
        {
            if( !m_chassis )
            {
                return;
            }
            if( m_wheels.empty() )
            {
                WP_LOG_WARNING( "CPhysicsVehicle3::finalize: vehicle has no wheels." );
            }
            m_finalized = true;
            m_chassis->wakeUp();
        }

        void CPhysicsVehicle3::applyEngineForce( f32 engineForce, u32 wheelIndex )
        {
            auto wheel = getNativeWheel( wheelIndex );
            if( !wheel || !m_chassis || !m_enabled )
            {
                return;
            }

            wheel->setEngineForce( static_cast<real_Num>( engineForce ) );
            const auto transform = m_chassis->getTransform();
            const auto steering =
                Quaternion<real_Num>::angleAxis( wheel->getSteering(), Vector3<real_Num>::unitY() );
            auto direction = transform.getOrientation() * ( steering * Vector3<real_Num>::unitZ() );
            if( direction.lengthSquared() > Math<real_Num>::epsilon() )
            {
                direction.normalise();
                m_chassis->addForce( direction * static_cast<real_Num>( engineForce ) );
            }
        }

        void CPhysicsVehicle3::setBrake( f32 brakeForce, u32 wheelIndex )
        {
            auto wheel = getNativeWheel( wheelIndex );
            if( !wheel || !m_chassis )
            {
                return;
            }

            wheel->setBrake( static_cast<real_Num>( brakeForce ) );
            if( brakeForce <= 0.0f )
            {
                return;
            }

            const auto mass = std::max( m_chassis->getMass(), static_cast<real_Num>( 0.001 ) );
            const auto factor =
                std::max( static_cast<real_Num>( 0.0 ),
                          static_cast<real_Num>( 1.0 ) - static_cast<real_Num>( brakeForce ) /
                                                             ( mass * static_cast<real_Num>( 60.0 ) ) );
            m_chassis->setLinearVelocity( m_chassis->getLinearVelocity() * factor );
            m_chassis->setAngularVelocity( m_chassis->getAngularVelocity() * factor );
        }

        void CPhysicsVehicle3::setSteeringValue( f32 steeringValue, u32 wheelIndex )
        {
            if( auto wheel = getNativeWheel( wheelIndex ) )
            {
                wheel->setSteering( static_cast<real_Num>( steeringValue ) );
            }
        }

        void CPhysicsVehicle3::setPosition( const Vector3<real_Num> &position )
        {
            if( m_chassis )
            {
                auto transform = m_chassis->getTransform();
                transform.setPosition( position );
                m_chassis->setTransform( transform );
            }
        }

        workphone::Vector3<workphone::real_Num> CPhysicsVehicle3::getPosition() const
        {
            return m_chassis ? m_chassis->getTransform().getPosition() : Vector3<real_Num>::zero();
        }

        void CPhysicsVehicle3::setOrientation( const Quaternion<real_Num> &orientation )
        {
            if( m_chassis )
            {
                auto transform = m_chassis->getTransform();
                transform.setOrientation( orientation );
                m_chassis->setTransform( transform );
            }
        }

        workphone::Quaternion<workphone::real_Num> CPhysicsVehicle3::getOrientation() const
        {
            return m_chassis ? m_chassis->getTransform().getOrientation()
                             : Quaternion<real_Num>::identity();
        }

        void CPhysicsVehicle3::setVelocity( const Vector3<real_Num> &velocity )
        {
            if( m_chassis )
            {
                m_chassis->setLinearVelocity( velocity );
            }
        }

        workphone::Vector3<workphone::real_Num> CPhysicsVehicle3::getVelocity() const
        {
            return m_chassis ? m_chassis->getLinearVelocity() : Vector3<real_Num>::zero();
        }

        void CPhysicsVehicle3::setMaterialId( u32 materialId )
        {
            m_materialId = materialId;
        }

        u32 CPhysicsVehicle3::getMaterialId() const
        {
            return m_materialId;
        }

        workphone::AABB3F CPhysicsVehicle3::getLocalAABB() const
        {
            return m_chassis ? toFloatBounds( m_chassis->getLocalAABB() ) : AABB3F();
        }

        workphone::AABB3F CPhysicsVehicle3::getWorldAABB() const
        {
            return m_chassis ? toFloatBounds( m_chassis->getWorldAABB() ) : AABB3F();
        }

        void CPhysicsVehicle3::setEnabled( bool enabled )
        {
            m_enabled = enabled;
            if( m_chassis )
            {
                m_chassis->setEnabled( enabled );
            }
        }

        bool CPhysicsVehicle3::isEnabled() const
        {
            return m_enabled && m_chassis && m_chassis->isEnabled();
        }

        const workphone::SmartPtr<workphone::physics::IPhysicsVehicleInput3> &
        CPhysicsVehicle3::getVehicleInput() const
        {
            return m_vehicleInput;
        }

        workphone::SmartPtr<workphone::physics::IPhysicsVehicleInput3> &
        CPhysicsVehicle3::getVehicleInput()
        {
            return m_vehicleInput;
        }

        workphone::Array<workphone::Transform3F> CPhysicsVehicle3::getWheelTransformations() const
        {
            Array<Transform3F> transforms;
            if( !m_chassis || m_wheels.empty() )
            {
                return transforms;
            }

            auto localBounds = m_chassis->getLocalAABB();
            Vector3<real_Num> minimum( -1.0, -0.5, -2.0 );
            Vector3<real_Num> maximum( 1.0, 0.5, 2.0 );
            if( !localBounds.isNull() && !localBounds.isInfinite() )
            {
                minimum = localBounds.getMinimum();
                maximum = localBounds.getMaximum();
            }

            const auto chassisTransform = m_chassis->getTransform();
            const auto center = ( minimum + maximum ) * static_cast<real_Num>( 0.5 );
            const auto half = ( maximum - minimum ) * static_cast<real_Num>( 0.5 );
            const auto wheelCount = m_wheels.size();
            transforms.reserve( wheelCount );

            for( size_t i = 0; i < wheelCount; ++i )
            {
                const auto wheel = m_wheels[i];
                const auto side =
                    ( i % 2 ) == 0 ? static_cast<real_Num>( -1.0 ) : static_cast<real_Num>( 1.0 );
                const auto row = i / 2;
                const auto rowCount = std::max<size_t>( 1, ( wheelCount + 1 ) / 2 );
                const auto rowFraction =
                    rowCount == 1 ? static_cast<real_Num>( 0.5 )
                                  : static_cast<real_Num>( row ) / static_cast<real_Num>( rowCount - 1 );
                const auto localPosition =
                    Vector3<real_Num>( center.X() + side * half.X(),
                                       minimum.Y() - wheel->getRadius() * static_cast<real_Num>( 0.25 ),
                                       maximum.Z() - rowFraction * ( maximum.Z() - minimum.Z() ) );
                const auto worldPosition = chassisTransform.transformPoint( localPosition );
                const auto steering =
                    Quaternion<real_Num>::angleAxis( wheel->getSteering(), Vector3<real_Num>::unitY() );
                const auto worldOrientation = chassisTransform.getOrientation() * steering;

                auto inContact = false;
                if( auto scene = m_chassis->getScene() )
                {
                    Vector3<real_Num> hitPosition;
                    Vector3<real_Num> hitNormal;
                    SmartPtr<ISharedObject> object;
                    const auto down =
                        chassisTransform.getOrientation() *
                        Vector3<real_Num>( 0.0,
                                           -( wheel->getRadius() + wheel->getMaxSuspensionTravelCm() /
                                                                       static_cast<real_Num>( 100.0 ) ),
                                           0.0 );
                    inContact = scene->intersects( worldPosition, worldPosition + down, hitPosition,
                                                   hitNormal, object ) &&
                                object.get() != m_chassis.get();
                }
                wheel->setInContact( inContact );

                transforms.emplace_back( Vector3F( static_cast<f32>( worldPosition.X() ),
                                                   static_cast<f32>( worldPosition.Y() ),
                                                   static_cast<f32>( worldPosition.Z() ) ),
                                         QuaternionF( static_cast<f32>( worldOrientation.W() ),
                                                      static_cast<f32>( worldOrientation.X() ),
                                                      static_cast<f32>( worldOrientation.Y() ),
                                                      static_cast<f32>( worldOrientation.Z() ) ) );
            }
            return transforms;
        }

        SmartPtr<IRigidBody3> CPhysicsVehicle3::getChassis() const
        {
            return m_chassis;
        }

        CPhysicsVehicleWheel *CPhysicsVehicle3::getNativeWheel(
            u32 wheelIndex ) const
        {
            return wheelIndex < m_wheels.size() ? m_wheels[wheelIndex].get() : nullptr;
        }

    }  // namespace physics
}  // namespace workphone
