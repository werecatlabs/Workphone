#ifndef MeshLoader_h__
#define MeshLoader_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <OgreMesh.h>

namespace workphone
{
    namespace render
    {

        class MeshLoader : public ISharedObject
        {
        public:
            MeshLoader();
            ~MeshLoader();

            static void createV2Mesh( Ogre::Mesh *mesh, SmartPtr<IMesh> fbmesh );
            static void createV2Mesh( Ogre::Mesh *mesh, SmartPtr<IMesh> fbmesh,
                                      const ProgressiveMeshOptions &options );

            static void loadFBMesh( Ogre::MeshPtr mesh, const String &meshPath );
            static void loadFBMesh( Ogre::Mesh *mesh, const String &meshPath );
            static void loadFBMesh( Ogre::Mesh *mesh, const String &meshPath,
                                    const ProgressiveMeshOptions &options );

            static Ogre::Mesh *loadFBMesh( SmartPtr<IStream> stream );

            static void loadFBMesh( Ogre::MeshPtr mesh, SmartPtr<IMesh> fbMesh );
            static void loadFBMesh( Ogre::Mesh *mesh, SmartPtr<IResource> resource );

            static Ogre::v1::MeshPtr convertFBMeshToOgreMesh( const String &newMeshName,
                                                              SmartPtr<IMesh> mesh );

            static Ogre::IndexBufferPacked *importFromV1( SmartPtr<IIndexBuffer> indexData );
            static void importBuffersFromV1( Ogre::MeshPtr newMesh, Ogre::SubMesh *pSubMesh,
                                             SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh,
                                             bool halfPos, bool halfTexCoords, bool qTangents,
                                             bool halfPose, size_t vaoPassIdx );
            static void importPosesFromV1( SmartPtr<ISubMesh> subMesh,
                                           Ogre::VertexBufferPacked *vertexBuffer, bool halfPrecision );
            static void importFromV1( Ogre::MeshPtr newMesh, Ogre::SubMesh *newSubMesh,
                                      SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh, bool halfPos,
                                      bool halfTexCoords, bool qTangents, bool halfPose );
            static void importV1( IMesh *mesh, bool halfPos, bool halfTexCoords, bool qTangents,
                                  bool halfPose );

            void loadMesh( const String &meshName );
            SmartPtr<IMesh> loadEngineMesh( const String &meshName );

            SmartPtr<IMesh> load( const String &meshName, SmartPtr<IGraphicsSceneNode> fbParent,
                                  SmartPtr<render::IGraphicsScene> smgr );

            bool getUseSingleMesh() const;
            void setUseSingleMesh( bool useSingleMesh );

            bool getQuietMode() const;
            void setQuietMode( bool quietMode );

            SmartPtr<IResourceManager> getMeshMgr() const;
            void setMeshMgr( SmartPtr<IResourceManager> meshManager );

        protected:
            Array<SmartPtr<IMesh>> m_meshes;
            bool m_useSingleMesh;
            bool m_quietMode;

            SmartPtr<IResourceManager> m_meshManager;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // MeshLoader_h__
