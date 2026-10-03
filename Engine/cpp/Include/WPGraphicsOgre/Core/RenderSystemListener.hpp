#ifndef RenderSystemListener_h__
#define RenderSystemListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>

#include <OgreRenderSystem.h>

namespace workphone
{
    namespace render
    {

        class RenderSystemListener : public Ogre::RenderSystem::Listener
        {
        public:
            void eventOccurred( const Ogre::String &eventName,
                                const Ogre::NameValuePairList *parameters = nullptr ) override
            {
            }
        };

    }  // end namespace render
}  // namespace workphone

#endif  // RenderSystemListener_h__
