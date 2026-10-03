#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/FoliageLoader.hpp>
#include <Workphone/Mesh/FoliageMesh.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/Vertex.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>

#if WP_USE_NGPLANT
#    include <ngpcore/p3dmodel.h>
#    include <ngpcore/p3dmodelstemtube.h>
#    include <ngpcore/p3dmodelstemquad.h>
#    include <ngpcore/p3dbalgbase.h>
#    include <ngpcore/p3dbalgstd.h>
#    include <ngpcore/p3diostream.h>
#    include <ngpcore/p3dhli.h>
#endif

namespace workphone
{
#if WP_USE_NGPLANT
    //--------------------------------------------
    class P3DMaterialInstanceSimple : public P3DMaterialInstance
    {
    public:
        const P3DMaterialDef *GetMaterialDef() const override
        {
            return ( &MatDef );
        }

        P3DMaterialInstance *CreateCopy() const override
        {
            return new P3DMaterialInstanceSimple;
        }

    private:
        P3DMaterialDef MatDef;
    };

    //--------------------------------------------
    class FBP3DMaterialFactory : public P3DMaterialFactory
    {
    public:
        FBP3DMaterialFactory()
        {
        }

        ~FBP3DMaterialFactory() override
        {
        }

        P3DMaterialInstance *CreateMaterial( const P3DMaterialDef &materialDef ) const override
        {
            return new P3DMaterialInstanceSimple;
        }
    };

    static void GetBranchGroupMaterial( const String &materialName, const P3DMaterialDef *MaterialDef )
    {
        ColourF matColour;
        MaterialDef->GetColor( &matColour.r, &matColour.g, &matColour.b );

        const char *TexName = MaterialDef->GetTexName( P3D_TEX_DIFFUSE );

        if( TexName != nullptr )
        {
        }

        bool DoubleSided = MaterialDef->IsDoubleSided();
        bool Transparent = MaterialDef->IsTransparent();

        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto materialManager = graphicsSystem->getMaterialManager();

        auto materialResult = materialManager->createOrRetrieve( materialName );
        auto fbMaterial = workphone::dynamic_pointer_cast<render::IMaterial>( materialResult.first );
        if( fbMaterial )
        {
            fbMaterial->setTexture( String( TexName ), 0 );
            fbMaterial->setTransparent( Transparent );
        }
    }

