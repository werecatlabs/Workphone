#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/ResourceLoadingListener.hpp>
#include <Workphone/Workphone.hpp>
#include <WPGraphicsOgre/Core/WPOgreDataStream.hpp>
#include <WPGraphicsOgre/Addons/OgreMemoryStream.hpp>
#include <OgreDataStream.h>
#include <OgreResource.h>
#include <OgreResourceManager.h>

namespace workphone
{
    namespace render
    {

        bool ResourceLoadingListener::resourceCollision( Ogre::Resource *resource,
                                                         Ogre::ResourceManager *resourceManager )
        {
#ifdef _DEBUG
            auto resourceName = resource->getName();
            auto resourceGroup = resource->getGroup();
            auto message = "Warning: Resource name collision. Resource Name: " + resourceName +
                           " Resource Group Name: " + resourceGroup;

            WP_LOG( message.c_str() );
#endif

            return false;
        }

        void ResourceLoadingListener::resourceStreamOpened( const Ogre::String &name,
                                                            const Ogre::String &group,
                                                            Ogre::Resource *resource,
                                                            Ogre::DataStreamPtr &dataStream )
        {
#ifdef _DEBUG
            auto message =
                "Resource stream opened. Resource Name: " + name + " Resource Group Name: " + group;

            WP_LOG( message.c_str() );
#endif
        }

        Ogre::DataStreamPtr ResourceLoadingListener::resourceLoading( const Ogre::String &name,
                                                                      const Ogre::String &group,
                                                                      Ogre::Resource *resource )
        {
            auto isBinary = true;
            auto fileExt = Path::getFileExtension( name.c_str() );
            if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
                fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" || fileExt == ".cfg" ||
                fileExt == ".h" || fileExt == ".cpp" )
            {
                isBinary = false;
            }

            if( fileExt == ".ttf" )
            {
                isBinary = true;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManager();

            WP_ASSERT( !StringUtil::isNullOrEmpty( name.c_str() ) );

            auto stream = fileSystem->open( name.c_str(), true, isBinary, false, false );
            if( !stream )
            {
                stream = fileSystem->open( name.c_str(), true, isBinary, false, true );
            }

            if( stream )
            {
                if( resource )
                {
                    resource->changeGroupOwnership(
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                }

                return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
            }

            return {};
        }

        bool ResourceLoadingListener::grouplessResourceExists( const String &name )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
            return fileSystem->isExistingFile( name );
        }

        Ogre::DataStreamPtr ResourceLoadingListener::grouplessResourceLoading( const String &name )
        {
            auto isBinary = true;
            auto fileExt = Path::getFileExtension( name );
            if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
                fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" || fileExt == ".cfg" ||
                fileExt == ".h" || fileExt == ".cpp" )
            {
                isBinary = false;
            }

            if( fileExt == ".ttf" )
            {
                isBinary = true;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            auto fileName = Path::getFileName( name );

            auto stream = fileSystem->open( name, true, isBinary, false, false );
            if( !stream )
            {
                stream = fileSystem->open( name, true, isBinary, false, true );
            }

            if( stream )
            {
                return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
            }

            return {};
        }

        Ogre::DataStreamPtr ResourceLoadingListener::grouplessResourceOpened(
            const String &name, Ogre::Archive *archive, Ogre::DataStreamPtr &dataStream )
        {
            return dataStream;
        }
    }  // end namespace render
}  // namespace workphone
