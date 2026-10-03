#ifndef WPPhysxVehicleCooking_h__
#define WPPhysxVehicleCooking_h__

#include "PxPhysicsAPI.h"

using namespace physx;

PxConvexMesh *createConvexMesh( const PxVec3 *verts, PxU32 numVerts, PxPhysics &physics,
                                PxCooking &cooking );
PxConvexMesh *createCuboidConvexMesh( const PxVec3 &halfExtents, PxPhysics &physics,
                                      PxCooking &cooking );
PxConvexMesh *createWedgeConvexMesh( const PxVec3 &halfExtents, PxPhysics &physics, PxCooking &cooking );
PxConvexMesh *createCylinderConvexMesh( PxF32 width, PxF32 radius, PxU32 numCirclePoints,
                                        PxPhysics &physics, PxCooking &cooking );
PxConvexMesh *createSquashedCuboidMesh( PxF32 baseLength, PxF32 baseDepth, PxF32 height1, PxF32 height2,
                                        PxPhysics &physics, PxCooking &cooking );
PxConvexMesh *createPrismConvexMesh( PxF32 baseLength, PxF32 baseDepth, PxF32 height, PxPhysics &physics,
                                     PxCooking &cooking );
PxConvexMesh *createWheelConvexMesh( const PxVec3 *verts, PxU32 numVerts, PxPhysics &physics,
                                     PxCooking &cooking );
PxConvexMesh *createChassisConvexMesh( const PxVec3 *verts, PxU32 numVerts, PxPhysics &physics,
                                       PxCooking &cooking );

#endif // WPPhysxVehicleCooking_h__
