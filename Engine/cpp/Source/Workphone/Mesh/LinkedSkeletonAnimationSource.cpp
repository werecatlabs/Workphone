#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/LinkedSkeletonAnimationSource.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>

namespace workphone
{
    void LinkedSkeletonAnimationSource::unload( SmartPtr<ISharedObject> data )
    {
        pSkeleton = nullptr;
        skeletonName.release();
    }

    LinkedSkeletonAnimationSource::LinkedSkeletonAnimationSource( const String &skelName, f32 scl,
                                                                  SmartPtr<ISkeleton> skelPtr ) :
        skeletonName( skelName ),
        pSkeleton( skelPtr ),
        scale( scl )
    {
    }

    LinkedSkeletonAnimationSource::LinkedSkeletonAnimationSource( const String &skelName, f32 scl ) :
        skeletonName( skelName ),
        scale( scl )
    {
    }

}  // namespace workphone
