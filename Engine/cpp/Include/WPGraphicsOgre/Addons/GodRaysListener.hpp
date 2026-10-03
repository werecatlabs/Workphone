#ifndef GodRaysListener_h__
#define GodRaysListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgrePrerequisites.h>
#include <OgreResource.h>
#include <OgreCompositorInstance.h>

namespace workphone
{
    namespace render
    {

        // Updates the god ray compositor parameters
        class GodRaysListener : public Ogre::CompositorInstance::Listener
        {
        public:
            GodRaysListener();

            virtual void notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

            virtual void notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

            Ogre::Vector4 SunScreenSpacePosition;

        private:
            Ogre::GpuProgramParametersSharedPtr params1;
            Ogre::GpuProgramParametersSharedPtr params2;
            Ogre::GpuProgramParametersSharedPtr params3;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // GodRaysListener_h__
