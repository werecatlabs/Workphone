#include <EditorPCH.hpp>
#include "jobs/GenerateSkybox.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    GenerateSkybox::GenerateSkybox() = default;

    GenerateSkybox::~GenerateSkybox() = default;

    void GenerateSkybox::createMaterialFromFolder( SmartPtr<IFolderExplorer> folder )
    {
        try
        {
            auto folderName = folder->getFolderName();
            auto leafFolderName = Path::getFileName( folderName );

            auto files = folder->getFiles();

            auto createMaterial = false;

            for( const auto &file : files )
            {
                auto fileNameLower = StringUtil::make_lower( file );
                if( fileNameLower.find( "front" ) != std::string::npos )
                {
                    createMaterial = true;
                }

                if( fileNameLower.find( "right" ) != std::string::npos )
                {
                    createMaterial = true;
                }

                if( fileNameLower.find( "back" ) != std::string::npos )
                {
                    createMaterial = true;
                }

                if( fileNameLower.find( "left" ) != std::string::npos )
                {
                    createMaterial = true;
                }

                if( fileNameLower.find( "top" ) != std::string::npos )
                {
                    createMaterial = true;
                }

                if( fileNameLower.find( "down" ) != std::string::npos )
                {
                    createMaterial = true;
                }
            }

            if( createMaterial )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                auto materialManager = graphicsSystem->getMaterialManagerPtr();

                auto fileSystem = applicationManager->getFileSystemPtr();

                auto resourceDatabase = applicationManager->getResourceDatabasePtr();

                auto materialFileExt = ".mat";
                auto materialFileName = leafFolderName + materialFileExt;

                auto projectPath = applicationManager->getProjectPath();

                auto skyboxMaterialPath = "/Assets/Materials/Skybox";
                auto materialPath = projectPath + skyboxMaterialPath + materialFileName;
                auto material = resourceDatabase->loadResourceByType<render::IMaterial>( materialPath );
                if( !material )
                {
                    auto uuid = StringUtil::getUUID();
                    auto materialResult = materialManager->createOrRetrieve( uuid, materialPath, "" );
                    material =
                        workphone::dynamic_pointer_cast<render::IMaterial>( materialResult.first );
                }

                material->setMaterialType( MaterialType::Skybox );
                material->load( nullptr );

                for( const auto &file : files )
                {
                    auto fileNameLower = StringUtil::make_lower( file );
                    auto relativePath =
                        Path::getRelativePath( applicationManager->getProjectPath(), file );

                    if( StringUtil::contains( fileNameLower, "front" ) ||
                        StringUtil::contains( fileNameLower, "fr" ) )
                    {
                        material->setTexture( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Front ) );
                    }

                    if( fileNameLower.find( "back" ) != std::string::npos )
                    {
                        material->setTexture( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Back ) );
                    }

                    if( fileNameLower.find( "right" ) != std::string::npos )
                    {
                        material->setTexture( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Right ) );
                    }

                    if( fileNameLower.find( "left" ) != std::string::npos )
                    {
                        material->setTexture( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Left ) );
                    }

                    if( fileNameLower.find( "top" ) != std::string::npos )
                    {
                        material->setTexture( relativePath, static_cast<u32>( SkyboxTextureTypes::Up ) );
                    }

                    if( fileNameLower.find( "down" ) != std::string::npos )
                    {
                        material->setTexture( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Down ) );
                    }
                }

                materialManager->saveToFile( materialPath, material );
            }

            auto subFolders = folder->getSubFolders();
            for( auto subFolder : subFolders )
            {
                createMaterialFromFolder( subFolder );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GenerateSkybox::setupSkyboxFromFolder( SmartPtr<IFolderExplorer> folder )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto materialManager = graphicsSystem->getMaterialManager();

            auto fileSystem = applicationManager->getFileSystem();

            auto resourceDatabase = applicationManager->getResourceDatabase();
            auto selectionManager = applicationManager->getSelectionManager();

            auto folderName = folder->getFolderName();
            auto leafFolderName = Path::getFileName( folderName );

            auto files = folder->getFiles();

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected )
                {
                    if( selected->isDerived<scene::IGameActor>() )
                    {
                        auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                        if( actor )
                        {
                            if( auto skybox = actor->getComponent<scene::Skybox>() )
                            {
                                setupSkyboxFromFiles( skybox, files );
                            }
                        }
                    }
                    else if( selected->isDerived<scene::Skybox>() )
                    {
                        auto skybox = workphone::dynamic_pointer_cast<scene::Skybox>( selected );
                        if( skybox )
                        {
                            setupSkyboxFromFiles( skybox, files );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GenerateSkybox::setupSkyboxFromFiles( SmartPtr<scene::Skybox> skybox,
                                               const Array<String> &files )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        for( const auto &file : files )
        {
            auto fileNameLower = StringUtil::make_lower( file );
            auto relativePath = Path::getRelativePath( applicationManager->getProjectPath(), file );

            if( StringUtil::contains( fileNameLower, "front" ) ||
                StringUtil::contains( fileNameLower, "fr" ) )
            {
                skybox->setTextureByName( relativePath, static_cast<u32>( SkyboxTextureTypes::Front ) );
            }

            if( StringUtil::contains( fileNameLower, "back" ) ||
                StringUtil::contains( fileNameLower, "bk" ) )
            {
                skybox->setTextureByName( relativePath, static_cast<u32>( SkyboxTextureTypes::Back ) );
            }

            if( !skybox->getSwapLeftRight() )
            {
                if( StringUtil::contains( fileNameLower, "right" ) ||
                    StringUtil::contains( fileNameLower, "rt" ) )
                {
                    skybox->setTextureByName( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Right ) );
                }

                if( StringUtil::contains( fileNameLower, "left" ) ||
                    StringUtil::contains( fileNameLower, "lf" ) )
                {
                    skybox->setTextureByName( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Left ) );
                }
            }
            else
            {
                if( StringUtil::contains( fileNameLower, "right" ) ||
                    StringUtil::contains( fileNameLower, "rt" ) )
                {
                    skybox->setTextureByName( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Left ) );
                }

                if( StringUtil::contains( fileNameLower, "left" ) ||
                    StringUtil::contains( fileNameLower, "lf" ) )
                {
                    skybox->setTextureByName( relativePath,
                                              static_cast<u32>( SkyboxTextureTypes::Right ) );
                }
            }

            if( StringUtil::contains( fileNameLower, "top" ) ||
                StringUtil::contains( fileNameLower, "up" ) )
            {
                skybox->setTextureByName( relativePath, static_cast<u32>( SkyboxTextureTypes::Up ) );
            }

            if( StringUtil::contains( fileNameLower, "down" ) ||
                StringUtil::contains( fileNameLower, "dn" ) )
            {
                skybox->setTextureByName( relativePath, static_cast<u32>( SkyboxTextureTypes::Down ) );
            }
        }
    }

    void GenerateSkybox::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto projectPath = applicationManager->getProjectPath();
            if( !fileSystem->isExistingFolder( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto folderPath = getFolderPath();
            if( StringUtil::isNullOrEmpty( folderPath ) )
            {
                if( auto fileDialog = fileSystem->openFileDialog() )
                {
                    fileDialog->setDialogMode( INativeFileDialog::DialogMode::Select );
                    fileDialog->setFileExtension( ".fbscene" );
                    fileDialog->setFilePath( projectPath );

                    auto result = fileDialog->openDialog();
                    if( result == INativeFileDialog::Result::Dialog_Okay )
                    {
                        auto filePath = fileDialog->getFilePath();
                        folderPath = Path::getRelativePath( projectPath, filePath );
                        setFolderPath( folderPath );
                    }
                }
            }

            if( !StringUtil::isNullOrEmpty( folderPath ) )
            {
                auto folder = fileSystem->getFolderListing( folderPath );
                if( !folder )
                {
                    auto folderAbsolutePath = Path::getAbsolutePath( projectPath, folderPath );
                    folder = fileSystem->getFolderListing( folderAbsolutePath );
                }

                if( folder )
                {
                    setupSkyboxFromFolder( folder );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String GenerateSkybox::getFolderPath() const
    {
        return m_folderPath;
    }

    void GenerateSkybox::setFolderPath( const String &folderPath )
    {
        m_folderPath = folderPath;
    }
}  // namespace workphone::editor
