#ifndef MeshSerializerListener_h__
#define MeshSerializerListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
 @remarks
    This class allows users to hook into the mesh loading process and
    modify references within the mesh as they are loading. Material and
    skeletal references can be processed using this interface which allows
    finer control over resources.
*/
    class MeshSerializerListener
    {
    public:
        virtual ~MeshSerializerListener()
        {
        }

        /// Called to override the loading of the given named material
        virtual void processMaterialName( Mesh *mesh, String *name ) = 0;
        /// Called to override the reference to a skeleton
        virtual void processSkeletonName( Mesh *mesh, String *name ) = 0;
    };

}  // namespace workphone

#endif  // MeshSerializerListener_h__
