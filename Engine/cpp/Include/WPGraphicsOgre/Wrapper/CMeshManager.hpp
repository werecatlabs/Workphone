#ifndef CMeshManager_h__
#define CMeshManager_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{
    namespace render
    {
        class CMeshManager : public MeshManager
        {
        public:
            CMeshManager();
            ~CMeshManager() override;

            SmartPtr<IMesh> loadMesh( const String &fileName );

            SmartPtr<IResource> create( const String &name ) override;

            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            void destroyResource( SmartPtr<IResource> resource ) override;

            void destroyAll() override;

            SmartPtr<IResource> loadResource( const String &name ) override;

            SmartPtr<IResource> getByName( const String &name ) override;

            SmartPtr<IResource> getById( const String &uuid ) override;

            void _getObject( void **ppObject ) const override;

            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

        protected:
            using MeshResources = HashMap<u32, SmartPtr<IMeshResource>>;
            MeshResources m_meshResources;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CMeshManager_h__
