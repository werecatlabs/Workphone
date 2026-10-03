#ifndef WPPhysxPrerequisites_h__
#define WPPhysxPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <WPPhysx/WPPhysxConfig.hpp>

//
// forward declarations
//
namespace physx
{

    class PxFoundation;
    class PxAllocatorCallback;
    class PxCapsuleController;
    class PxControllerManager;
    class PxCooking;
    class PxJoint;
    class PxD6Joint;
    class PxErrorCallback;
    class PxMaterial;
    class PxPhysics;
    class PxRigidActor;
    class PxRigidDynamic;
    class PxRigidStatic;
    class PxScene;
    class PxVehicleDrive4W;
    class PxCapsuleController;
    class PxControllerManager;
    class PxD6Joint;
    class PxFixedJoint;
    class PxShape;
    class PxDefaultCpuDispatcher;
    class PxContactModifyPair;
    class PxOutputStream;
    struct PxTriggerPair;
    struct PxConstraintInfo;

    class PxVehicleDrive4WRawInputData;
} // end namespace physx

namespace workphone
{
    namespace physics
    {
        class PhysxCharacterController;
        class PhysxManager;
        class PhysxRigidDynamic;
        class PhysxRigidStatic;
        class PhysxVehicle3;
        class PhysxVehicleInput;
        class PhysxScene;
        class PhysxVehicleManager;
        class PhysxCooker;
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxPrerequisites_h__
