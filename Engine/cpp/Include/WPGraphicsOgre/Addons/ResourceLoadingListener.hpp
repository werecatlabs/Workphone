#ifndef ResourceLoadingListener_h__
#define ResourceLoadingListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgreResourceGroupManager.h>
#include <OgreDataStream.h>

namespace workphone
{
    namespace render
    {

        class ResourceLoadingListener : public Ogre::ResourceLoadingListener
        {
        public:
            ResourceLoadingListener() = default;
            ~ResourceLoadingListener() = default;

            /** @copy Ogre::ResourceLoadingListener::resourceCollision */
            bool resourceCollision( Ogre::Resource *resource, Ogre::ResourceManager *resourceManager );

            /** @copy Ogre::ResourceLoadingListener::resourceStreamOpened */
            void resourceStreamOpened( const Ogre::String &name, const Ogre::String &group,
                                       Ogre::Resource *resource, Ogre::DataStreamPtr &dataStream );

            /** @copy Ogre::ResourceLoadingListener::resourceLoading */
            Ogre::DataStreamPtr resourceLoading( const Ogre::String &name, const Ogre::String &group,
                                                 Ogre::Resource *resource );

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceExists */
            bool grouplessResourceExists( const String &name );

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceLoading */
            Ogre::DataStreamPtr grouplessResourceLoading( const String &name );

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceOpened */
            Ogre::DataStreamPtr grouplessResourceOpened( const String &name, Ogre::Archive *archive,
                                                         Ogre::DataStreamPtr &dataStream );
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ResourceLoadingListener_h__
