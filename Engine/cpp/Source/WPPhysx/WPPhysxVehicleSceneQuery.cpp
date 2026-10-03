#include "WPPhysx/WPPhysxPCH.hpp"
#include "WPPhysx/WPPhysxVehicleSceneQuery.hpp"
#include "vehicle/PxVehicleSDK.h"
#include "PxFiltering.h"
// #include "PsFoundation.hpp"
// #include "PsUtilities.hpp"

#define CHECK_MSG( \
    exp,           \
    msg ) // (!!(exp) || (physx::shdfnd::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER,
          // __FILE__, __LINE__, msg), 0) )

void SampleVehicleSetupDrivableShapeQueryFilterData( PxFilterData *qryFilterData )
{
    CHECK_MSG( 0 == qryFilterData->word3,
               "word3 is reserved for filter data for vehicle raycast queries" );
    qryFilterData->word3 = static_cast<PxU32>( SAMPLEVEHICLE_DRIVABLE_SURFACE );
}

void SampleVehicleSetupNonDrivableShapeQueryFilterData( PxFilterData *qryFilterData )
{
    CHECK_MSG( 0 == qryFilterData->word3,
               "word3 is reserved for filter data for vehicle raycast queries" );
    qryFilterData->word3 = static_cast<PxU32>( SAMPLEVEHICLE_UNDRIVABLE_SURFACE );
}

void SampleVehicleSetupVehicleShapeQueryFilterData( PxFilterData *qryFilterData )
{
    CHECK_MSG( 0 == qryFilterData->word3,
               "word3 is reserved for filter data for vehicle raycast queries" );
    qryFilterData->word3 = static_cast<PxU32>( SAMPLEVEHICLE_UNDRIVABLE_SURFACE );
}

auto SampleVehicleSceneQueryData::allocate( const PxU32 maxNumWheels ) -> SampleVehicleSceneQueryData *
{
    const PxU32                  size = sizeof( SampleVehicleSceneQueryData ) +
                                        sizeof( PxRaycastQueryResult ) * maxNumWheels +
                                        sizeof( PxRaycastHit ) * maxNumWheels;
    SampleVehicleSceneQueryData *sqData = nullptr;
    // = (SampleVehicleSceneQueryData*)PX_ALLOC(size, PX_DEBUG_EXP("PxVehicleNWSceneQueryData"));
    sqData->init();
    auto ptr = reinterpret_cast<PxU8 *>( sqData );
    ptr += sizeof( SampleVehicleSceneQueryData );
    sqData->mSqResults = reinterpret_cast<PxRaycastQueryResult *>( ptr );
    ptr += sizeof( PxRaycastQueryResult ) * maxNumWheels;
    sqData->mSqHitBuffer = reinterpret_cast<PxRaycastHit *>( ptr );
    ptr += sizeof( PxRaycastHit ) * maxNumWheels;
    sqData->mNumQueries = maxNumWheels;
    return sqData;
}

void SampleVehicleSceneQueryData::free()
{
    // PX_FREE(this);
}

auto SampleVehicleSceneQueryData::setUpBatchedSceneQuery( PxScene *scene ) -> PxBatchQuery *
{
    // PxBatchQueryDesc sqDesc;
    // sqDesc.userRaycastResultBuffer = mSqResults;
    // sqDesc.userRaycastHitBuffer = mSqHitBuffer;
    // sqDesc.raycastHitBufferSize = mNumQueries;
    // sqDesc.preFilterShader = mPreFilterShader;
    // sqDesc.spuPreFilterShader = mSpuPreFilterShader;
    // sqDesc.spuPreFilterShaderSize = mSpuPreFilterShaderSize;
    // return scene->createBatchQuery(sqDesc);

    return nullptr;
}
