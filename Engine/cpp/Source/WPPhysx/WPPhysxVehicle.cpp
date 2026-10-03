#include "WPPhysx/WPPhysxPCH.hpp"
#include "WPPhysx/WPPhysxVehicle.hpp"
#include "WPPhysx/WPPhysxVehicleInput.hpp"
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

using namespace physx;

PxVehicleKeySmoothingData gKeySmoothingData = { {
                                                    3.0f,  // rise rate eANALOG_INPUT_ACCEL
                                                    3.0f,  // rise rate eANALOG_INPUT_BRAKE
                                                    10.0f, // rise rate eANALOG_INPUT_HANDBRAKE
                                                    2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
                                                    2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
                                                },
                                                {
                                                    5.0f,  // fall rate eANALOG_INPUT__ACCEL
                                                    5.0f,  // fall rate eANALOG_INPUT__BRAKE
                                                    10.0f, // fall rate eANALOG_INPUT__HANDBRAKE
                                                    5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
                                                    5.0f   // fall rate eANALOG_INPUT_STEER_RIGHT
                                                } };

PxVehiclePadSmoothingData gCarPadSmoothingData = { {
                                                       6.0f,  // rise rate eANALOG_INPUT_ACCEL
                                                       6.0f,  // rise rate eANALOG_INPUT_BRAKE
                                                       12.0f, // rise rate eANALOG_INPUT_HANDBRAKE
                                                       2.5f,  // rise rate eANALOG_INPUT_STEER_LEFT
                                                       2.5f,  // rise rate eANALOG_INPUT_STEER_RIGHT
                                                   },
                                                   {
                                                       10.0f, // fall rate eANALOG_INPUT_ACCEL
                                                       10.0f, // fall rate eANALOG_INPUT_BRAKE
                                                       12.0f, // fall rate eANALOG_INPUT_HANDBRAKE
                                                       5.0f,  // fall rate eANALOG_INPUT_STEER_LEFT
                                                       5.0f   // fall rate eANALOG_INPUT_STEER_RIGHT
                                                   } };

PxF32 gSteerVsForwardSpeedData[2 * 8] = { 0.0f,       0.75f,      5.0f,       0.75f,
                                          30.0f,      0.125f,     120.0f,     0.1f,
                                          PX_MAX_F32, PX_MAX_F32, PX_MAX_F32, PX_MAX_F32,
                                          PX_MAX_F32, PX_MAX_F32, PX_MAX_F32, PX_MAX_F32 };
PxFixedSizeLookupTable<8> gSteerVsForwardSpeedTable( gSteerVsForwardSpeedData, 4 );

namespace workphone::physics
{

    PhysxVehicle3::PhysxVehicle3()
    {
        m_vehicleInput = SmartPtr<IPhysicsVehicleInput3>( new PhysxVehicleInput );
    }

    PhysxVehicle3::~PhysxVehicle3()
    {
        unload( nullptr );
    }

    void PhysxVehicle3::initialise( SmartPtr<IBuildDirector> objectTemplate )
    {
    }

    void PhysxVehicle3::update( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        PxVehicleDriveDynData &driveDynData = m_vehicle->mDriveDynData;

        driveDynData.forceGearChange( PxVehicleGearsData::eFIRST );

        SmartPtr<PhysxVehicleInput>  input; // = m_vehicleInput;
        PxVehicleDrive4WRawInputData pxVehicleInput = *input->getInputs();
        PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(
            gKeySmoothingData, gSteerVsForwardSpeedTable, pxVehicleInput,
            static_cast<physx::PxReal>( dt ), false, *m_vehicle );

#define PX_MAX_NUM_WHEELS 4
        const PxRigidDynamic *actor = m_vehicle->getRigidDynamicActor();
        PxShape              *shapeBuffer[PX_MAX_NUM_WHEELS];
        actor->getShapes( shapeBuffer, m_vehicle->mWheelsSimData.getNbWheels() );
        const PxTransform vehGlobalPose = actor->getGlobalPose();
        const PxU32       numWheels = m_vehicle->mWheelsSimData.getNbWheels();

        m_wheelTransforms.resize( 0 );
        for( PxU32 j = 0; j < numWheels; j++ )
        {
            const PxTransform wheelTransform = shapeBuffer[j]->getLocalPose();

            Transform3F transform;
            // transform.position = Vector3F(wheelTransform.p.x, wheelTransform.p.y, wheelTransform.p.z);
            // transform.orientation = QuaternionF(wheelTransform.q.w, wheelTransform.q.x,
            // wheelTransform.q.y, wheelTransform.q.z);
            m_wheelTransforms.push_back( transform );
        }

        PxTransform transform = m_vehicle->getRigidDynamicActor()->getGlobalPose();
        m_position = Vector3F( transform.p.x, transform.p.y, transform.p.z );
        m_orientation = QuaternionF( transform.q.w, transform.q.x, transform.q.y, transform.q.z );
    }

