#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshConverter.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{

    MeshConverter::MeshConverter() = default;

    MeshConverter::~MeshConverter() = default;

    void MeshConverter::writeMesh( const SmartPtr<scene::IGameActor> actor )
    {
        try
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();

                auto meshComponent = actor->getComponent<scene::Mesh>();
                if( meshComponent )
                {
                    auto meshResource = meshComponent->getMeshResource();
                    auto mesh = meshResource->getMesh();
                    auto pMesh = mesh.get();
                    if( pMesh )
                    {
                        MeshSerializer serializer;

                        auto cacheFolder = applicationManager->getCachePath();
                        if( StringUtil::isNullOrEmpty( cacheFolder ) )
                        {
                            WP_LOG_ERROR( "MeshConverter::writeMesh: Cache path is empty." );
                            return;
                        }
                        if( !Path::isPathAbsolute( cacheFolder ) )
                        {
                            auto projectFolder = applicationManager->getProjectPath();
                            if( StringUtil::isNullOrEmpty( projectFolder ) )
                            {
                                projectFolder = Path::getWorkingDirectory();
                            }
                            cacheFolder = Path::getAbsolutePath( projectFolder, cacheFolder );
                        }
                        Path::createDirectories( cacheFolder );
                        auto fileName = pMesh->getName();

                        static const auto ext = String( ".fbmeshbin" );
                        if( fileName.find( ext ) == String::npos )
                        {
                            fileName += ext;
                        }

                        fileName = Path::getFileName( fileName );
                        const auto outputPath = Path::lexically_normal( cacheFolder, fileName );
                        serializer.exportMesh( static_cast<Mesh *>( pMesh ), outputPath );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            auto children = actor->getChildren();
            for( auto child : children )
            {
                writeMesh( child );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

}  // namespace workphone
