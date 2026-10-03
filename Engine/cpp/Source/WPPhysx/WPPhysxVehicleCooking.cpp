#include "WPPhysx/WPPhysxPCH.hpp"
#include "WPPhysx/WPPhysxVehicleCooking.hpp"
// #include "PxTkStream.hpp"

auto createConvexMesh( const PxVec3 *verts, const PxU32 numVerts, PxPhysics &physics,
                       PxCooking &cooking ) -> PxConvexMesh *
{
    // Create descriptor for convex mesh
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = numVerts;
    convexDesc.points.stride = sizeof( PxVec3 );
    convexDesc.points.data = verts;
    convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eINFLATE_CONVEX;

    PxConvexMesh *convexMesh = nullptr;
    //	PxToolkit::MemoryOutputStream buf;
    //	if(cooking.cookConvexMesh(convexDesc, buf))
    //	{
    //		PxToolkit::MemoryInputData id(buf.getData(), buf.getSize());
    //		convexMesh = physics.createConvexMesh(id);
    //	}

    return convexMesh;
}

auto createCuboidConvexMesh( const PxVec3 &halfExtents, PxPhysics &physics, PxCooking &cooking )
    -> PxConvexMesh *
{
    PxVec3 verts[8] = { PxVec3( -halfExtents.x, -halfExtents.y, -halfExtents.z ),
                        PxVec3( -halfExtents.x, -halfExtents.y, +halfExtents.z ),
                        PxVec3( -halfExtents.x, +halfExtents.y, -halfExtents.z ),
                        PxVec3( -halfExtents.x, +halfExtents.y, +halfExtents.z ),
                        PxVec3( +halfExtents.x, -halfExtents.y, -halfExtents.z ),
                        PxVec3( +halfExtents.x, -halfExtents.y, +halfExtents.z ),
                        PxVec3( +halfExtents.x, +halfExtents.y, -halfExtents.z ),
                        PxVec3( +halfExtents.x, +halfExtents.y, +halfExtents.z ) };
    PxU32  numVerts = 8;

    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createWedgeConvexMesh( const PxVec3 &halfExtents, PxPhysics &physics, PxCooking &cooking )
    -> PxConvexMesh *
{
    PxVec3 verts[6] = { PxVec3( -halfExtents.x, -halfExtents.y, -halfExtents.z ),
                        PxVec3( -halfExtents.x, -halfExtents.y, +halfExtents.z ),
                        PxVec3( -halfExtents.x, +halfExtents.y, -halfExtents.z ),
                        PxVec3( +halfExtents.x, -halfExtents.y, -halfExtents.z ),
                        PxVec3( +halfExtents.x, -halfExtents.y, +halfExtents.z ),
                        PxVec3( +halfExtents.x, +halfExtents.y, -halfExtents.z ) };
    PxU32  numVerts = 6;

    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createCylinderConvexMesh( const PxF32 width, const PxF32 radius, const PxU32 numCirclePoints,
                               PxPhysics &physics, PxCooking &cooking ) -> PxConvexMesh *
{
#define MAX_NUM_VERTS_IN_CIRCLE 16
    PX_ASSERT( numCirclePoints < MAX_NUM_VERTS_IN_CIRCLE );
    PxVec3      verts[2 * MAX_NUM_VERTS_IN_CIRCLE];
    PxU32       numVerts = 2 * numCirclePoints;
    const PxF32 dtheta = 2 * PxPi / ( 1.0f * numCirclePoints );
    for( PxU32 i = 0; i < MAX_NUM_VERTS_IN_CIRCLE; i++ )
    {
        const PxF32 theta = dtheta * i;
        const PxF32 cosTheta = radius * PxCos( theta );
        const PxF32 sinTheta = radius * PxSin( theta );
        verts[2 * i + 0] = PxVec3( -0.5f * width, cosTheta, sinTheta );
        verts[2 * i + 1] = PxVec3( +0.5f * width, cosTheta, sinTheta );
    }

    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createSquashedCuboidMesh( const PxF32 baseLength, const PxF32 baseDepth, const PxF32 height1,
                               const PxF32 height2, PxPhysics &physics, PxCooking &cooking )
    -> PxConvexMesh *
{
    const PxF32 x = baseLength * 0.5f;
    const PxF32 z = baseDepth * 0.5f;
    PxVec3 verts[8] = { PxVec3( -x, -0.5f * height1, -z ),           PxVec3( -x, -0.5f * height1, +z ),
                        PxVec3( +x, -0.5f * height1, -z ),           PxVec3( +x, -0.5f * height1, +z ),
                        PxVec3( -x, -0.5f * height1 + height2, -z ), PxVec3( -x, +0.5f * height1, +z ),
                        PxVec3( +x, -0.5f * height1 + height2, -z ), PxVec3( +x, +0.5f * height1, +z ) };
    PxU32  numVerts = 8;

    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createPrismConvexMesh( const PxF32 baseLength, const PxF32 baseDepth, const PxF32 height,
                            PxPhysics &physics, PxCooking &cooking ) -> PxConvexMesh *
{
    const PxF32 x = baseLength * 0.5f;
    const PxF32 z = baseDepth * 0.5f;

    PxVec3 verts[6] = {
        PxVec3( -x, 0, -z ), PxVec3( -x, 0, +z ),     PxVec3( +x, 0, -z ),
        PxVec3( +x, 0, +z ), PxVec3( -x, height, 0 ), PxVec3( +x, height, 0 ),
    };
    PxU32 numVerts = 6;

    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createChassisConvexMesh( const PxVec3 *verts, const PxU32 numVerts, PxPhysics &physics,
                              PxCooking &cooking ) -> PxConvexMesh *
{
    return createConvexMesh( verts, numVerts, physics, cooking );
}

auto createWheelConvexMesh( const PxVec3 *verts, const PxU32 numVerts, PxPhysics &physics,
                            PxCooking &cooking ) -> PxConvexMesh *
{
    // Extract the wheel radius and width from the aabb of the wheel convex mesh.
    PxVec3 wheelMin( PX_MAX_F32, PX_MAX_F32, PX_MAX_F32 );
    PxVec3 wheelMax( -PX_MAX_F32, -PX_MAX_F32, -PX_MAX_F32 );
    for( PxU32 i = 0; i < numVerts; i++ )
    {
        wheelMin.x = PxMin( wheelMin.x, verts[i].x );
        wheelMin.y = PxMin( wheelMin.y, verts[i].y );
        wheelMin.z = PxMin( wheelMin.z, verts[i].z );
        wheelMax.x = PxMax( wheelMax.x, verts[i].x );
        wheelMax.y = PxMax( wheelMax.y, verts[i].y );
        wheelMax.z = PxMax( wheelMax.z, verts[i].z );
    }
    const PxF32 wheelWidth = wheelMax.x - wheelMin.x;
    const PxF32 wheelRadius = PxMax( wheelMax.y, wheelMax.z );

    return createCylinderConvexMesh( wheelWidth, wheelRadius, 8, physics, cooking );
}
