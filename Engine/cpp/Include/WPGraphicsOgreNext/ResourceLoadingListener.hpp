#ifndef ResourceLoadingListener_h__
#define ResourceLoadingListener_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <OgreResourceGroupManager.h>
#include <OgreDataStream.h>

namespace workphone
{
    namespace render
    {
        class ResourceLoadingListener : public Ogre::ResourceLoadingListener
        {
        public:
            ResourceLoadingListener();
            ~ResourceLoadingListener() override;

            /** @copy Ogre::ResourceLoadingListener::resourceCollision */
            bool resourceCollision( Ogre::Resource *resource,
                                    Ogre::ResourceManager *resourceManager ) override;

            /** @copy Ogre::ResourceLoadingListener::resourceStreamOpened */
            void resourceStreamOpened( const Ogre::String &name, const Ogre::String &group,
                                       Ogre::Resource *resource,
                                       Ogre::DataStreamPtr &dataStream ) override;

            /** @copy Ogre::ResourceLoadingListener::resourceLoading */
            Ogre::DataStreamPtr resourceLoading( const Ogre::String &name, const Ogre::String &group,
                                                 Ogre::Resource *resource ) override;

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceExists */
            bool grouplessResourceExists( const Ogre::String &name ) override;

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceLoading */
            Ogre::DataStreamPtr grouplessResourceLoading( const Ogre::String &name ) override;

            /** @copy Ogre::ResourceLoadingListener::grouplessResourceOpened */
            Ogre::DataStreamPtr grouplessResourceOpened( const Ogre::String &name,
                                                         Ogre::Archive *archive,
                                                         Ogre::DataStreamPtr &dataStream ) override;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // ResourceLoadingListener_h__
