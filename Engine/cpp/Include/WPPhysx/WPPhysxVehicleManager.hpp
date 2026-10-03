#ifndef WPPhysxVehicleManager_h__
#define WPPhysxVehicleManager_h__

#include "vehicle/PxVehicleSDK.h"
#include "vehicle/PxVehicleDrive4W.h"
#include "foundation/PxFoundation.h"

#if PX_DEBUG_VEHICLE_ON
#    include "vehicle/PxVehicleUtilTelemetry.h"
#endif

using namespace physx;

namespace physx
{
    class PxScene;
    class PxBatchQuery;
    class PxCooking;
    class PxMaterial;
    class PxConvexMesh;
    struct PxVehicleDrivableSurfaceType;
} // namespace physx

class SampleVehicleSceneQueryData;

class PxVehicle4WAlloc;

namespace workphone
{
    namespace physics
    {

        // Collision types and flags describing collision interactions of each collision type.
        enum
        {
            COLLISION_FLAG_GROUND = 1 << 0,
            COLLISION_FLAG_WHEEL = 1 << 1,
            COLLISION_FLAG_CHASSIS = 1 << 2,
            COLLISION_FLAG_OBSTACLE = 1 << 3,
            COLLISION_FLAG_DRIVABLE_OBSTACLE = 1 << 4,

            COLLISION_FLAG_GROUND_AGAINST =
                COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
            COLLISION_FLAG_WHEEL_AGAINST =
                COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
            COLLISION_FLAG_CHASSIS_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |
                                             COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE |
                                             COLLISION_FLAG_DRIVABLE_OBSTACLE,
            COLLISION_FLAG_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |
                                              COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE |
                                              COLLISION_FLAG_DRIVABLE_OBSTACLE,
            COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS |
                                                       COLLISION_FLAG_OBSTACLE |
                                                       COLLISION_FLAG_DRIVABLE_OBSTACLE,
        };

        // Id of drivable surface (used by suspension raycast filtering).
        enum
        {
            DRIVABLE_SURFACE_ID = 0xffffffff
        };

        // Drivable surface types.
        enum
        {
            SURFACE_TYPE_MUD = 0,
            SURFACE_TYPE_TARMAC,
            SURFACE_TYPE_SNOW,
            SURFACE_TYPE_GRASS,
            MAX_NUM_SURFACE_TYPES
        };

        static char gSurfaceTypes[MAX_NUM_SURFACE_TYPES + 1][64] = {
            "mud",
            "tarmac",
            "ice",
            "grass",
        };

        // Tire types.
        enum
        {
            TIRE_TYPE_WETS = 0,
            TIRE_TYPE_SLICKS,
            TIRE_TYPE_ICE,
            TIRE_TYPE_MUD,
            MAX_NUM_TIRE_TYPES
        };

        // Tire model friction for each combination of drivable surface type and tire type.
        static PxF32 gTireFrictionMultipliers[MAX_NUM_SURFACE_TYPES][MAX_NUM_TIRE_TYPES] = {
            // WETS	SLICKS	ICE		MUD
            { 0.95f, 0.95f, 0.95f, 0.95f }, // MUD
            { 1.10f, 1.15f, 1.10f, 1.10f }, // TARMAC
            { 0.70f, 0.70f, 0.70f, 0.70f }, // ICE
            { 0.80f, 0.80f, 0.80f, 0.80f }  // GRASS
        };

        class PhysxVehicleManager
        {
        public:
            enum
            {
                MAX_NUM_4W_VEHICLES = 8
            };

            PhysxVehicleManager();
            ~PhysxVehicleManager();

            void init( PxPhysics &physics, const PxMaterial **drivableSurfaceMaterials,
                       const PxVehicleDrivableSurfaceType *drivableSurfaceTypes );

            void shutdown();

            // Create a vehicle ready to drive.
            PxVehicleDrive4W *create4WVehicle( PxScene &scene, PxPhysics &physics, PxCooking &cooking,
                                               const PxMaterial &material, PxF32 chassisMass,
                                               const PxVec3      *wheelCentreOffsets4,
                                               PxConvexMesh      *chassisConvexMesh,
                                               PxConvexMesh     **wheelConvexMeshes4,
                                               const PxTransform &startTransform, bool useAutoGearFlag );

            PX_FORCE_INLINE PxU32 getNbVehicles() const
            {
                return mNumVehicles;
            }
            PX_FORCE_INLINE PxVehicleWheels *getVehicle( const PxU32 i )
            {
                return mVehicles[i];
            }

            // Start the suspension raycasts (always call before calling update)
            void suspensionRaycasts( PxScene *scene );

            // Update vehicle dynamics and compute forces/torques to apply to sdk rigid bodies.
#if PX_DEBUG_VEHICLE_ON
            void updateAndRecordTelemetryData( PxF32 timestep, const PxVec3 &gravity,
                                               PxVehicleWheels        *focusVehicleNW,
                                               PxVehicleTelemetryData *telemetryDataNW );
#else
            void update( const PxF32 timestep, const PxVec3 &gravity );
#endif

            // Reset the car back to its rest state with a specified transform.
            void resetNWCar( const PxTransform &transform, PxU32 vehicleId );

        private:
            // Array of all cars.
            PxVehicleWheels *mVehicles[MAX_NUM_4W_VEHICLES];
            PxU32            mNumVehicles;

            // sdk raycasts (for the suspension lines).
            SampleVehicleSceneQueryData *mSqData;
            PxBatchQuery                *mSqWheelRaycastBatchQuery;

            // Friction from combinations of tire and surface types.
            PxVehicleDrivableSurfaceToTireFrictionPairs *mSurfaceTirePairs;

            // Initialise a car back to its start transform and state.
            void resetNWCar( const PxTransform &startTransform, PxVehicleWheels *car );
        };

    } // namespace physics
} // namespace workphone

#endif // WPPhysxVehicleManager_h__
