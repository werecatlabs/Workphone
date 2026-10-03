#ifndef MeshLodUsage_h__
#define MeshLodUsage_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /// Mesh LOD usage.
    class MeshLodUsage : public ISharedObject
    {
    public:
        /** Constructor. */
        MeshLodUsage();

        /** Destructor. */
        ~MeshLodUsage();

        /** User-supplied values used to determine on which distance the lod is applies.
        @remarks
        This is required in case the LOD strategy changes.
        */
        f32 userValue;

        /** Value used by to determine when this LOD applies.
        @remarks
            May be interpretted differently by different strategies.
            Transformed from user-supplied values with LodStrategy::transformUserValue.
        */
        f32 value;

        /// Only relevant if mIsLodManual is true, the name of the alternative mesh to use.
        String manualName;
        /// Hard link to mesh to avoid looking up each time.
        mutable SmartPtr<Mesh> manualMesh;
        /// Edge list for this LOD level (may be derived from manual mesh).
        mutable EdgeData *edgeData;
    };

}  // namespace workphone

#endif  // MeshLodUsage_h__
