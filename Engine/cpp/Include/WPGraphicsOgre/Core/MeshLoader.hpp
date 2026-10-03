#ifndef MeshLoader_h__
#define MeshLoader_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <OgreMesh.h>

namespace workphone
{
    namespace render
    {

        class MeshLoader : public ISharedObject
        {
        public:
            MeshLoader();
            ~MeshLoader() override;

            static void loadFBMesh( Ogre::MeshPtr mesh, SmartPtr<IMesh> fbMesh );
        };
    }  // end namespace render
}  // namespace workphone

#endif  // MeshLoader_h__
