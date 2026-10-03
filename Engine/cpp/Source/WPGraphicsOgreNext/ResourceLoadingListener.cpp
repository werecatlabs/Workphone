#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/ResourceLoadingListener.hpp>
#include <WPGraphicsOgreNext/WPOgreDataStream.hpp>
#include <WPGraphicsOgreNext/OgreMemoryStream.hpp>
#include <WPGraphicsOgreNext/WPOgreDataStream.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreDataStream.h>
#include <OgreResource.h>
#include <OgreResourceManager.h>

namespace workphone::render
{

    ResourceLoadingListener::ResourceLoadingListener() = default;

    ResourceLoadingListener::~ResourceLoadingListener() = default;

    auto ResourceLoadingListener::resourceCollision( Ogre::Resource *resource,
                                                     Ogre::ResourceManager *resourceManager ) -> bool
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

    auto ResourceLoadingListener::resourceLoading( const Ogre::String &name, const Ogre::String &group,
                                                   Ogre::Resource *resource ) -> Ogre::DataStreamPtr
    {
        auto fileName = String( name.c_str(), name.length() );
        if( StringUtil::isNullOrEmpty( fileName ) )
        {
            return {};
        }

        auto isBinary = true;
        auto fileExt = Path::getFileExtension( fileName.c_str() );
        if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
            fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" )
        {
            isBinary = false;
        }

        if( fileExt == ".ttf" )
        {
            isBinary = true;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();

        WP_ASSERT( !StringUtil::isNullOrEmpty( fileName ) );

        auto stream = fileSystem->open( fileName, true, isBinary, false, false );
        if( !stream )
        {
            stream = fileSystem->open( fileName, true, isBinary, false, true );
        }

        if( !stream )
        {
            stream = fileSystem->open( fileName, true, isBinary, true, true );
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

        WP_LOG( String( "Resource not found: " ) + name.c_str() );
        return {};
    }

    auto ResourceLoadingListener::grouplessResourceExists( const Ogre::String &name ) -> bool
    {
        auto fileName = String( name.c_str(), name.length() );

        if( StringUtil::contains( fileName, "_rt" ) )
        {
            return true;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        WP_ASSERT( !StringUtil::isNullOrEmpty( fileName ) );
        auto existing = fileSystem->isExistingFile( fileName, false, false );
        if( !existing )
        {
            existing = fileSystem->isExistingFile( fileName, true, false );
            if( !existing )
            {
                existing = fileSystem->isExistingFile( fileName, true, true );
            }
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto textureManager = graphicsSystem->getTextureManager();
        auto materialManager = graphicsSystem->getMaterialManager();

        if( textureManager->getById( fileName ) != nullptr )
        {
            existing = true;
        }

        if( materialManager->getById( fileName ) != nullptr )
        {
            existing = true;
        }

        return existing;
    }

    auto ResourceLoadingListener::grouplessResourceLoading( const Ogre::String &name )
        -> Ogre::DataStreamPtr
    {
        auto isBinary = true;
        auto fileExt = Path::getFileExtension( name.c_str() );
        if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
            fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" )
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
        if( fileSystem )
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name.c_str() ) );

            auto stream = fileSystem->open( name.c_str(), true, isBinary, false, false, false );
            if( !stream )
            {
                stream = fileSystem->open( name.c_str(), true, isBinary, false, true, true );
            }

            if( stream )
            {
                return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
            }
        }

        return {};
    }

    auto ResourceLoadingListener::grouplessResourceOpened( const Ogre::String &name,
                                                           Ogre::Archive *archive,
                                                           Ogre::DataStreamPtr &dataStream )
        -> Ogre::DataStreamPtr
    {
        return dataStream;
    }

}  // namespace workphone::render
