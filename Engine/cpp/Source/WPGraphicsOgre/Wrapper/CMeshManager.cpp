#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMeshManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        CMeshManager::CMeshManager()
        {
        }

        CMeshManager::~CMeshManager()
        {
        }

        SmartPtr<IMesh> CMeshManager::loadMesh( const String &meshName )
        {
            String fileExt = Path::getFileExtension( meshName );
            if( fileExt == ( ".mesh" ) )
            {
                auto engine = core::IApplicationManager::instance();
                SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();

                String fileName = Path::getFileName( meshName );
                SmartPtr<IStream> stream = fileSystem->open( fileName );
                if( stream )
                {
                    auto mesh = workphone::make_ptr<Mesh>();

                    MeshSerializer meshSerializer;
                    meshSerializer.importMesh( stream, static_cast<Mesh *>( mesh.get() ) );

                    mesh->updateAABB( true );

                    return mesh;
                }
            }

            return nullptr;
        }

        SmartPtr<IResource> CMeshManager::create( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IResource> CMeshManager::create( const String &uuid, const String &name )
        {
            return nullptr;
        }

        void CMeshManager::destroyResource( SmartPtr<IResource> resource )
        {
        }

        void CMeshManager::destroyAll()
        {
        }

        SmartPtr<IResource> CMeshManager::loadResource( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IResource> CMeshManager::getByName( const String &name )
        {
            return nullptr;
        }

        SmartPtr<IResource> CMeshManager::getById( const String &uuid )
        {
            return nullptr;
        }

        void CMeshManager::_getObject( void **ppObject ) const
        {
        }

        Pair<SmartPtr<IResource>, bool> CMeshManager::createOrRetrieve( const String &uuid,
                                                                        const String &path,
                                                                        const String &type )
        {
            return Pair<SmartPtr<IResource>, bool>();
        }

        Pair<SmartPtr<IResource>, bool> CMeshManager::createOrRetrieve( const String &path )
        {
            return Pair<SmartPtr<IResource>, bool>();
        }

        void CMeshManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
        {
        }

        SmartPtr<IResource> CMeshManager::loadFromFile( const String &filePath )
        {
            return nullptr;
        }
    }  // end namespace render
}  // namespace workphone