    auto PhysxVehicle3::addWheel() -> IPhysicsVehicleWheel3 *
    {
        return nullptr;
    }

    auto PhysxVehicle3::getWheel( u32 wheelIndex ) const -> IPhysicsVehicleWheel3 *
    {
        return nullptr;
    }

    auto PhysxVehicle3::getNumWheels() const -> u32
    {
        return 0;
    }

    void PhysxVehicle3::finalize()
    {
    }

    void PhysxVehicle3::applyEngineForce( f32 engineForce, u32 wheelIndex )
    {
    }

    void PhysxVehicle3::setBrake( f32 brakeForce, u32 wheelIndex )
    {
    }

    void PhysxVehicle3::setSteeringValue( f32 steeringValue, u32 wheelIndex )
    {
    }

    void PhysxVehicle3::setPosition( const Vector3F &position )
    {
        // RecursiveMutex::ScopedLock lock(PhysxMutex);

        // WP_ASSERT(eVEHICLE_TYPE_DRIVE4W==m_vehicle->getVehicleType());
        auto vehDrive4W = m_vehicle;
        // Set the car back to its rest state.
        vehDrive4W->setToRestState();
        // Set the car to first gear.
        vehDrive4W->mDriveDynData.forceGearChange( PxVehicleGearsData::eFIRST );

        PxTransform transform = m_vehicle->getRigidDynamicActor()->getGlobalPose();
        transform.p = PxVec3( position.X(), position.Y(), position.Z() );
        m_vehicle->getRigidDynamicActor()->setGlobalPose( transform );
    }

    auto PhysxVehicle3::getPosition() const -> Vector3F
    {
        return m_position;
    }

    void PhysxVehicle3::setOrientation( const QuaternionF &orientation )
    {
    }

    auto PhysxVehicle3::getOrientation() const -> QuaternionF
    {
        return m_orientation;
    }

    void PhysxVehicle3::setVelocity( const Vector3F &velocity )
    {
    }

    auto PhysxVehicle3::getVelocity() const -> Vector3F
    {
        return Vector3F::zero();
    }

    void PhysxVehicle3::setMaterialId( u32 materialId )
    {
    }

    auto PhysxVehicle3::getMaterialId() const -> u32
    {
        return 0;
    }

    auto PhysxVehicle3::getLocalAABB() const -> AABB3F
    {
        return {};
    }

    auto PhysxVehicle3::getWorldAABB() const -> AABB3F
    {
        return {};
    }

    void PhysxVehicle3::setEnabled( bool enabled )
    {
    }

    auto PhysxVehicle3::isEnabled() const -> bool
    {
        return false;
    }

    auto PhysxVehicle3::getVehicle() const -> PxVehicleDrive4W *
    {
        return m_vehicle;
    }

    void PhysxVehicle3::setVehicle( PxVehicleDrive4W *vehicle )
    {
        m_vehicle = vehicle;
    }

    auto PhysxVehicle3::getVehicleInput() -> SmartPtr<IPhysicsVehicleInput3> &
    {
        return m_vehicleInput;
    }

    auto PhysxVehicle3::getVehicleInput() const -> const SmartPtr<IPhysicsVehicleInput3> &
    {
        return m_vehicleInput;
    }

    auto PhysxVehicle3::getWheelTransformations() const -> Array<Transform3F>
    {
        return m_wheelTransforms;
    }

} // namespace workphone::physics

// end namespace fb