    static bool GetBranchGroupGeometry( AABB3F &box, SmartPtr<ISubMesh> subMesh,
                                        const P3DHLIPlantTemplate *PlantTemplate,
                                        const P3DHLIPlantInstance *PlantInstance,
                                        unsigned int GroupIndex )
    {
        unsigned int AttrCount;
        unsigned int BranchAttrCount;
        P3DHLIVAttrBuffers VAttrBuffers;
        unsigned int IndexCount;
        unsigned int BranchIndex;
        unsigned int BranchCount;

        AttrCount = PlantInstance->GetVAttrCountI( GroupIndex );

        if( AttrCount == 0 )
        {
            return ( true );
        }

        Array<Vertex> verts;
        verts.reserve( 1024 );

        Array<u32> indicies;
        indicies.reserve( 1024 );

        f32 *PosBuffer = new f32[3 * AttrCount];
        f32 *NormalBuffer = new f32[3 * AttrCount];
        f32 *TexCoordBuffer = new f32[2 * AttrCount];

        if( ( PosBuffer != 0 ) && ( NormalBuffer != 0 ) )
        {
            VAttrBuffers.AddAttr( P3D_ATTR_VERTEX, (void *)PosBuffer, 0, sizeof( float ) * 3 );
            VAttrBuffers.AddAttr( P3D_ATTR_NORMAL, (void *)NormalBuffer, 0, sizeof( float ) * 3 );
            VAttrBuffers.AddAttr( P3D_ATTR_TEXCOORD0, (void *)TexCoordBuffer, 0, sizeof( float ) * 2 );
        }
        else
        {
            return ( false );
        }

        PlantInstance->FillVAttrBuffersI( &VAttrBuffers, GroupIndex );

        auto reverseUVs = true;

        u32 numVerts = AttrCount;
        Vector3F *vertPositions = (Vector3F *)PosBuffer;
        Vector3F *vertNormals = (Vector3F *)NormalBuffer;
        Vector2F *vertTexCoords = (Vector2F *)TexCoordBuffer;
        for( u32 vertIdx = 0; vertIdx < numVerts; ++vertIdx )
        {
            Vertex vert;
            vert.position = vertPositions[vertIdx];
            vert.normal = vertNormals[vertIdx];

            if( reverseUVs )
            {
                vert.texCoord =
                    vertTexCoords[( numVerts - 1 ) - vertIdx];  //why do uvs have to be reversed?
            }
            else
            {
                vert.texCoord = vertTexCoords[vertIdx];
            }

            verts.push_back( vert );
        }

        BranchCount = PlantInstance->GetBranchCount( GroupIndex );
        IndexCount = PlantTemplate->GetIndexCount( GroupIndex, P3D_TRIANGLE_LIST );

        u32 meshIndexCount = IndexCount * BranchCount;
        u32 *IndexBuffer = new u32[meshIndexCount];

        BranchAttrCount = PlantTemplate->GetVAttrCountI( GroupIndex );

        for( BranchIndex = 0; BranchIndex < BranchCount; BranchIndex++ )
        {
            PlantTemplate->FillIndexBuffer( &( IndexBuffer[BranchIndex * IndexCount] ), GroupIndex,
                                            P3D_TRIANGLE_LIST, P3D_UNSIGNED_INT,
                                            BranchAttrCount * BranchIndex );
        }

        for( int i = 0; i < (int)meshIndexCount; i++ )
        //for (int i = (int)pSubMesh->indexData->indexCount - 1; i >= 0; i--)
        {
            u32 indexValue = IndexBuffer[i];
            indicies.push_back( indexValue );
        }

        return true;
    }

    static bool getBranchGroupGeometry2( AABB3F &box, SmartPtr<ISubMesh> subMesh,
                                         const P3DHLIPlantTemplate *PlantTemplate,
                                         const P3DHLIPlantInstance *PlantInstance,
                                         unsigned int GroupIndex )
    {
        unsigned int AttrCount;
        unsigned int BranchAttrCount;
        P3DHLIVAttrBuffers VAttrBuffers;
        unsigned int IndexCount;
        unsigned int BranchIndex;
        unsigned int BranchCount;

        AttrCount = PlantInstance->GetVAttrCountI( GroupIndex );

        if( AttrCount == 0 )
        {
            return ( true );
        }

        Array<Vertex> verts;
        verts.reserve( 1024 );

        Array<u32> indicies;
        indicies.reserve( 1024 );

        auto PosBuffer = new f32[3 * AttrCount];
        auto NormalBuffer = new f32[3 * AttrCount];
        auto TexCoordBuffer = new f32[2 * AttrCount];

        if( ( PosBuffer != nullptr ) && ( NormalBuffer != nullptr ) )
        {
            VAttrBuffers.AddAttr( P3D_ATTR_VERTEX, PosBuffer, 0, sizeof( float ) * 3 );
            VAttrBuffers.AddAttr( P3D_ATTR_NORMAL, NormalBuffer, 0, sizeof( float ) * 3 );
            VAttrBuffers.AddAttr( P3D_ATTR_TEXCOORD0, TexCoordBuffer, 0, sizeof( float ) * 2 );
        }
        else
        {
            return ( false );
        }

        PlantInstance->FillVAttrBuffersI( &VAttrBuffers, GroupIndex );

        auto reverseUVs = true;

        u32 numVerts = AttrCount;
        auto vertPositions = (Vector3F *)PosBuffer;
        auto vertNormals = (Vector3F *)NormalBuffer;
        auto vertTexCoords = (Vector2F *)TexCoordBuffer;
        for( u32 vertIdx = 0; vertIdx < numVerts; ++vertIdx )
        {
            Vertex vert;
            vert.position = vertPositions[vertIdx];
            vert.normal = vertNormals[vertIdx];

            if( reverseUVs )
            {
                vert.texCoord =
                    vertTexCoords[( numVerts - 1 ) - vertIdx];  //why do uvs have to be reversed?
            }
            else
            {
                vert.texCoord = vertTexCoords[vertIdx];
            }

            verts.push_back( vert );
        }

        BranchCount = PlantInstance->GetBranchCount( GroupIndex );
        IndexCount = PlantTemplate->GetIndexCount( GroupIndex, P3D_TRIANGLE_LIST );

        u32 meshIndexCount = IndexCount * BranchCount;
        auto pIndexBuffer = new u32[meshIndexCount];

        BranchAttrCount = PlantTemplate->GetVAttrCountI( GroupIndex );

        for( BranchIndex = 0; BranchIndex < BranchCount; BranchIndex++ )
        {
            PlantTemplate->FillIndexBuffer( &( pIndexBuffer[BranchIndex * IndexCount] ), GroupIndex,
                                            P3D_TRIANGLE_LIST, P3D_UNSIGNED_INT,
                                            BranchAttrCount * BranchIndex );
        }

        for( int i = 0; i < static_cast<int>( meshIndexCount ); ++i )
        // for (int i = (int)pSubMesh->indexData->indexCount - 1; i >= 0; i--)
        {
            u32 indexValue = pIndexBuffer[i];
            indicies.push_back( indexValue );
        }

        return true;
    }

