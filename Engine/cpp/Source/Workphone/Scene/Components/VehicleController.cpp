#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/VehicleController.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IJoystickState.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/Interface/Vehicle/IAircraftCallback.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    namespace
    {
        /**
         * Bridges the callback-driven WPVehiclePhysics aerodynamic model to a
         * scene Rigidbody without exposing a concrete physics-plugin type to
         * the core Workphone library.
         */
        class AircraftSceneCallback final : public vehicle::IAircraftCallback
        {
        public:
            String getData( const String &filePath ) override
            {
                return {};
            }

            void addForce( s32 bodyId, const Vector3<real_Num> &force,
                           const Vector3<real_Num> &pos ) override
            {
                addForce( bodyId, force );
            }

            void addForce( s32 bodyId, const Vector3<real_Num> &force ) override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        chassis->addForce( force );
                    }
                }
            }

            void addTorque( s32 bodyId, const Vector3<real_Num> &torque ) override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        chassis->addTorque( torque );
                    }
                }
            }

            void addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                                const Vector3<real_Num> &pos ) override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        const auto orientation = getOrientation();
                        const auto worldForce = orientation * force;
                        chassis->addForce( worldForce );

                        const auto worldOffset = orientation * pos;
                        const auto torque = worldOffset.crossProduct( worldForce );
                        chassis->addTorque( torque );
                    }
                }
            }

            void addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque ) override
            {
                addTorque( bodyId, getOrientation() * torque );
            }

            Vector3<real_Num> getAngularVelocity() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        return chassis->getAngularVelocity();
                    }
                }
                return Vector3<real_Num>::zero();
            }

            Vector3<real_Num> getLinearVelocity() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        return chassis->getLinearVelocity();
                    }
                }
                return Vector3<real_Num>::zero();
            }

            Vector3<real_Num> getLocalAngularVelocity() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        return chassis->getLocalAngularVelocity();
                    }
                }
                return Vector3<real_Num>::zero();
            }

            Vector3<real_Num> getLocalLinearVelocity() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        return chassis->getLocalLinearVelocity();
                    }
                }
                return Vector3<real_Num>::zero();
            }

            Vector3<real_Num> getPosition() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        if( auto rigidDynamic = chassis->getRigidDynamic() )
                        {
                            return rigidDynamic->getTransform().getPosition();
                        }
                    }
                    if( auto actor = owner->getActor() )
                    {
                        return actor->getPosition();
                    }
                }
                return Vector3<real_Num>::zero();
            }

            Vector3<real_Num> getScale() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto actor = owner->getActor() )
                    {
                        return actor->getScale();
                    }
                }
                return Vector3<real_Num>::unit();
            }

            Quaternion<real_Num> getOrientation() const override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        if( auto rigidDynamic = chassis->getRigidDynamic() )
                        {
                            return rigidDynamic->getTransform().getOrientation();
                        }
                    }
                    if( auto actor = owner->getActor() )
                    {
                        return actor->getOrientation();
                    }
                }
                return Quaternion<real_Num>::identity();
            }

            void displayLocalVector( s32 bodyId, const Vector3<real_Num> &value,
                                     const Vector3<real_Num> &origin, s32 colour ) const override
            {
                displayLocalVector( bodyId, 0, value, origin, colour );
            }

            void displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &value,
                                const Vector3<real_Num> &origin, s32 colour ) const override
            {
                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
                    {
                        if( auto debug = graphicsSystem->getDebug() )
                        {
                            debug->drawLine( id, value, origin, colour );
                        }
                    }
                }
            }

            void displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &value,
                                     const Vector3<real_Num> &origin, s32 colour ) const override
            {
                const auto orientation = getOrientation();
                const auto position = getPosition();
                displayVector( bodyId, id, position + orientation * value,
                               position + orientation * origin, colour );
            }

            void *getCallbackFunction() const override
            {
                return m_callbackFunction;
            }

            void setCallbackFunction( void *callbackFunction ) override
            {
                m_callbackFunction = callbackFunction;
            }

            void *getCallbackDataFunction() const override
            {
                return m_callbackDataFunction;
            }

            void setCallbackDataFunction( void *callbackDataFunction ) override
            {
                m_callbackDataFunction = callbackDataFunction;
            }

            FixedArray<f32, 8> getInputData() const override
            {
                auto input = FixedArray<f32, 8>();
                if( auto owner = getOwner() )
                {
                    input[0] = owner->getAircraftThrottle();
                    input[1] = owner->getAircraftRoll();
                    input[2] = owner->getAircraftPitch();
                    input[3] = owner->getAircraftYaw();
                }
                return input;
            }

            FixedArray<f32, 11> getControlAngles() const override
            {
                auto angles = FixedArray<f32, 11>();
                if( auto owner = getOwner() )
                {
                    constexpr auto maxPrimaryDeflection = 30.0f;
                    constexpr auto maxElevatorDeflection = 25.0f;
                    angles[0] = owner->getAircraftRoll() * maxPrimaryDeflection;
                    angles[1] = -angles[0];
                    angles[2] = owner->getAircraftPitch() * maxElevatorDeflection;
                    angles[3] = owner->getAircraftYaw() * maxPrimaryDeflection;
                    angles[5] = angles[3];
                }
                return angles;
            }

            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &point ) override
            {
                if( auto owner = getOwner() )
                {
                    if( auto chassis = owner->getChassis() )
                    {
                        return chassis->getPointVelocity( point );
                    }
                }
                return Vector3<real_Num>::zero();
            }

            bool castLocalRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override
            {
                const auto origin = getPosition() + getOrientation() * ray.getOrigin();
                const auto direction = getOrientation() * ray.getDirection();
                return castWorldRay( Ray3<real_Num>( origin, direction ), data );
            }

            bool castWorldRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override
            {
                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
                    {
                        if( auto physicsScene = physicsManager->getPhysicsScene() )
                        {
                            return physicsScene->castRay( ray, data );
                        }
                    }
                }
                return false;
            }

            SmartPtr<VehicleController> getOwner() const
            {
                return m_owner.lock();
            }

            void setOwner( SmartPtr<VehicleController> owner )
            {
                m_owner = owner;
            }

        private:
            WeakPtr<VehicleController> m_owner;
            void *m_callbackFunction = nullptr;
            void *m_callbackDataFunction = nullptr;
        };
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, VehicleController, Component );

    const String VehicleController::cgStr = "cg";
    const String VehicleController::massStr = "mass";
    const String VehicleController::moiStr = "moi";
    const String VehicleController::collisionStr = "Collision";
    const String VehicleController::chassisStr = "Chassis";
    const String VehicleController::resetStr = "Reset";
    const String VehicleController::resetTransformStr = "Reset Transform";
    const String VehicleController::tireModelStr = "Tire Model";
    const String VehicleController::vehicleTypeStr = "Vehicle Type";
    const String VehicleController::airDensityStr = "Air Density";
    const String VehicleController::sectionMultiplierStr = "Aerodynamic Section Multiplier";
    const String VehicleController::rollwiseDampingStr = "Rollwise Damping";

    VehicleController::VehicleController() = default;

    VehicleController::~VehicleController() = default;

    void VehicleController::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );

        if( auto actor = getActor() )
        {
            if( !m_collision )
            {
                m_collision = actor->getComponent<Collision>();
            }
            if( !m_chassis )
            {
                m_chassis = actor->getComponent<Rigidbody>();
            }
        }

        if( m_chassis )
        {
            m_chassis->setMass( m_mass );
            m_chassis->setMassSpaceInertiaTensor( m_moi );
        }

        setLoadingState( LoadingState::Loaded );

        if( m_vehicleType == VehicleType::Aircraft )
        {
            createAircraftController();
        }
    }

    void VehicleController::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        destroyAircraftController( data );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void VehicleController::update()
    {
        Component::update();

        if( m_vehicleType == VehicleType::Aircraft )
        {
            if( !m_aircraftController )
            {
                createAircraftController();
            }
            applyAircraftConfiguration();
        }
    }

    SmartPtr<Properties> VehicleController::getProperties() const
    {
        auto properties = Component::getProperties();

        properties->setProperty( cgStr, m_cg );
        properties->setProperty( massStr, m_mass );
        properties->setProperty( moiStr, m_moi );

        properties->setPropertyAsType<Collision>( collisionStr, m_collision );
        properties->setPropertyAsType<Rigidbody>( chassisStr, m_chassis );

        auto tireModelsEnumTypes = Array<String>( { "Simple", "Pacejka", "Brush" } );
        properties->setPropertyAsEnum( tireModelStr, static_cast<s32>( m_tireModel ),
                                       tireModelsEnumTypes );

        properties->setPropertyAsEnum( vehicleTypeStr, static_cast<s32>( m_vehicleType ),
                                       Array<String>( { "Car", "Aircraft" } ) );
        properties->setProperty( airDensityStr, m_airDensity );
        properties->setProperty( sectionMultiplierStr, m_sectionMultiplier );
        properties->setProperty( rollwiseDampingStr, m_rollwiseDamping );

        properties->setButtonPressed( resetStr, false );
        properties->setButtonPressed( resetTransformStr, false );

        return properties;
    }

    void VehicleController::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        auto actor = getActor();

        properties->getPropertyValue( cgStr, m_cg );
        properties->getPropertyValue( massStr, m_mass );
        properties->getPropertyValue( moiStr, m_moi );

        properties->getPropertyAsType<Collision>( collisionStr, m_collision );
        properties->getPropertyAsType<Rigidbody>( chassisStr, m_chassis );

        s32 tireModel = static_cast<s32>( m_tireModel );
        properties->getPropertyValue( tireModelStr, tireModel );
        m_tireModel = static_cast<TireModel>( tireModel );

        auto vehicleType = static_cast<s32>( m_vehicleType );
        properties->getPropertyValue( vehicleTypeStr, vehicleType );
        properties->getPropertyValue( airDensityStr, m_airDensity );
        properties->getPropertyValue( sectionMultiplierStr, m_sectionMultiplier );
        properties->getPropertyValue( rollwiseDampingStr, m_rollwiseDamping );

        m_mass = Math<real_Num>::clamp( m_mass, 0.1f, 100000.0f );
        m_airDensity = Math<real_Num>::clamp( m_airDensity, static_cast<real_Num>( 0.05 ),
                                              static_cast<real_Num>( 5.0 ) );
        m_sectionMultiplier = Math<real_Num>::clamp( m_sectionMultiplier, static_cast<real_Num>( 0.01 ),
                                                     static_cast<real_Num>( 1000.0 ) );
        m_rollwiseDamping = Math<real_Num>::clamp( m_rollwiseDamping, static_cast<real_Num>( 0.0 ),
                                                   static_cast<real_Num>( 100.0 ) );

        setVehicleType( vehicleType );

        if( properties->isButtonPressed( resetStr ) )
        {
            if( actor )
            {
                auto wheelControllers = actor->getComponentsInChildren<WheelController>();
                for( auto wheelController : wheelControllers )
                {
                    wheelController->reset();
                }
            }
        }

        if( properties->isButtonPressed( resetTransformStr ) )
        {
            if( actor )
            {
                auto resetTransform = getResetTransform();
                auto position = resetTransform.getPosition();
                auto orientation = resetTransform.getOrientation();

                actor->setLocalPosition( position );
                actor->setLocalOrientation( orientation );

                if( auto chassis = getChassis() )
                {
                    chassis->setLinearVelocity( Vector3<real_Num>::zero() );
                    chassis->setAngularVelocity( Vector3<real_Num>::zero() );
                }
            }
        }

        if( m_chassis )
        {
            m_chassis->setMass( m_mass );
            m_chassis->setMassSpaceInertiaTensor( m_moi );
        }

        applyAircraftConfiguration();

        if( actor )
        {
            auto components = actor->getAllComponentsAndInChildren<WheelController>();
            for( auto component : components )
            {
                component->setTireModel( m_tireModel );
            }
        }
    }

    Array<SmartPtr<ISharedObject>> VehicleController::getChildObjects() const
    {
        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( 7 );

        objects.emplace_back( m_inputListener );
        objects.emplace_back( m_collision );
        objects.emplace_back( m_chassis );
        objects.emplace_back( m_aircraftController );
        objects.emplace_back( m_aircraftCallback );

        return objects;
    }

    SmartPtr<Rigidbody> VehicleController::getChassis() const
    {
        return m_chassis;
    }

    void VehicleController::setChassis( SmartPtr<Rigidbody> chassis )
    {
        m_chassis = chassis;
        if( m_chassis )
        {
            m_chassis->setMass( m_mass );
            m_chassis->setMassSpaceInertiaTensor( m_moi );
        }
        applyAircraftConfiguration();
    }

    Vector3<real_Num> VehicleController::getMOI() const
    {
        return m_moi;
    }

    void VehicleController::setMOI( const Vector3<real_Num> &moi )
    {
        m_moi = moi;

        if( m_chassis )
        {
            m_chassis->setMassSpaceInertiaTensor( m_moi );
        }
    }

    void VehicleController::setMass( f32 mass )
    {
        m_mass = Math<f32>::clamp( mass, 0.1f, 100000.0f );
        if( m_chassis )
        {
            m_chassis->setMass( m_mass );
        }
        applyAircraftConfiguration();
    }

    f32 VehicleController::getMass() const
    {
        return m_mass;
    }

    void VehicleController::setCg( const Vector3<real_Num> &cg )
    {
        m_cg = cg;
        applyAircraftConfiguration();
    }

    Vector3<real_Num> VehicleController::getCg() const
    {
        return m_cg;
    }

    void VehicleController::setCollision( SmartPtr<Collision> collision )
    {
        m_collision = collision;
    }

    SmartPtr<Collision> VehicleController::getCollision() const
    {
        return m_collision;
    }

    void VehicleController::setResetTransform( Transform3<real_Num> resetTransform )
    {
        m_resetTransform = resetTransform;
    }

    Transform3<real_Num> VehicleController::getResetTransform() const
    {
        return m_resetTransform;
    }

    FSMReturnType VehicleController::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                auto actor = getActor();
                WP_ASSERT( actor );

                if( actor )
                {
                    if( auto transform = actor->getTransform() )
                    {
                        auto resetTransform = transform->getLocalTransform();
                        setResetTransform( resetTransform );
                    }
                }
            }
            break;
            default:
            {
            }
            };
        }
        break;
        default:
        {
        }
        }
        return FSMReturnType::Ok;
    }

    TireModel VehicleController::getTireModel() const
    {
        return m_tireModel;
    }

    void VehicleController::setTireModel( TireModel tireModel )
    {
        m_tireModel = tireModel;

        if( auto actor = getActor() )
        {
            auto components = actor->getAllComponentsAndInChildren<WheelController>();
            for( auto component : components )
            {
                component->setTireModel( tireModel );
            }
        }
    }

    s32 VehicleController::getVehicleType() const
    {
        return static_cast<s32>( m_vehicleType );
    }

    void VehicleController::setVehicleType( s32 vehicleType )
    {
        if( vehicleType < static_cast<s32>( VehicleType::Car ) ||
            vehicleType > static_cast<s32>( VehicleType::Aircraft ) )
        {
            return;
        }

        const auto newType = static_cast<VehicleType>( vehicleType );
        if( newType == m_vehicleType )
        {
            if( newType == VehicleType::Aircraft && isLoaded() && !m_aircraftController )
            {
                createAircraftController();
            }
            return;
        }

        if( m_vehicleType == VehicleType::Aircraft )
        {
            destroyAircraftController();
        }

        m_vehicleType = newType;
        if( m_vehicleType == VehicleType::Aircraft && isLoaded() )
        {
            createAircraftController();
        }
    }

    bool VehicleController::isAircraftPhysicsEnabled() const
    {
        return m_vehicleType == VehicleType::Aircraft && m_aircraftController != nullptr;
    }

    SmartPtr<vehicle::IAircraft> VehicleController::getAircraftController() const
    {
        return m_aircraftController;
    }

    void VehicleController::setAircraftControls( f32 throttle, f32 pitch, f32 roll, f32 yaw )
    {
        setAircraftThrottle( throttle );
        setAircraftPitch( pitch );
        setAircraftRoll( roll );
        setAircraftYaw( yaw );
        applyAircraftConfiguration();
    }

    f32 VehicleController::getAircraftThrottle() const
    {
        return m_aircraftThrottle;
    }

    void VehicleController::setAircraftThrottle( f32 throttle )
    {
        m_aircraftThrottle = Math<f32>::clamp( throttle, 0.0f, 1.0f );
    }

    f32 VehicleController::getAircraftPitch() const
    {
        return m_aircraftPitch;
    }

    void VehicleController::setAircraftPitch( f32 pitch )
    {
        m_aircraftPitch = Math<f32>::clamp( pitch, -1.0f, 1.0f );
    }

    f32 VehicleController::getAircraftRoll() const
    {
        return m_aircraftRoll;
    }

    void VehicleController::setAircraftRoll( f32 roll )
    {
        m_aircraftRoll = Math<f32>::clamp( roll, -1.0f, 1.0f );
    }

    f32 VehicleController::getAircraftYaw() const
    {
        return m_aircraftYaw;
    }

    void VehicleController::setAircraftYaw( f32 yaw )
    {
        m_aircraftYaw = Math<f32>::clamp( yaw, -1.0f, 1.0f );
    }

    real_Num VehicleController::getAirDensity() const
    {
        return m_airDensity;
    }

    void VehicleController::setAirDensity( real_Num airDensity )
    {
        m_airDensity = Math<real_Num>::clamp( airDensity, static_cast<real_Num>( 0.05 ),
                                              static_cast<real_Num>( 5.0 ) );
        applyAircraftConfiguration();
    }

    real_Num VehicleController::getAerodynamicSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void VehicleController::setAerodynamicSectionMultiplier( real_Num sectionMultiplier )
    {
        m_sectionMultiplier = Math<real_Num>::clamp( sectionMultiplier, static_cast<real_Num>( 0.01 ),
                                                     static_cast<real_Num>( 1000.0 ) );
        applyAircraftConfiguration();
    }

    real_Num VehicleController::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void VehicleController::setRollwiseDamping( real_Num rollwiseDamping )
    {
        m_rollwiseDamping = Math<real_Num>::clamp( rollwiseDamping, static_cast<real_Num>( 0.0 ),
                                                   static_cast<real_Num>( 100.0 ) );
        applyAircraftConfiguration();
    }

    void VehicleController::createAircraftController()
    {
        if( m_aircraftController || m_vehicleType != VehicleType::Aircraft )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            return;
        }

        auto aircraft = factoryManager->make_object<vehicle::IAircraft>();
        if( !aircraft )
        {
            WP_LOG_ERROR(
                "VehicleController could not create IAircraft. Ensure WPVehiclePhysics is loaded." );
            return;
        }

        auto callback = workphone::make_ptr<AircraftSceneCallback>();
        callback->setOwner( this );

        aircraft->load( nullptr );
        aircraft->setVehicleCallback( callback );
        aircraft->setCallback( callback );

        m_aircraftCallback = callback;
        m_aircraftController = aircraft;
        applyAircraftConfiguration();

        if( auto vehicleManager = applicationManager->getVehicleManager() )
        {
            vehicleManager->addVehicle( m_aircraftController );
        }
    }

    void VehicleController::destroyAircraftController( SmartPtr<ISharedObject> data )
    {
        if( !m_aircraftController )
        {
            m_aircraftCallback = nullptr;
            return;
        }

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto vehicleManager = applicationManager->getVehicleManager() )
            {
                vehicleManager->removeVehicle( m_aircraftController );
            }
        }

        m_aircraftController->setVehicleCallback( nullptr );
        m_aircraftController->setCallback( nullptr );
        m_aircraftController->unload( data );
        m_aircraftController = nullptr;
        m_aircraftCallback = nullptr;
    }

    void VehicleController::applyAircraftConfiguration()
    {
        if( !m_aircraftController )
        {
            return;
        }

        m_aircraftController->setMass( m_mass );
        m_aircraftController->setAirDensity( m_airDensity );
        m_aircraftController->setSectionMultiplier( m_sectionMultiplier );
        m_aircraftController->setRollwiseDamping( m_rollwiseDamping );
        m_aircraftController->setChannel( 0, m_aircraftThrottle );
        m_aircraftController->setChannel( 1, m_aircraftRoll );
        m_aircraftController->setChannel( 2, m_aircraftPitch );
        m_aircraftController->setChannel( 3, m_aircraftYaw );
    }

}  // namespace workphone::scene
