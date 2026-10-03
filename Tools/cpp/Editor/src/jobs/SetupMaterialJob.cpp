#include <EditorPCH.hpp>
#include <jobs/SetupMaterialJob.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialTechnique.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, SetupMaterialJob, Job );

    SetupMaterialJob::SetupMaterialJob() = default;

    SetupMaterialJob::~SetupMaterialJob() = default;

    void SetupMaterialJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();

            auto meshLoader = applicationManager->getMeshLoader();

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto editorManager = EditorManager::getSingletonPtr();
            auto uiManager = editorManager->getUI();

            auto projectPath = applicationManager->getProjectPath();

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<render::IMaterial>() )
                {
                }
                else if( selected->isDerived<FileSelection>() )
                {
                    auto fileSelection = workphone::static_pointer_cast<FileSelection>( selected );
                    auto filePath = fileSelection->getFilePath();
                    if( filePath.empty() )
                    {
                        continue;
                    }

                    auto fileExt = Path::getFileExtension( filePath );
                    if( fileExt == ".mat" )
                    {
                        auto materialPath = Path::getParentPath( filePath );
                        auto searchPath = projectPath + "/" + Path::getParentPath( materialPath );

                        auto textureExtensions = ApplicationUtil::getSupportedTextureFormats();
                        for( const auto &textureExt : textureExtensions )
                        {
                            auto textureFiles =
                                fileSystem->getFileNamesWithExtension( searchPath, textureExt, true );
                            for( const auto &textureFile : textureFiles )
                            {
                                // fuzzy match the texture file name with the material name
                                auto materialName = Path::getFileNameWithoutExtension( filePath );
                                auto textureName = Path::getFileNameWithoutExtension( textureFile );

                                // Check if texture name contains material name or vice versa
                                bool isMatch = false;

                                // Convert to lowercase for case-insensitive comparison
                                auto materialNameLower = StringUtil::make_lower( materialName );
                                auto textureNameLower = StringUtil::make_lower( textureName );

                                // Check for common texture type suffixes
                                static const std::vector<std::string> textureTypes = {
                                    "_diffuse",  "_albedo",      "_basecolor", "_normal",
                                    "_nrm",      "_nrml",        "_roughness", "_rough",
                                    "_metallic", "_metal",       "_ao",        "_ambientocclusion",
                                    "_emissive", "_emission",    "_opacity",   "_alpha",
                                    "_height",   "_displacement"
                                };

                                // First check if the texture name contains the material name
                                if( textureNameLower.find( materialNameLower ) != std::string::npos )
                                {
                                    isMatch = true;
                                }
                                // Then check if the material name contains the texture name
                                else if( materialNameLower.find( textureNameLower ) !=
                                         std::string::npos )
                                {
                                    isMatch = true;
                                }
                                // Finally check if they share a common prefix before any texture type suffix
                                else
                                {
                                    for( const auto &type : textureTypes )
                                    {
                                        auto pos = textureNameLower.find( type );
                                        if( pos != std::string::npos )
                                        {
                                            auto textureBaseName = textureNameLower.substr( 0, pos );
                                            if( materialNameLower.find( textureBaseName ) !=
                                                std::string::npos )
                                            {
                                                isMatch = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if( isMatch )
                                {
                                    // Create a texture resource
                                    auto texturePath = StringUtil::cleanupPath( textureFile );
                                    auto relativeTexturePath =
                                        Path::lexically_relative( projectPath, texturePath );

                                    auto resourceDatabase = applicationManager->getResourceDatabase();
                                    auto texture =
                                        resourceDatabase->loadResourceByType<render::ITexture>(
                                            relativeTexturePath );

                                    if( texture )
                                    {
                                        // Determine texture type based on filename
                                        auto textureType = PbsTextureTypes::PBSM_DIFFUSE;

                                        if( textureNameLower.find( "_normal" ) != std::string::npos ||
                                            textureNameLower.find( "_nrm" ) != std::string::npos ||
                                            textureNameLower.find( "_nrml" ) != std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::PBSM_NORMAL;
                                        }
                                        else if( textureNameLower.find( "_roughness" ) !=
                                                     std::string::npos ||
                                                 textureNameLower.find( "_rough" ) != std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::PBSM_ROUGHNESS;
                                        }
                                        else if( textureNameLower.find( "_metallic" ) !=
                                                     std::string::npos ||
                                                 textureNameLower.find( "_metal" ) != std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::PBSM_METALLIC;
                                        }
                                        else if( textureNameLower.find( "_ao" ) != std::string::npos ||
                                                 textureNameLower.find( "_ambientocclusion" ) !=
                                                     std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::
                                                PBSM_DIFFUSE;  // AO maps are typically used in the diffuse channel
                                        }
                                        else if( textureNameLower.find( "_emissive" ) !=
                                                     std::string::npos ||
                                                 textureNameLower.find( "_emission" ) !=
                                                     std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::PBSM_EMISSIVE;
                                        }
                                        else if( textureNameLower.find( "_opacity" ) !=
                                                     std::string::npos ||
                                                 textureNameLower.find( "_alpha" ) != std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::
                                                PBSM_DIFFUSE;  // Opacity maps are typically used in the diffuse channel
                                        }
                                        else if( textureNameLower.find( "_height" ) !=
                                                     std::string::npos ||
                                                 textureNameLower.find( "_displacement" ) !=
                                                     std::string::npos )
                                        {
                                            textureType = PbsTextureTypes::
                                                PBSM_DIFFUSE;  // Height maps are typically used in the diffuse channel
                                        }

                                        // Add texture to material pass
                                        if( auto material =
                                                resourceDatabase->loadResourceByType<render::IMaterial>(
                                                    filePath ) )
                                        {
                                            material->setTexture( texture,
                                                                  static_cast<u32>( textureType ) );
                                            material->save();
                                        }
                                    }
                                }
                            }
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
}  // namespace workphone::editor
