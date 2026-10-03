#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CResourceGroupManager.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/WPOgreDataStream.hpp>
#include <WPGraphicsOgreNext/WPOgreArchive.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreFontManager.h>
#include <OgreOverlayManager.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreOverlayManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreMaterialManager.h>
#include <OgreParticleSystemManager.h>
#include <OgreGpuProgramManager.h>
#include <OgreConfigFile.h>
#include <OgreRoot.h>
#include <OgreHlms.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsDiskCache.h>
#include <OgreTextureGpuManager.h>
#include <OgreArchiveManager.h>
#include <OgreArchive.h>
#include <OgreLogManager.h>
#include <Terra/Hlms/OgreHlmsTerra.h>
#include "Ogre/OgreHlmsColibri.h"
#include <fstream>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CResourceGroupManager, IResourceGroupManager );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CResourceGroupManager::ResourceLoadJob, Job );

    const u32 CResourceGroupManager::RGMID_UNLOADRESOURCEGROUP = 0;

    namespace
    {
        const Ogre::String TerraPbsShadowArchiveName = "PbsTerraShadows";

        bool isTerraPbsShadowArchive( Ogre::Archive *archive )
        {
            return archive && archive->getName().find( TerraPbsShadowArchiveName ) != Ogre::String::npos;
        }

        void queueWPOgreArchiveForDestroy( Ogre::Archive *archive,
                                           Set<Ogre::Archive *> &archivesToDestroy )
        {
            WP_ASSERT( archive );
            if( !archive )
            {
                return;
            }

            auto fbArchive = dynamic_cast<WPOgreArchive *>( archive );
            WP_ASSERT( fbArchive );
            if( fbArchive )
            {
                archivesToDestroy.insert( archive );
            }
        }

        void queueHlmsArchivesForDestroy( Ogre::Hlms *hlms, Set<Ogre::Archive *> &archivesToDestroy )
        {
            WP_ASSERT( hlms );
            if( !hlms )
            {
                return;
            }

            queueWPOgreArchiveForDestroy( hlms->getDataFolder(), archivesToDestroy );

            auto libraryArchives = hlms->getPiecesLibraryAsArchiveVec();
            for( auto archive : libraryArchives )
            {
                queueWPOgreArchiveForDestroy( archive, archivesToDestroy );
            }
        }

        void removeTerraPbsShadowArchives( Ogre::Hlms *hlmsPbs, Set<Ogre::Archive *> &archivesToDestroy )
        {
            WP_ASSERT( hlmsPbs );
            if( !hlmsPbs )
            {
                return;
            }

            auto archivePbs = hlmsPbs->getDataFolder();
            WP_ASSERT( archivePbs );
            if( !archivePbs )
            {
                return;
            }

            auto libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
            Ogre::ArchiveVec filteredLibraries;
            auto removedTerraShadowArchive = false;

            for( auto archive : libraryPbs )
            {
                WP_ASSERT( archive );
                if( isTerraPbsShadowArchive( archive ) )
                {
                    queueWPOgreArchiveForDestroy( archive, archivesToDestroy );
                    removedTerraShadowArchive = true;
                }
                else
                {
                    filteredLibraries.push_back( archive );
                }
            }

            if( removedTerraShadowArchive )
            {
                hlmsPbs->reloadFrom( archivePbs, &filteredLibraries );

                auto updatedLibraries = hlmsPbs->getPiecesLibraryAsArchiveVec();
                for( auto archive : updatedLibraries )
                {
                    WP_ASSERT( !isTerraPbsShadowArchive( archive ) );
                }
            }
        }

        void destroyQueuedWPOgreArchives( const Set<Ogre::Archive *> &archivesToDestroy )
        {
            for( auto archive : archivesToDestroy )
            {
                auto fbArchive = dynamic_cast<WPOgreArchive *>( archive );
                WP_ASSERT( fbArchive );
                if( fbArchive )
                {
                    fbArchive->unload();
                    OGRE_DELETE fbArchive;
                }
            }
        }
    }  // namespace

    CResourceGroupManager::CResourceGroupManager()
    {
        setName( "CResourceGroupManager" );
    }

    CResourceGroupManager::~CResourceGroupManager()
    {
        unload( nullptr );
    }

    void CResourceGroupManager::loadResourceFile()
    {
        try
        {
            using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // Note:  macBundlePath works for iOS too. It's misnamed.
            const String resourcePath = Path::macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            const String resourcePath = Ogre::macBundlePath() + "/";
#else
            String resourcePath = "";
#endif

            const String resourceFileName = "resources2.cfg";

            auto resourceGroupManager = ResourceGroupManager::getSingletonPtr();

            ConfigFile cf;

            auto configFilePath = resourcePath + resourceFileName;

            try
            {
                auto ogreConfigFileName =
                    Ogre::String( configFilePath.c_str(), configFilePath.length() );
                cf.load( ogreConfigFileName );
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( e.getFullDescription().c_str() );
            }

            // Go through all sections & settings in the file
            ConfigFile::SectionIterator seci = cf.getSectionIterator();

            String secName, typeName, archName;
            while( seci.hasMoreElements() )
            {
                secName = seci.peekNextKey();
                ConfigFile::SettingsMultiMap *settings = seci.getNext();
                ConfigFile::SettingsMultiMap::iterator i;
                for( i = settings->begin(); i != settings->end(); ++i )
                {
                    try
                    {
                        typeName = i->first;
                        archName = i->second;

                        resourceGroupManager->addResourceLocation( archName.c_str(), typeName.c_str(),
                                                                   secName.c_str(), false );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_ERROR( e.getFullDescription().c_str() );
        }
    }

    void CResourceGroupManager::baseRegisterHlms()
    {
        try
        {
            using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // Note:  macBundlePath works for iOS too. It's misnamed.
            const String resourcePath = Path::macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            const String resourcePath = Ogre::macBundlePath() + "/";
#else
            String resourcePath = "";
#endif

            const String resourceFileName = "resources2.cfg";

            auto resourceGroupManager = ResourceGroupManager::getSingletonPtr();

            ConfigFile cf;

            auto configFilePath = resourcePath + resourceFileName;

            try
            {
                auto ogreConfigFileName =
                    Ogre::String( configFilePath.c_str(), configFilePath.length() );
                cf.load( ogreConfigFileName );
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( e.getFullDescription().c_str() );
            }

            auto applicationManager = core::IApplicationManager::instance();

            auto workingDirectory = Path::getWorkingDirectory();
            auto mediaPath = applicationManager->getRenderMediaPath();

#if defined WP_PLATFORM_WIN32
            auto rootHlmsFolder = Path::lexically_normal( workingDirectory, mediaPath );
#elif defined WP_PLATFORM_APPLE
            auto rootHlmsFolder = mediaPath;
#else
            auto rootHlmsFolder = mediaPath;
#endif
            //auto rootHlmsFolder = mediaPath;

            // At this point rootHlmsFolder should be a valid path to the Hlms data folder
            auto root = Root::getSingletonPtr();
            auto hlmsManager = root->getHlmsManager();

            Ogre::HlmsColibri *hlmsColibri = nullptr;
            Ogre::HlmsUnlit *hlmsUnlit = nullptr;
            Ogre::HlmsPbs *hlmsPbs = nullptr;

            // For retrieval of the paths to the different folders needed
            Ogre::String mainFolderPath;
            Ogre::StringVector libraryFoldersPaths;
            Ogre::StringVector::const_iterator libraryFolderPathIt;
            Ogre::StringVector::const_iterator libraryFolderPathEn;

            auto &archiveManager = ArchiveManager::getSingleton();

#if WP_USE_COLLIBRI
            {
                //Create & Register HlmsColibri
                //Get the path to all the subdirectories used by HlmsColibri
                Ogre::HlmsColibri::getDefaultPaths( mainFolderPath, libraryFoldersPaths );

                auto archivePath =
                    Path::lexically_normal( rootHlmsFolder.c_str(), mainFolderPath.c_str() );
                //Ogre::Archive *archiveUnlit =
                //     archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );
                auto archiveUnlit = OGRE_NEW WPOgreArchive( archivePath.c_str(), "FileSystem" );

                Ogre::ArchiveVec archiveUnlitLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    auto archiveLibraryPath = Path::lexically_normal( rootHlmsFolder.c_str(),
                                                                      ( *libraryFolderPathIt ).c_str() );
                    //Ogre::Archive *archiveLibrary = archiveManager.load(
                    //    rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
                    auto archiveLibrary =
                        OGRE_NEW WPOgreArchive( archiveLibraryPath.c_str(), "FileSystem" );
                    archiveUnlitLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                //Create and register the unlit Hlms
                hlmsColibri = OGRE_NEW Ogre::HlmsColibri( archiveUnlit, &archiveUnlitLibraryFolders,
                                                          Ogre::HlmsTypes::HLMS_USER0, "ui" );
                hlmsManager->registerHlms( hlmsColibri, true );
            }
#endif

            {
                // Create & Register HlmsUnlit
                // Get the path to all the subdirectories used by HlmsUnlit
                HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );

                auto archivePath =
                    Path::lexically_normal( rootHlmsFolder.c_str(), mainFolderPath.c_str() );
                //Archive *archiveUnlit =
                //    archiveManager.load( archivePath, "FileSystem", true );
                auto archiveUnlit = OGRE_NEW WPOgreArchive( archivePath.c_str(), "FileSystem" );

                ArchiveVec archiveUnlitLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    auto archiveLibraryPath = Path::lexically_normal( rootHlmsFolder.c_str(),
                                                                      ( *libraryFolderPathIt ).c_str() );
                    //Archive *archiveLibrary = archiveManager.load(
                    //    rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
                    auto archiveLibrary =
                        OGRE_NEW WPOgreArchive( archiveLibraryPath.c_str(), "FileSystem" );
                    archiveUnlitLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                // Create and register the unlit Hlms
                hlmsUnlit = OGRE_NEW HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
                hlmsManager->registerHlms( hlmsUnlit, true );
            }

            {
                //Create & Register HlmsPbs
                // Do the same for HlmsPbs:
                HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );

                auto archivePath =
                    Path::lexically_normal( rootHlmsFolder.c_str(), mainFolderPath.c_str() );
                //auto archivePbs = archiveManager.load( archivePath, "FileSystem", true );
                auto archivePbs = OGRE_NEW WPOgreArchive( archivePath.c_str(), "FileSystem" );

                // Get the library archive(s)
                ArchiveVec archivePbsLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    auto archiveLibraryPath = Path::lexically_normal( rootHlmsFolder.c_str(),
                                                                      ( *libraryFolderPathIt ).c_str() );
                    //Archive *archiveLibrary =
                    //    archiveManager.load( archiveLibraryPath, "FileSystem", true );

                    auto archiveLibrary =
                        OGRE_NEW WPOgreArchive( archiveLibraryPath.c_str(), "FileSystem" );
                    archivePbsLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                // Create and register
                hlmsPbs = OGRE_NEW HlmsPbs( archivePbs, &archivePbsLibraryFolders );
                hlmsManager->registerHlms( hlmsPbs, true );
            }

            RenderSystem *renderSystem = Root::getSingletonPtr()->getRenderSystem();
            if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            {
                // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
                // and below to avoid saturating AMD's discard limit (8MB) or
                // saturate the PCIE bus in some low end machines.
                bool supportsNoOverwriteOnTextureBuffers;
                renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                                  &supportsNoOverwriteOnTextureBuffers );

                if( !supportsNoOverwriteOnTextureBuffers )
                {
                    hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
                    hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto isAndroid() -> bool
    {
        return false;
    }

    auto getMediaReadArchiveType() -> const char *
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
        return "FileSystem";
#else
        return "APKFileSystem";
#endif
    }

    void CResourceGroupManager::registerHlms()
    {
        try
        {
            using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // Note:  macBundlePath works for iOS too. It's misnamed.
            const String resourcePath = Path::macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            const String resourcePath = Ogre::macBundlePath() + "/";
#else
            String resourcePath = "";
#endif

            auto mRoot = Ogre::Root::getSingletonPtr();

            const String resourceFileName = "resources2.cfg";

            auto resourceGroupManager = ResourceGroupManager::getSingletonPtr();

            ConfigFile cf;

            auto configFilePath = resourcePath + resourceFileName;

            try
            {
                auto ogreConfigFileName =
                    Ogre::String( configFilePath.c_str(), configFilePath.length() );
                cf.load( ogreConfigFileName );
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( e.getFullDescription().c_str() );
            }

            auto applicationManager = core::IApplicationManager::instance();

            auto workingDirectory = Path::getWorkingDirectory();
            auto mediaPath = applicationManager->getRenderMediaPath();

#if defined WP_PLATFORM_WIN32
            auto rootHlmsFolder = Path::lexically_normal( workingDirectory, mediaPath );
#elif defined WP_PLATFORM_APPLE
            auto rootHlmsFolder = mediaPath;
#else
            auto rootHlmsFolder = mediaPath;
#endif

            Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();

            Ogre::String shaderSyntax = "GLSL";
            if( renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem" )
            {
                shaderSyntax = "GLSLES";
            }
            if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            {
                shaderSyntax = "HLSL";
            }
            else if( renderSystem->getName() == "Metal Rendering Subsystem" )
            {
                shaderSyntax = "Metal";
            }

            Ogre::String mainFolderPath;
            Ogre::StringVector libraryFoldersPaths;
            Ogre::StringVector::const_iterator libraryFolderPathIt;
            Ogre::StringVector::const_iterator libraryFolderPathEn;

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

            Ogre::HlmsTerra *hlmsTerra = nullptr;
            Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();

            {
                // Create & Register HlmsTerra
                // Get the path to all the subdirectories used by HlmsTerra
                Ogre::HlmsTerra::getDefaultPaths( mainFolderPath, libraryFoldersPaths );

                auto archivePath =
                    Path::lexically_normal( rootHlmsFolder.c_str(), mainFolderPath.c_str() );
                //Ogre::Archive *archiveTerra = archiveManager.load( archivePath, "FileSystem", true );
                auto archiveTerra = OGRE_NEW WPOgreArchive( archivePath.c_str(), "FileSystem" );

                Ogre::ArchiveVec archiveTerraLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    auto archiveLibraryPath = Path::lexically_normal( rootHlmsFolder.c_str(),
                                                                      ( *libraryFolderPathIt ).c_str() );

                    //Ogre::Archive *archiveLibrary = archiveManager.load(
                    //    rootHlmsFolder + *libraryFolderPathIt, getMediaReadArchiveType(), true );

                    auto archiveLibrary =
                        OGRE_NEW WPOgreArchive( archiveLibraryPath.c_str(), "FileSystem" );
                    archiveTerraLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                // Create and register the terra Hlms
                hlmsTerra = OGRE_NEW Ogre::HlmsTerra( archiveTerra, &archiveTerraLibraryFolders );
                hlmsManager->registerHlms( hlmsTerra, true );
            }

            // Add Terra's piece files that customize the PBS implementation.
            // These pieces are coded so that they will be activated when
            // we set the HlmsPbsTerraShadows listener and there's an active Terra
            //(see Tutorial_TerrainGameState::createScene01)
            if( Ogre::Hlms *hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS ) )
            {
                Ogre::Archive *archivePbs = hlmsPbs->getDataFolder();
                Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
                //libraryPbs.push_back( Ogre::ArchiveManager::getSingletonPtr()->load(
                //    rootHlmsFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", "FileSystem",
                //    true ) );

                auto terraShadowFolder =
                    Ogre::String( "Hlms/Terra/" ) + shaderSyntax + "/PbsTerraShadows";
                auto archivePath =
                    Path::lexically_normal( rootHlmsFolder.c_str(), terraShadowFolder.c_str() );
                auto archiveLibrary = OGRE_NEW WPOgreArchive( archivePath.c_str(), "FileSystem" );
                libraryPbs.push_back( archiveLibrary );

                hlmsPbs->reloadFrom( archivePbs, &libraryPbs );
            }
            else
            {
                WP_LOG_ERROR( "HlmsPbs was not found - cannot add Terra's PbsTerraShadows pieces" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    //        void CResourceGroupManager::baseRegisterHlms()
    //        {
    //            try
    //            {
    //                using namespace Ogre;
    //
    //                auto applicationManager = core::IApplicationManager::instance();
    //
    //                auto workingDirectory = Path::getWorkingDirectory();
    //                auto mediaPath = applicationManager->getMediaPath();
    //                //auto rootHlmsFolder = Path::lexically_normal( workingDirectory, mediaPath );
    //                auto rootHlmsFolder = mediaPath;
    //
    //#if WP_FINAL
    //                rootHlmsFolder = String( "" );
    //#endif
    //
    //                auto root = Root::getSingletonPtr();
    //                auto hlmsManager = root->getHlmsManager();
    //
    //                // At this point rootHlmsFolder should be a valid path to the Hlms data folder
    //
    //                HlmsUnlit *hlmsUnlit = nullptr;
    //                HlmsPbs *hlmsPbs = nullptr;
    //
    //                // For retrieval of the paths to the different folders needed
    //                String mainFolderPath;
    //                StringVector libraryFoldersPaths;
    //                StringVector::const_iterator libraryFolderPathIt;
    //                StringVector::const_iterator libraryFolderPathEn;
    //
    //                ArchiveManager &archiveManager = ArchiveManager::getSingleton();
    //
    //                {
    //                    // Create & Register HlmsUnlit
    //                    // Get the path to all the subdirectories used by HlmsUnlit
    //                    HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
    //
    //                    auto archivePath = Path::lexically_normal( rootHlmsFolder, mainFolderPath );
    //                    //auto archiveUnlit = archiveManager.load( archivePath, "FileSystem", true );
    //                    auto archiveUnlit = new WPOgreArchive( archivePath, "FileSystem" );
    //
    //                    ArchiveVec archiveUnlitLibraryFolders;
    //                    libraryFolderPathIt = libraryFoldersPaths.begin();
    //                    libraryFolderPathEn = libraryFoldersPaths.end();
    //
    //                    while( libraryFolderPathIt != libraryFolderPathEn )
    //                    {
    //                        auto archiveLibraryPath =
    //                            Path::lexically_normal( rootHlmsFolder, *libraryFolderPathIt );
    //                        //Archive *archiveLibrary =
    //                        //    archiveManager.load( archiveLibraryPath, "FileSystem", true );
    //
    //                        auto archiveLibrary = new WPOgreArchive( archiveLibraryPath, "FileSystem" );
    //                        archiveUnlitLibraryFolders.push_back( archiveLibrary );
    //                        ++libraryFolderPathIt;
    //                    }
    //
    //                    // Create and register the unlit Hlms
    //                    hlmsUnlit = new HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
    //                    hlmsManager->registerHlms( hlmsUnlit );
    //                }
    //
    //                {
    //                    // Create & Register HlmsPbs
    //                    // Do the same for HlmsPbs:
    //                    HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
    //
    //                    auto archivePath = Path::lexically_normal( rootHlmsFolder, mainFolderPath );
    //                    //auto archivePbs = archiveManager.load( archivePath, "FileSystem", true );
    //                    auto archivePbs = new WPOgreArchive( archivePath, "FileSystem" );
    //
    //                    // Get the library archive(s)
    //                    ArchiveVec archivePbsLibraryFolders;
    //                    libraryFolderPathIt = libraryFoldersPaths.begin();
    //                    libraryFolderPathEn = libraryFoldersPaths.end();
    //                    while( libraryFolderPathIt != libraryFolderPathEn )
    //                    {
    //                        auto archiveLibraryPath =
    //                            Path::lexically_normal( rootHlmsFolder, *libraryFolderPathIt );
    //                        //Archive *archiveLibrary =
    //                        //    archiveManager.load( archiveLibraryPath, "FileSystem", true );
    //
    //                        auto archiveLibrary = new WPOgreArchive( archiveLibraryPath, "FileSystem" );
    //                        archivePbsLibraryFolders.push_back( archiveLibrary );
    //                        ++libraryFolderPathIt;
    //                    }
    //
    //                    // Create and register
    //                    hlmsPbs = new HlmsPbs( archivePbs, &archivePbsLibraryFolders );
    //                    hlmsManager->registerHlms( hlmsPbs );
    //                }
    //
    //                auto renderSystem = root->getRenderSystem();
    //                if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
    //                {
    //                    // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
    //                    // and below to avoid saturating AMD's discard limit (8MB) or
    //                    // saturate the PCIE bus in some low end machines.
    //                    bool supportsNoOverwriteOnTextureBuffers;
    //                    renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
    //                                                      &supportsNoOverwriteOnTextureBuffers );
    //
    //                    if( !supportsNoOverwriteOnTextureBuffers )
    //                    {
    //                        hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
    //                        hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
    //                    }
    //                }
    //            }
    //            catch( std::exception &e )
    //            {
    //                WP_LOG_EXCEPTION( e );
    //            }
    //        }
    //
    //        void CResourceGroupManager::registerHlms( void )
    //        {
    //            try
    //            {
    //                auto applicationManager = core::IApplicationManager::instance();
    //
    //                auto rootHlmsFolder = applicationManager->getMediaPath();
    //                WP_ASSERT( !StringUtil::isNullOrEmpty( rootHlmsFolder ) );  // media path not set
    //
    //                if( StringUtil::isNullOrEmpty( rootHlmsFolder ) )
    //                {
    //                    WP_LOG( "Warning: media path empty." );
    //                }
    //
    //#if WP_FINAL
    //                rootHlmsFolder = String( "" );
    //#endif
    //
    //                auto root = Ogre::Root::getSingletonPtr();
    //                auto renderSystem = root->getRenderSystem();
    //
    //                auto shaderSyntax = Ogre::String( "GLSL" );
    //                auto renderSystemName = renderSystem->getName();
    //
    //                if( renderSystemName == "OpenGL ES 2.x Rendering Subsystem" )
    //                {
    //                    shaderSyntax = "GLSLES";
    //                }
    //                if( renderSystemName == "Direct3D11 Rendering Subsystem" )
    //                {
    //                    shaderSyntax = "HLSL";
    //                }
    //                else if( renderSystemName == "Metal Rendering Subsystem" )
    //                {
    //                    shaderSyntax = "Metal";
    //                }
    //
    //                Ogre::String mainFolderPath;
    //                Ogre::StringVector libraryFoldersPaths;
    //                Ogre::StringVector::const_iterator libraryFolderPathIt;
    //                Ogre::StringVector::const_iterator libraryFolderPathEn;
    //
    //                Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
    //
    //                Ogre::HlmsTerra *hlmsTerra = nullptr;
    //                Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
    //
    //                {
    //                    // Create & Register HlmsTerra
    //                    // Get the path to all the subdirectories used by HlmsTerra
    //                    Ogre::HlmsTerra::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
    //
    //                    auto archivePath = Path::lexically_normal( rootHlmsFolder, mainFolderPath );
    //                    //Ogre::Archive *archiveTerra = archiveManager.load( archivePath, "FileSystem", true );
    //                    auto archiveTerra = new WPOgreArchive( archivePath, "FileSystem" );
    //
    //                    Ogre::ArchiveVec archiveTerraLibraryFolders;
    //                    libraryFolderPathIt = libraryFoldersPaths.begin();
    //                    libraryFolderPathEn = libraryFoldersPaths.end();
    //                    while( libraryFolderPathIt != libraryFolderPathEn )
    //                    {
    //                        auto archiveLibraryPath =
    //                            Path::lexically_normal( rootHlmsFolder, mainFolderPath );
    //                        //Ogre::Archive *archiveLibrary =
    //                        //   archiveManager.load( archiveLibraryPath, "FileSystem", true );
    //
    //                        auto archiveLibrary = new WPOgreArchive( archiveLibraryPath, "FileSystem" );
    //                        archiveTerraLibraryFolders.push_back( archiveLibrary );
    //                        ++libraryFolderPathIt;
    //                    }
    //
    //                    // Create and register the terra Hlms
    //                    hlmsTerra = OGRE_NEW Ogre::HlmsTerra( archiveTerra, &archiveTerraLibraryFolders );
    //                    hlmsManager->registerHlms( hlmsTerra );
    //                }
    //
    //                // Add Terra's piece files that customize the PBS implementation.
    //                // These pieces are coded so that they will be activated when
    //                // we set the HlmsPbsTerraShadows listener and there's an active Terra
    //                //(see Tutorial_TerrainGameState::createScene01)
    //                Ogre::Hlms *hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS );
    //                Ogre::Archive *archivePbs = hlmsPbs->getDataFolder();
    //                Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
    //                //libraryPbs.push_back( Ogre::ArchiveManager::getSingletonPtr()->load(
    //                //    rootHlmsFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", "FileSystem",
    //                //    true ) );
    //
    //                auto archiveLibrary = new WPOgreArchive(
    //                    rootHlmsFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", "FileSystem" );
    //                libraryPbs.push_back( archiveLibrary );
    //
    //                hlmsPbs->reloadFrom( archivePbs, &libraryPbs );
    //            }
    //            catch( std::exception &e )
    //            {
    //                WP_LOG_EXCEPTION( e );
    //            }
    //        }

    void CResourceGroupManager::setupResources()
    {
        try
        {
            auto root = Ogre::Root::getSingletonPtr();
            auto hlmsManager = root->getHlmsManager();
            WP_ASSERT( hlmsManager );

            // Initialize resources for LTC area lights and accurate specular reflections (IBL)
            auto hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
            // WP_ASSERT(hlms);

            if( hlms )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbs *>( hlms ) );

                auto hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlms );

                try
                {
                    WP_ASSERT( hlmsPbs );
                    hlmsPbs->loadLtcMatrix();
                }
                catch( Ogre::FileNotFoundException &e )
                {
                    WP_LOG_ERROR( e.getFullDescription().c_str() );
                }
            }
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_ERROR( e.getFullDescription().c_str() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::loadTextureCache()
    {
#if !OGRE_NO_JSON
        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
        Ogre::Archive *rwAccessFolderArchive =
            archiveManager.load( m_writeAccessFolder.c_str(), "FileSystem", true );

        auto m_root = Ogre::Root::getSingletonPtr();

        try
        {
            const Ogre::String filename = "textureMetadataCache.json";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr stream = rwAccessFolderArchive->open( filename );
                Array<char> fileData;
                fileData.resize( stream->size() + 1 );
                if( !fileData.empty() )
                {
                    stream->read( &fileData[0], stream->size() );
                    // Add null terminator just in case (to prevent bad input)
                    fileData.back() = '\0';
                    Ogre::TextureGpuManager *textureManager =
                        m_root->getRenderSystem()->getTextureGpuManager();
                    textureManager->importTextureMetadataCache( stream->getName(), &fileData[0], false );
                }
            }
            else
            {
                auto msg = String( "[INFO] Texture cache not found at " ) +
                           String( m_writeAccessFolder.c_str() ) + "/textureMetadataCache.json";
                Ogre::LogManager::getSingleton().logMessage( msg.c_str() );
            }
        }
        catch( Ogre::Exception &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription() );
        }

        archiveManager.unload( rwAccessFolderArchive );
#endif
    }

    void CResourceGroupManager::saveTextureCache()
    {
        auto root = Ogre::Root::getSingletonPtr();

        if( root->getRenderSystem() )
        {
            Ogre::TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();
            if( textureManager )
            {
                Ogre::String jsonString;
                textureManager->exportTextureMetadataCache( jsonString );
                const auto path =
                    String( m_writeAccessFolder.c_str() ) + "/textureMetadataCache.json";
                std::ofstream file( path.c_str(), std::ios::binary | std::ios::out );
                if( file.is_open() )
                {
                    file.write( jsonString.c_str(), static_cast<std::streamsize>( jsonString.size() ) );
                }
                file.close();
            }
        }
    }

    void CResourceGroupManager::loadHlmsDiskCache()
    {
        if( !m_useMicrocodeCache && !m_useHlmsDiskCache )
        {
            return;
        }

        auto root = Ogre::Root::getSingletonPtr();

        Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
        Ogre::HlmsDiskCache diskCache( hlmsManager );

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

        Ogre::Archive *rwAccessFolderArchive =
            archiveManager.load( m_writeAccessFolder.c_str(), "FileSystem", true );

        if( m_useMicrocodeCache )
        {
            // Make sure the microcode cache is enabled.
            Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache( true );
            const Ogre::String filename = "microcodeCodeCache.cache";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->open( filename );
                Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache( shaderCacheFile );
            }
        }

        if( m_useHlmsDiskCache )
        {
            for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
            {
                Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                if( hlms )
                {
                    Ogre::String filename =
                        "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin";

                    try
                    {
                        if( rwAccessFolderArchive->exists( filename ) )
                        {
                            Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->open( filename );
                            diskCache.loadFrom( diskCacheFile );
                            //diskCache.applyTo( hlms );
                        }
                    }
                    catch( Ogre::Exception & )
                    {
                        Ogre::LogManager::getSingleton().logMessage(
                            Ogre::String( "Error loading cache from " ) + m_writeAccessFolder.c_str() +
                            "/" + filename +
                            "! If you have issues, try deleting the file "
                            "and restarting the app" );
                    }
                }
            }
        }

        archiveManager.unload( m_writeAccessFolder.c_str() );
    }

    void CResourceGroupManager::saveHlmsDiskCache()
    {
        auto root = Ogre::Root::getSingletonPtr();

        if( root->getRenderSystem() && Ogre::GpuProgramManager::getSingletonPtr() &&
            ( m_useMicrocodeCache || m_useHlmsDiskCache ) )
        {
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
            Ogre::HlmsDiskCache diskCache( hlmsManager );

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

            Ogre::Archive *rwAccessFolderArchive =
                archiveManager.load( m_writeAccessFolder.c_str(), "FileSystem", false );

            if( m_useHlmsDiskCache )
            {
                for( size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i )
                {
                    Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                    if( hlms )
                    {
                        diskCache.copyFrom( hlms );

                        Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->create(
                            "hlmsDiskCache" + Ogre::StringConverter::toString( i ) + ".bin" );
                        diskCache.saveTo( diskCacheFile );
                    }
                }
            }

            if( Ogre::GpuProgramManager::getSingleton().isCacheDirty() && m_useMicrocodeCache )
            {
                const Ogre::String filename = "microcodeCodeCache.cache";
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create( filename );
                Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache( shaderCacheFile );
            }

            archiveManager.unload( m_writeAccessFolder.c_str() );
        }
    }

    void CResourceGroupManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto isRenderTask = Thread::getTaskFlag( Thread::Render_Flag );
            WP_ASSERT( isRenderTask );
            if( !isRenderTask )
            {
                return;
            }

            const auto loadingState = getLoadingState();
            WP_ASSERT( loadingState != LoadingState::Loading );
            WP_ASSERT( loadingState != LoadingState::Unloading );
            if( loadingState == LoadingState::Loaded )
            {
                return;
            }

            ScopedLock lock( this );

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );
            if( !fileSystem )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );
            if( !factoryManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto renderMediaPath = applicationManager->getRenderMediaPath();
            WP_ASSERT( !StringUtil::isNullOrEmpty( renderMediaPath ) );
            if( StringUtil::isNullOrEmpty( renderMediaPath ) )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            fileSystem->addFolder( renderMediaPath, true );

            auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( resourceGroupManager );
            if( !resourceGroupManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto currentFolder = Ogre::String( "./" );
            auto locationType = Ogre::String( "FileSystem" );
            auto defaultGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
            WP_ASSERT( resourceGroupManager->resourceGroupExists( defaultGroup ) );

            if( !resourceGroupManager->resourceLocationExists( currentFolder, defaultGroup ) )
            {
                resourceGroupManager->addResourceLocation( currentFolder, locationType );
            }

            loadResourceFile();

            baseRegisterHlms();
            registerHlms();

            loadTextureCache();
            loadHlmsDiskCache();
            setupResources();

            auto resourceLoadJob = factoryManager->make_ptr<ResourceLoadJob>();
            WP_ASSERT( resourceLoadJob );
            if( !resourceLoadJob )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            resourceLoadJob->setResourceGroupManager( this );
            WP_ASSERT( resourceLoadJob->getResourceGroupManager() == this );
            resourceLoadJob->execute();

            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            if( !root )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto rs = root->getRenderSystem();
            WP_ASSERT( rs );
            if( !rs )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            auto textureManager = rs->getTextureGpuManager();
            WP_ASSERT( textureManager );
            if( !textureManager )
            {
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            constexpr auto textureBudget = 512u * 1024u * 1024u;
            textureManager->setStagingTextureMaxBudgetBytes( textureBudget );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            auto message = String( e.what() );
            WP_LOG_ERROR( message );
            
            setLoadingState( LoadingState::Error );
        }
    }

    void CResourceGroupManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded )
            {
                m_fileSystemArchive.reset();
                return;
            }

            if( loadingState == LoadingState::Unloading )
            {
                return;
            }

            ScopedLock lock( this );

            setLoadingState( LoadingState::Unloading );
            m_fileSystemArchive.reset();

            Set<Ogre::Archive *> hlmsArchivesToDestroy;
            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            if( root )
            {
                auto hlmsManager = root->getHlmsManager();
                WP_ASSERT( hlmsManager );
                if( hlmsManager )
                {
                    auto canDestroyTerraResources = true;

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto graphicsSystem =
                        (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
                    WP_ASSERT( graphicsSystem || loadingState != LoadingState::Loaded );
                    if( graphicsSystem )
                    {
                        WP_ASSERT( !graphicsSystem->getTerra() );
                        if( graphicsSystem->getTerra() )
                        {
                            WP_LOG_ERROR(
                                "CResourceGroupManager::unload: graphics system still "
                                "references Terra. Unload terrain objects before unloading "
                                "resource groups." );
                            canDestroyTerraResources = false;
                        }
                    }

                    auto hlms = hlmsManager->getHlms( Ogre::HLMS_USER3 );
                    auto hlmsTerra = dynamic_cast<Ogre::HlmsTerra *>( hlms );
                    if( hlms )
                    {
                        WP_ASSERT( hlmsTerra );
                    }

                    if( hlmsTerra )
                    {
                        const auto &linkedTerras = hlmsTerra->getLinkedTerras();
                        WP_ASSERT( linkedTerras.empty() );
                        if( !linkedTerras.empty() )
                        {
                            WP_LOG_ERROR(
                                "CResourceGroupManager::unload: Terra HLMS still has "
                                "linked Terra instances. Unload terrain objects before "
                                "unloading resource groups." );
                            canDestroyTerraResources = false;
                        }
                    }

                    if( canDestroyTerraResources )
                    {
                        if( hlmsTerra )
                        {
                            queueHlmsArchivesForDestroy( hlmsTerra, hlmsArchivesToDestroy );
                            hlmsManager->unregisterHlms( Ogre::HLMS_USER3 );
                            WP_ASSERT( !hlmsManager->getHlms( Ogre::HLMS_USER3 ) );
                        }
                    }

                    if( auto hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS ) )
                    {
                        queueHlmsArchivesForDestroy( hlmsPbs, hlmsArchivesToDestroy );
                        hlmsManager->unregisterHlms( Ogre::HLMS_PBS );
                        WP_ASSERT( !hlmsManager->getHlms( Ogre::HLMS_PBS ) );
                    }

                    if( auto hlmsUnlit = hlmsManager->getHlms( Ogre::HLMS_UNLIT ) )
                    {
                        queueHlmsArchivesForDestroy( hlmsUnlit, hlmsArchivesToDestroy );
                        hlmsManager->unregisterHlms( Ogre::HLMS_UNLIT );
                        WP_ASSERT( !hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );
                    }

#if WP_USE_COLLIBRI
                    if( auto hlmsColibri = hlmsManager->getHlms( Ogre::HLMS_USER0 ) )
                    {
                        queueHlmsArchivesForDestroy( hlmsColibri, hlmsArchivesToDestroy );
                        hlmsManager->unregisterHlms( Ogre::HLMS_USER0 );
                        WP_ASSERT( !hlmsManager->getHlms( Ogre::HLMS_USER0 ) );
                    }
#endif
                }
            }

            destroyQueuedWPOgreArchives( hlmsArchivesToDestroy );

            auto ogreResourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( ogreResourceGroupManager );
            if( ogreResourceGroupManager )
            {
                const auto isBuiltInGroup = []( const Ogre::String &groupName ) {
                    return groupName == Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ||
                           groupName == Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME ||
                           groupName == Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
                };

                Set<String> resourceLocations;
                auto resourceGroups = ogreResourceGroupManager->getResourceGroups();
                WP_ASSERT( !resourceGroups.empty() );

                for( const auto &groupName : resourceGroups )
                {
                    if( ogreResourceGroupManager->resourceGroupExists( groupName ) )
                    {
                        try
                        {
                            if( auto locations =
                                    ogreResourceGroupManager->listResourceLocations( groupName ) )
                            {
                                for( const auto &location : *locations )
                                {
                                    if( !StringUtil::isNullOrEmpty( location ) )
                                    {
                                        resourceLocations.insert( location );
                                    }
                                }
                            }
                        }
                        catch( Ogre::Exception &e )
                        {
                            WP_LOG_ERROR( e.getFullDescription().c_str() );
                        }
                    }
                }

                for( const auto &groupName : resourceGroups )
                {
                    if( !ogreResourceGroupManager->resourceGroupExists( groupName ) )
                    {
                        continue;
                    }

                    try
                    {
                        if( isBuiltInGroup( groupName ) )
                        {
                            ogreResourceGroupManager->clearResourceGroup( groupName );

                            if( auto locations =
                                    ogreResourceGroupManager->listResourceLocations( groupName ) )
                            {
                                auto locationSnapshot = *locations;
                                for( const auto &location : locationSnapshot )
                                {
                                    if( ogreResourceGroupManager->resourceLocationExists( location,
                                                                                          groupName ) )
                                    {
                                        ogreResourceGroupManager->removeResourceLocation( location,
                                                                                          groupName );
                                    }
                                }
                            }

                            WP_ASSERT( ogreResourceGroupManager->resourceGroupExists( groupName ) );
                        }
                        else
                        {
                            ogreResourceGroupManager->destroyResourceGroup( groupName );
                            WP_ASSERT( !ogreResourceGroupManager->resourceGroupExists( groupName ) );
                        }
                    }
                    catch( Ogre::Exception &e )
                    {
                        WP_LOG_ERROR( e.getFullDescription().c_str() );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }

                ogreResourceGroupManager->shutdownAll();

                if( auto archiveManager = Ogre::ArchiveManager::getSingletonPtr() )
                {
                    for( const auto &location : resourceLocations )
                    {
                        try
                        {
                            archiveManager->unload( location.c_str() );
                        }
                        catch( Ogre::Exception &e )
                        {
                            WP_LOG_ERROR( e.getFullDescription().c_str() );
                        }
                    }
                }
                else
                {
                    WP_ASSERT( false );
                }
            }

            setLoadingState( LoadingState::Unloaded );
            WP_ASSERT( getLoadingState() == LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void CResourceGroupManager::initialiseAllResourceGroups()
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->initialiseAllResourceGroups( true );
    }

    void CResourceGroupManager::initialiseResourceGroup( const String &groupName )
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->initialiseResourceGroup( groupName.c_str(), true );
    }

    void CResourceGroupManager::unloadResourceGroup( const String &groupName )
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->unloadResourceGroup( groupName.c_str() );
    }

    void CResourceGroupManager::clearResourceGroup( const String &groupName )
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->clearResourceGroup( groupName.c_str() );
    }

    void CResourceGroupManager::destroyResourceGroup( const String &groupName )
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->destroyResourceGroup( groupName.c_str() );
    }

    void CResourceGroupManager::_getObject( void **ppObject ) const
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        *ppObject = resourceGroupManager;
    }

    void CResourceGroupManager::reloadResources( const String &groupName )
    {
    }

    void CResourceGroupManager::parseScripts( const Array<String> &scripts )
    {
        parseScripts( Util::createSet( scripts ) );
    }

    void CResourceGroupManager::parseScripts( const Set<String> &scripts )
    {
        Ogre::String defaultGrp = "General";

        for( const auto &fileName : scripts )
        {
            try
            {
                String extension = Path::getFileExtension( fileName );

                if( extension == ( ".program" ) )
                {
                    parseScript( Ogre::MaterialManager::getSingletonPtr(), fileName, defaultGrp );
                }
                else if( extension == ( ".material" ) )
                {
                    parseScript( Ogre::MaterialManager::getSingletonPtr(), fileName, defaultGrp );
                }
                else if( extension == ( ".compositor" ) )
                {
                    // parseScript(Ogre::CompositorManager::getSingletonPtr(), fileName, defaultGrp);
                }
                else if( extension == ( ".fontdef" ) )
                {
                    // parseScript(Ogre::FontManager::getSingletonPtr(), fileName, defaultGrp);
                }
                else if( extension == ( ".overlay" ) )
                {
                    // parseScript(Ogre::OverlayManager::getSingletonPtr(), fileName, defaultGrp);
                }
                else if( extension == ( ".pu" ) )
                {
                    // parseScripts(Ogre::ParticleSystemManager::getSingletonPtr(), ".pu",
                    // defaultGrp);
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    void CResourceGroupManager::parseScript( Ogre::ScriptLoader *loader, const String &fileName,
                                             const String &group )
    {
        try
        {
            WP_ASSERT( loader );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto data = fileSystem->open( fileName );
            if( data )
            {
                Ogre::DataStreamPtr dataStream( new WPOgreDataStream( data ) );
                loader->parseScript( dataStream, group.c_str() );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::parseScripts( const String &extension, const String &group )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( extension ) );
            WP_ASSERT( !StringUtil::isNullOrEmpty( group ) );

            auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( resourceGroupManager );

            auto wildCard = "*" + extension;
            auto scriptLoader = resourceGroupManager->_findScriptLoader( wildCard.c_str() );
            WP_ASSERT( scriptLoader );

            auto groupMgr = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( groupMgr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto scriptFiles = fileSystem->getFilesWithExtension( extension );
            if( !scriptFiles.empty() )
            {
                for( auto &file : scriptFiles )
                {
                    try
                    {
                        auto fileName = String( file.filePath.c_str() );

#if 0
                            WP_LOG( "Parse script: " + fileName );

                            if( fileName.find( "PbsMaterials.material" ) != String::npos )
                            {
                                s32 stop = 0;
                                stop++;
                            }

                            auto text = fileSystem->readAllText( fileName );
                            if( text.find( "Rocks" ) != String::npos )
                            {
                                s32 stop = 0;
                                stop++;
                            }
#endif

                        auto data = fileSystem->open( fileName );
                        if( data )
                        {
                            Ogre::DataStreamPtr dataStream( new WPOgreDataStream( data ) );
                            scriptLoader->parseScript(
                                dataStream, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                        }
                    }
                    catch( Ogre::Exception &e )
                    {
                        auto message = e.getFullDescription();
                        WP_LOG_ERROR( message.c_str() );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::parseScripts( Ogre::ResourceManager *resMgr, const String &extension,
                                              const String &group )
    {
        try
        {
            WP_ASSERT( resMgr );

            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            Array<String> scriptFileNames;
            scriptFileNames.reserve( 32 );

            fileSystem->getFileNamesWithExtension( extension, scriptFileNames );
            if( !scriptFileNames.empty() )
            {
                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( const auto &fileName : uniqueScriptFileNames )
                {
                    try
                    {
                        auto data = fileSystem->open( fileName );
                        if( data )
                        {
                            Ogre::DataStreamPtr dataStream( new WPOgreDataStream( data ) );
                            resMgr->parseScript( dataStream, group.c_str() );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::parseScripts( Ogre::ParticleSystemManager *resMgr,
                                              const String &extension, const String &group )
    {
        try
        {
            WP_ASSERT( resMgr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            Array<String> scriptFileNames;
            scriptFileNames.reserve( 32 );

            fileSystem->getFileNamesWithExtension( extension, scriptFileNames );
            if( !scriptFileNames.empty() )
            {
                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( const auto &fileName : uniqueScriptFileNames )
                {
                    try
                    {
                        auto data = fileSystem->open( fileName );
                        if( data )
                        {
                            Ogre::DataStreamPtr dataStream( new WPOgreDataStream( data ) );
                            resMgr->parseScript( dataStream, group.c_str() );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::parseScripts( Ogre::ScriptLoader *loader, const String &extension,
                                              const String &group )
    {
        try
        {
            WP_ASSERT( loader );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            ScopedLock lock( graphicsSystem );

            Array<String> scriptFileNames;
            scriptFileNames.reserve( 32 );

            fileSystem->getFileNamesWithExtension( extension, scriptFileNames );
            if( !scriptFileNames.empty() )
            {
                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( const auto &fileName : uniqueScriptFileNames )
                {
                    try
                    {
                        auto data = fileSystem->open( fileName );
                        if( data )
                        {
                            Ogre::DataStreamPtr dataStream( new WPOgreDataStream( data ) );
                            loader->parseScript( dataStream, group.c_str() );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::parseHlmsScript( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        ScopedLock lock( graphicsSystem );

        auto root = Ogre::Root::getSingletonPtr();
        auto hlmsManager = root->getHlmsManager();

        static const auto defaultGrp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        try
        {
            if( auto stream = fileSystem->open( filePath ) )
            {
                auto ogreStream = Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
                hlmsManager->parseScript( ogreStream, defaultGrp );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::initialiseResourceGroups()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        WP_ASSERT( resourceGroupManager );

        ScopedLock lock( graphicsSystem );

        //resourceGroupManager->initialiseAllResourceGroups( false );

        auto groups = resourceGroupManager->getResourceGroups();
        for( auto &group : groups )
        {
            try
            {
                resourceGroupManager->initialiseResourceGroup( group, false );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    CResourceGroupManager::ResourceLoadJob::ResourceLoadJob() = default;

    CResourceGroupManager::ResourceLoadJob::~ResourceLoadJob() = default;

    void CResourceGroupManager::ResourceLoadJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();

            auto root = Ogre::Root::getSingletonPtr();
            auto hlmsManager = root->getHlmsManager();

            auto compositorManager = root->getCompositorManager2();
            WP_ASSERT( compositorManager );

            auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
            WP_ASSERT( resourceGroupManager );

            static const auto defaultGrp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            auto gpuProgramManager = Ogre::GpuProgramManager::getSingletonPtr();

            //// load scripts
            try
            {
#ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
#    if WP_BUILD_RENDERER_DX11
                static const auto hlslProgramExt = String( ".hlsl" );
                m_resourceGroupManager->parseScripts( gpuProgramManager, hlslProgramExt.c_str(),
                                                      defaultGrp.c_str() );
#    endif
#endif

#ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
#    if WP_BUILD_RENDERER_GL3PLUS
                static const auto glProgramExt = String( ".glsl" );
                m_resourceGroupManager->parseScripts( gpuProgramManager, glProgramExt, defaultGrp );
#    endif
#endif

#ifdef OGRE_BUILD_RENDERSYSTEM_METAL
                static const auto metalProgramExt = String( ".metal" );
                m_resourceGroupManager->parseScripts( gpuProgramManager, metalProgramExt, defaultGrp );
#endif
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            try
            {
                auto materialManager = Ogre::MaterialManager::getSingletonPtr();
                static const auto programExt = String( ".program" );
                m_resourceGroupManager->parseScripts( materialManager, programExt.c_str(),
                                                      defaultGrp.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            try
            {
                static const auto materialExt = String( ".material" );
                m_resourceGroupManager->parseScripts( materialExt.c_str(), defaultGrp.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            static const auto jsonMaterialExt = String( ".json" );
            // m_resourceGroupManager->parseScripts(jsonMaterialExt, defaultGrp);

            auto path = String( "2.0/scripts" );
            auto scriptFiles = fileSystem->getFilesWithExtension( path, jsonMaterialExt );
            if( !scriptFiles.empty() )
            {
                Set<String> uniqueScriptFileNames;
                for( auto &file : scriptFiles )
                {
                    if( !StringUtil::isNullOrEmpty( file.filePath ) )
                    {
                        uniqueScriptFileNames.insert( file.filePath );
                    }
                }

                for( const auto &filePath : uniqueScriptFileNames )
                {
                    m_resourceGroupManager->parseHlmsScript( filePath );
                }
            }

            try
            {
                auto fontManager = Ogre::FontManager::getSingletonPtr();
                static const auto fontExt = String( ".fontdef" );
                m_resourceGroupManager->parseScripts( fontManager, fontExt.c_str(), defaultGrp.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            // m_resourceGroupManager->parseScripts(Ogre::v1::OverlayManager::getSingletonPtr(),
            // ".overlay", defaultGrp); parseScripts(Ogre::ParticleSystemManager::getSingletonPtr(),
            // ".pu", defaultGrp);

            // m_resourceGroupManager->parseScripts(Ogre::GpuProgramManager::getSingletonPtr(),
            // ".cg", defaultGrp);

            try
            {
                static const auto compositorExt = String( ".compositor" );
                m_resourceGroupManager->parseScripts( compositorExt.c_str(), defaultGrp.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            try
            {
                m_resourceGroupManager->initialiseResourceGroups();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CResourceGroupManager::ResourceLoadJob::coroutine_execute_step(
        SmartPtr<ICoroutineData> &rYield )
    {
        auto applicationManager = core::IApplicationManager::instance();

        static const auto defaultGrp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        // load scripts
        m_resourceGroupManager->parseScripts( Ogre::MaterialManager::getSingletonPtr(), ".program",
                                              defaultGrp.c_str() );
        m_resourceGroupManager->parseScripts( Ogre::MaterialManager::getSingletonPtr(), ".material",
                                              defaultGrp.c_str() );
        // parseScripts(Ogre::CompositorManager::getSingletonPtr(), ".compositor", defaultGrp);

        auto fontManager = Ogre::FontManager::getSingletonPtr();
        static const auto fontExt = String( ".fontdef" );
        m_resourceGroupManager->parseScripts( fontManager, fontExt.c_str(), defaultGrp.c_str() );

        m_resourceGroupManager->parseScripts( Ogre::v1::OverlayManager::getSingletonPtr(), ".overlay",
                                              defaultGrp.c_str() );
        // parseScripts(Ogre::ParticleSystemManager::getSingletonPtr(), ".pu", defaultGrp);

        m_resourceGroupManager->parseScripts( Ogre::GpuProgramManager::getSingletonPtr(), ".cg",
                                              defaultGrp.c_str() );
        m_resourceGroupManager->parseScripts( Ogre::GpuProgramManager::getSingletonPtr(), ".hlsl",
                                              defaultGrp.c_str() );

        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
        resourceGroupManager->initialiseAllResourceGroups( false );
    }

    auto CResourceGroupManager::ResourceLoadJob::getResourceGroupManager() const
        -> CResourceGroupManager *
    {
        return m_resourceGroupManager;
    }

    void CResourceGroupManager::ResourceLoadJob::setResourceGroupManager( CResourceGroupManager *resourceGroupManager )
    {
        m_resourceGroupManager = resourceGroupManager;
    }
}  // namespace workphone::render