    SmartPtr<IMesh> FoliageLoader::loadMesh( const String &plantFilePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();

            FBP3DMaterialFactory materialFactory;

            P3DInputStringStreamFile sourceStream;
            sourceStream.Open( plantFilePath.c_str() );

            P3DHLIPlantTemplate plantTemplate( &sourceStream );

            P3DHLIPlantInstance *plantInstance = plantTemplate.CreateInstance();

            u32 branchGroupCount = plantTemplate.GetGroupCount();

            AABB3F aabb;

            auto pMesh = workphone::make_ptr<Mesh>();

            Vector3F vMin( -20.f, 0.f, -20.f );
            Vector3F vMax( 20.f, 20.f, 20.f );
            Vector3F currPos;
            f32 maxSquaredRadius = 20 * 20;
            bool firstVert = true;

            if( branchGroupCount > 0 )
            {
                bool result = true;
                u32 branchGroupIndex = 0;
                while( ( branchGroupIndex < branchGroupCount ) && result )
                {
                    auto materialDef = plantTemplate.GetMaterial( branchGroupIndex );
                    auto textureName = materialDef->GetTexName( 0 );

                    auto materialName = Path::getFileNameWithoutExtension( plantFilePath ) + "_" +
                                        Path::getFileNameWithoutExtension( textureName );

                    // create subMesh
                    auto pSubMesh = workphone::make_ptr<SubMesh>();
                    pSubMesh->setMaterialName( materialName.c_str() );

                    result = getBranchGroupGeometry2( aabb, pSubMesh, &plantTemplate, plantInstance,
                                                      branchGroupIndex );

                    branchGroupIndex++;
                }
            }
            else
            {
                WP_LOG( "Plant model is empty" );
            }

            vMin = aabb.getMinimum();
            vMax = aabb.getMaximum();
            maxSquaredRadius = MathF::max( vMin.lengthSquared(), vMax.lengthSquared() );

            // Set bounds
            pMesh->setBoundingSphereRadius( MathF::Sqrt( maxSquaredRadius ) );
            pMesh->setAABB( AABB3F( vMin, vMax ) );

            return pMesh;
        }
        catch( P3DException &e )
        {
            WP_LOG( e.GetMessage() );
        }

        return nullptr;
    }
#else

    SmartPtr<IMesh> FoliageLoader::loadMesh( const String &meshName )
    {
        return nullptr;
    }

#endif
}  // namespace workphone
