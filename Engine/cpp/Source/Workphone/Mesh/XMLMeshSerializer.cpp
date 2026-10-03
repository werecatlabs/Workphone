#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/XMLMeshSerializer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <tinyxml.h>

namespace workphone
{

    namespace
    {
        TiXmlElement *appendElement( TiXmlNode *parent, const char *name )
        {
            auto element = new TiXmlElement( name );
            parent->LinkEndChild( element );
            return element;
        }

        const char *getAttribute( const TiXmlElement *element, const char *name )
        {
            const char *value = element ? element->Attribute( name ) : nullptr;
            return value ? value : "";
        }
    }  // namespace

    XMLMeshSerializer::XMLMeshSerializer() = default;

    XMLMeshSerializer::~XMLMeshSerializer() = default;

    void XMLMeshSerializer::importMesh( const String &filename, VertexElementType colourElementType,
                                        Mesh *pMesh )
    {
        WP_LOG( "XMLMeshSerializer reading mesh data from " + filename + "..." );
        mMesh = pMesh;
        mColourElementType = colourElementType;
        TiXmlDocument mXMLDoc;
        if( !mXMLDoc.LoadFile( filename.c_str() ) )
        {
            WP_LOG_ERROR( "XMLMeshSerializer failed reading the XML file: " +
                          String( mXMLDoc.ErrorDesc() ) );
            return;
        }

        TiXmlElement *elem = nullptr;

        TiXmlElement *rootElem = mXMLDoc.RootElement();
        if( !rootElem )
        {
            WP_LOG_ERROR( "XMLMeshSerializer XML file has no root element." );
            return;
        }

        // shared geometry
        elem = rootElem->FirstChildElement( "sharedgeometry" );
        if( elem )
        {
            if( StringUtil::parseInt( getAttribute( elem, "vertexcount" ) ) > 0 )
            {
                auto sharedVertexData = new VertexBuffer();
                //mMesh->sharedVertexData = sharedVertexData;
                //readGeometry( elem, sharedVertexData );
            }
        }

        // submeshes
        elem = rootElem->FirstChildElement( "submeshes" );
        if( elem )
            readSubMeshes( elem );

        // skeleton link
        elem = rootElem->FirstChildElement( "skeletonlink" );
        if( elem )
            readSkeletonLink( elem );

        // bone assignments
        elem = rootElem->FirstChildElement( "boneassignments" );
        if( elem )
            readBoneAssignments( elem );

        //Lod
        elem = rootElem->FirstChildElement( "levelofdetail" );
        if( elem )
            readLodInfo( elem );

        // submesh names
        elem = rootElem->FirstChildElement( "submeshnames" );
        if( elem )
            readSubMeshNames( elem, mMesh );

        // submesh extremes
        elem = rootElem->FirstChildElement( "extremes" );
        if( elem )
            readExtremes( elem, mMesh );

        // poses
        elem = rootElem->FirstChildElement( "poses" );
        if( elem )
            readPoses( elem, mMesh );

        // animations
        elem = rootElem->FirstChildElement( "animations" );
        if( elem )
            readAnimations( elem, mMesh );

        WP_LOG( "XMLMeshSerializer import successful." );
    }

    void XMLMeshSerializer::exportMesh( const Mesh *pMesh, const String &filename )
    {
        WP_LOG( "XMLMeshSerializer writing mesh data to " + filename + "..." );

        mMesh = const_cast<Mesh *>( pMesh );

        TiXmlDocument mXMLDoc;
        TiXmlElement *rootNode = appendElement( &mXMLDoc, "mesh" );

        WP_LOG( "Populating DOM..." );

        // Write to DOM
        writeMesh( pMesh, rootNode );
        WP_LOG( "DOM populated, writing XML file.." );

        // Write out to a file
        if( !mXMLDoc.SaveFile( filename.c_str() ) )
        {
            WP_LOG_ERROR( "XMLMeshSerializer failed writing the XML file." );
        }
        else
        {
            WP_LOG( "XMLMeshSerializer export successful." );
        }
    }

    void XMLMeshSerializer::writeMesh( const Mesh *pMesh, TiXmlElement *rootNode )
    {
        // Write geometry
        if( pMesh->getSharedVertexBuffer() )
        {
            TiXmlElement *geomNode = appendElement( rootNode, "sharedgeometry" );
            writeGeometry( geomNode, pMesh->getSharedVertexBuffer() );
        }

        // Write Submeshes
        TiXmlElement *subMeshesNode = appendElement( rootNode, "submeshes" );
        for( size_t i = 0; i < pMesh->getNumSubMeshes(); ++i )
        {
            WP_LOG( "Writing submesh..." );
            writeSubMesh( subMeshesNode, pMesh->getSubMesh( (u32)i ) );
            WP_LOG( "Submesh exported." );
        }

        // Write skeleton info if required
        if( pMesh->hasSkeleton() )
        {
            WP_LOG( "Exporting skeleton link..." );
            // Write skeleton link
            writeSkeletonLink( rootNode, pMesh->getSkeletonName() );
            WP_LOG( "Skeleton link exported." );

            // Write bone assignments
            auto boneAssigns = pMesh->getBoneAssignments();
            if( !boneAssigns.empty() )
            {
                WP_LOG( "Exporting shared geometry bone assignments..." );
                TiXmlElement *boneAssignNode = appendElement( rootNode, "boneassignments" );

                for( auto &e : boneAssigns )
                {
                    writeBoneAssignment( boneAssignNode, e.get() );
                }

                WP_LOG( "Shared geometry bone assignments exported." );
            }
        }
        if( pMesh->getNumLodLevels() > 1 )
        {
            WP_LOG( "Exporting LOD information..." );
            writeLodInfo( rootNode, pMesh );
            WP_LOG( "LOD information exported." );
        }
        // Write submesh names
        writeSubMeshNames( rootNode, pMesh );
        // Write poses
        writePoses( rootNode, pMesh );
        // Write animations
        writeAnimations( rootNode, pMesh );
        // Write extremes
        writeExtremes( rootNode, pMesh );
    }

    void XMLMeshSerializer::writeSubMesh( TiXmlElement *mSubMeshesNode, const SmartPtr<SubMesh> s )
    {
        TiXmlElement *subMeshNode = appendElement( mSubMeshesNode, "submesh" );

        size_t numFaces;

        // Material name
        subMeshNode->SetAttribute( "material", s->getMaterialName().c_str() );
        // bool useSharedVertices
        subMeshNode->SetAttribute( "usesharedvertices",
                                   StringUtil::toString( s->getUseSharedVertices() ).c_str() );

        auto indexData = s->getIndexBuffer();

        // bool use32BitIndexes
        bool use32BitIndexes =
            ( indexData && indexData->getIndexType() == IIndexBuffer::Type::IT_32BIT );
        subMeshNode->SetAttribute( "use32bitindexes", StringUtil::toString( use32BitIndexes ).c_str() );

        // Operation type
        switch( s->getRenderOperationType() )
        {
        case RenderOperationType::OT_LINE_LIST:
            subMeshNode->SetAttribute( "operationtype", "line_list" );
            break;
        case RenderOperationType::OT_LINE_STRIP:
            subMeshNode->SetAttribute( "operationtype", "line_strip" );
            break;
        case RenderOperationType::OT_POINT_LIST:
            subMeshNode->SetAttribute( "operationtype", "point_list" );
            break;
        case RenderOperationType::OT_TRIANGLE_FAN:
            subMeshNode->SetAttribute( "operationtype", "triangle_fan" );
            break;
        case RenderOperationType::OT_TRIANGLE_LIST:
            subMeshNode->SetAttribute( "operationtype", "triangle_list" );
            break;
        case RenderOperationType::OT_TRIANGLE_STRIP:
            subMeshNode->SetAttribute( "operationtype", "triangle_strip" );
            break;
        case RenderOperationType::OT_TRIANGLE_LIST_ADJ:
            subMeshNode->SetAttribute( "operationtype", "triangle_list_adj" );
            break;
        case RenderOperationType::OT_TRIANGLE_STRIP_ADJ:
            subMeshNode->SetAttribute( "operationtype", "triangle_strip_adj" );
            break;
        case RenderOperationType::OT_LINE_LIST_ADJ:
            subMeshNode->SetAttribute( "operationtype", "line_list_adj" );
            break;
        case RenderOperationType::OT_LINE_STRIP_ADJ:
            subMeshNode->SetAttribute( "operationtype", "line_strip_adj" );
            break;
        default:
            WP_ASSERT( false );  // "Patch control point operations not supported" );
            break;
        }

        if( indexData->getNumIndices() > 0 )
        {
            // Faces
            TiXmlElement *facesNode = appendElement( subMeshNode, "faces" );
            switch( s->getRenderOperationType() )
            {
            case RenderOperationType::OT_TRIANGLE_LIST:
                // tri list
                numFaces = indexData->getNumIndices() / 3;

                break;
            case RenderOperationType::OT_LINE_LIST:
                numFaces = indexData->getNumIndices() / 2;

                break;
            case RenderOperationType::OT_TRIANGLE_FAN:
            case RenderOperationType::OT_TRIANGLE_STRIP:
                // triangle fan or triangle strip
                numFaces = indexData->getNumIndices() - 2;

                break;
            default:
            {
                //OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Unsupported render operation type" );
            }
            }

            facesNode->SetAttribute( "count",
                                     StringUtil::toString( static_cast<s64>( numFaces ) ).c_str() );
            // Write each face in turn
            size_t i;
            unsigned int *pInt = 0;
            unsigned short *pShort = 0;

            auto ibuf = indexData;
            if( use32BitIndexes )
            {
                pInt = static_cast<unsigned int *>( ibuf->getIndexData() );
            }
            else
            {
                pShort = static_cast<unsigned short *>( ibuf->getIndexData() );
            }

            for( i = 0; i < numFaces; ++i )
            {
                TiXmlElement *faceNode = appendElement( facesNode, "face" );
                if( use32BitIndexes )
                {
                    faceNode->SetAttribute( "v1", StringUtil::toString( *pInt++ ).c_str() );
                    if( s->getRenderOperationType() == RenderOperationType::OT_LINE_LIST )
                    {
                        faceNode->SetAttribute( "v2", StringUtil::toString( *pInt++ ).c_str() );
                    }
                    /// Only need all 3 vertex indices if trilist or first face
                    else if( s->getRenderOperationType() == RenderOperationType::OT_TRIANGLE_LIST ||
                             i == 0 )
                    {
                        faceNode->SetAttribute( "v2", StringUtil::toString( *pInt++ ).c_str() );
                        faceNode->SetAttribute( "v3", StringUtil::toString( *pInt++ ).c_str() );
                    }
                }
                else
                {
                    faceNode->SetAttribute( "v1", StringUtil::toString( *pShort++ ).c_str() );
                    if( s->getRenderOperationType() == RenderOperationType::OT_LINE_LIST )
                    {
                        faceNode->SetAttribute( "v2", StringUtil::toString( *pShort++ ).c_str() );
                    }
                    /// Only need all 3 vertex indices if trilist or first face
                    else if( s->getRenderOperationType() == RenderOperationType::OT_TRIANGLE_LIST ||
                             i == 0 )
                    {
                        faceNode->SetAttribute( "v2", StringUtil::toString( *pShort++ ).c_str() );
                        faceNode->SetAttribute( "v3", StringUtil::toString( *pShort++ ).c_str() );
                    }
                }
            }

            ibuf->unlock();
        }

        // M_GEOMETRY chunk (Optional: present only if useSharedVertices = false)
        if( !s->getUseSharedVertices() )
        {
            TiXmlElement *geomNode = appendElement( subMeshNode, "geometry" );
            writeGeometry( geomNode, s->getVertexBuffer() );
        }

        // texture aliases
        writeTextureAliases( subMeshNode, s );

        // Bone assignments
        if( mMesh->hasSkeleton() )
        {
            WP_LOG( "Exporting dedicated geometry bone assignments..." );

            TiXmlElement *boneAssignNode = appendElement( subMeshNode, "boneassignments" );
            for( const auto &e : s->getBoneAssignments() )
            {
                //writeBoneAssignment( boneAssignNode, &e.second );
            }
        }

        WP_LOG( "Dedicated geometry bone assignments exported." );
    }

    void XMLMeshSerializer::writeGeometry( TiXmlElement *mParentNode,
                                           const SmartPtr<IVertexBuffer> vertexData )
    {
        // Write a vertex buffer per element

        TiXmlElement *vbNode = nullptr;
        TiXmlElement *vertexNode = nullptr;
        TiXmlElement *dataNode = nullptr;

        // Set num verts on parent
        mParentNode->SetAttribute( "vertexcount",
                                   StringUtil::toString( vertexData->getNumVertices() ).c_str() );

        auto decl = vertexData->getVertexDeclaration();

        vbNode = appendElement( mParentNode, "vertexbuffer" );
        auto vbuf = vertexData;
        unsigned short bufferIdx = 0;
        // Get all the elements that relate to this buffer
        auto elems = decl->findElementsBySource( bufferIdx );

        // Set up the data access for this buffer (lock read-only)
        unsigned char *pVert;
        float *pFloat;
        u16 *pShort;
        u8 *pChar;
        ABGR *pColour;

        pVert = static_cast<u8 *>( vbuf->getVertexData() );

        // Skim over the elements to set up the general data
        unsigned short numTextureCoords = 0;
        for( auto elem : elems )
        {
            switch( (VertexElementSemantic)elem->getSemantic() )
            {
            case VertexElementSemantic::VES_POSITION:
                vbNode->SetAttribute( "positions", "true" );
                break;
            case VertexElementSemantic::VES_NORMAL:
                vbNode->SetAttribute( "normals", "true" );
                break;
            case VertexElementSemantic::VES_TANGENT:
                vbNode->SetAttribute( "tangents", "true" );
                if( elem->getType() == VertexElementType::VET_FLOAT4 )
                {
                    vbNode->SetAttribute( "tangent_dimensions", "4" );
                }
                break;
            case VertexElementSemantic::VES_BINORMAL:
                vbNode->SetAttribute( "binormals", "true" );
                break;
            case VertexElementSemantic::VES_DIFFUSE:
                vbNode->SetAttribute( "colours_diffuse", "true" );
                break;
            case VertexElementSemantic::VES_SPECULAR:
                vbNode->SetAttribute( "colours_specular", "true" );
                break;
            case VertexElementSemantic::VES_TEXTURE_COORDINATES:
            {
                const char *type = "float2";

                switch( elem->getType() )
                {
                case VertexElementType::VET_FLOAT1:
                    type = "float1";
                    break;
                case VertexElementType::VET_FLOAT2:
                    type = "float2";
                    break;
                case VertexElementType::VET_FLOAT3:
                    type = "float3";
                    break;
                case VertexElementType::VET_FLOAT4:
                    type = "float4";
                    break;
                case VertexElementType::VET_COLOUR:
                    //case VertexElementType::VET_COLOUR_ARGB:
                    //case VertexElementType::VET_COLOUR_ABGR:
                    type = "colour";
                    break;
                case VertexElementType::VET_SHORT1:
                    type = "short1";
                    break;
                case VertexElementType::VET_SHORT2:
                    type = "short2";
                    break;
                case VertexElementType::VET_SHORT3:
                    type = "short3";
                    break;
                case VertexElementType::VET_SHORT4:
                    type = "short4";
                    break;
                case VertexElementType::VET_UBYTE4:
                    type = "ubyte4";
                    break;
                default:
                    WP_ASSERT( false );  //  "Unsupported VET"
                    break;
                }
                vbNode->SetAttribute(
                    ( "texture_coord_dimensions_" + StringUtil::toString( numTextureCoords ) ).c_str(),
                    type );
                ++numTextureCoords;
            }
            break;

            default:
                break;
            }
        }
        if( numTextureCoords > 0 )
        {
            vbNode->SetAttribute( "texture_coords", StringUtil::toString( numTextureCoords ).c_str() );
        }

        // For each vertex
        for( size_t v = 0; v < vertexData->getNumVertices(); ++v )
        {
            vertexNode = appendElement( vbNode, "vertex" );
            // Iterate over the elements
            for( auto elem : elems )
            {
                switch( (VertexElementSemantic)elem->getSemantic() )
                {
                case VertexElementSemantic::VES_POSITION:
                    elem->baseVertexPointerToElement( pVert, &pFloat );
                    dataNode = appendElement( vertexNode, "position" );
                    dataNode->SetAttribute( "x", StringUtil::toString( pFloat[0] ).c_str() );
                    dataNode->SetAttribute( "y", StringUtil::toString( pFloat[1] ).c_str() );
                    dataNode->SetAttribute( "z", StringUtil::toString( pFloat[2] ).c_str() );
                    break;
                case VertexElementSemantic::VES_NORMAL:
                    elem->baseVertexPointerToElement( pVert, &pFloat );
                    dataNode = appendElement( vertexNode, "normal" );
                    dataNode->SetAttribute( "x", StringUtil::toString( pFloat[0] ).c_str() );
                    dataNode->SetAttribute( "y", StringUtil::toString( pFloat[1] ).c_str() );
                    dataNode->SetAttribute( "z", StringUtil::toString( pFloat[2] ).c_str() );
                    break;
                case VertexElementSemantic::VES_TANGENT:
                    elem->baseVertexPointerToElement( pVert, &pFloat );
                    dataNode = appendElement( vertexNode, "tangent" );
                    dataNode->SetAttribute( "x", StringUtil::toString( pFloat[0] ).c_str() );
                    dataNode->SetAttribute( "y", StringUtil::toString( pFloat[1] ).c_str() );
                    dataNode->SetAttribute( "z", StringUtil::toString( pFloat[2] ).c_str() );
                    if( elem->getType() == VertexElementType::VET_FLOAT4 )
                    {
                        dataNode->SetAttribute( "w", StringUtil::toString( pFloat[3] ).c_str() );
                    }
                    break;
                case VertexElementSemantic::VES_BINORMAL:
                    elem->baseVertexPointerToElement( pVert, &pFloat );
                    dataNode = appendElement( vertexNode, "binormal" );
                    dataNode->SetAttribute( "x", StringUtil::toString( pFloat[0] ).c_str() );
                    dataNode->SetAttribute( "y", StringUtil::toString( pFloat[1] ).c_str() );
                    dataNode->SetAttribute( "z", StringUtil::toString( pFloat[2] ).c_str() );
                    break;
                case VertexElementSemantic::VES_DIFFUSE:
                    elem->baseVertexPointerToElement( pVert, &pColour );
                    dataNode = appendElement( vertexNode, "colour_diffuse" );
                    {
                        ColourF cv;
                        elem->getType() == VertexElementType::VET_COLOUR_ARGB ? cv.setAsARGB( *pColour )
                                                                              : cv.setAsABGR( *pColour );
                        dataNode->SetAttribute( "value", StringUtil::toString( cv ).c_str() );
                    }
                    break;
                case VertexElementSemantic::VES_SPECULAR:
                    elem->baseVertexPointerToElement( pVert, &pColour );
                    dataNode = appendElement( vertexNode, "colour_specular" );
                    {
                        ColourF cv;
                        elem->getType() == VertexElementType::VET_COLOUR_ARGB ? cv.setAsARGB( *pColour )
                                                                              : cv.setAsABGR( *pColour );
                        dataNode->SetAttribute( "value", StringUtil::toString( cv ).c_str() );
                    }
                    break;
                case VertexElementSemantic::VES_TEXTURE_COORDINATES:
                    dataNode = appendElement( vertexNode, "texcoord" );

                    switch( elem->getType() )
                    {
                    case VertexElementType::VET_FLOAT1:
                        elem->baseVertexPointerToElement( pVert, &pFloat );
                        dataNode->SetAttribute( "u", StringUtil::toString( *pFloat++ ).c_str() );
                        break;
                    case VertexElementType::VET_FLOAT2:
                        elem->baseVertexPointerToElement( pVert, &pFloat );
                        dataNode->SetAttribute( "u", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "v", StringUtil::toString( *pFloat++ ).c_str() );
                        break;
                    case VertexElementType::VET_FLOAT3:
                        elem->baseVertexPointerToElement( pVert, &pFloat );
                        dataNode->SetAttribute( "u", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "v", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "w", StringUtil::toString( *pFloat++ ).c_str() );
                        break;
                    case VertexElementType::VET_FLOAT4:
                        elem->baseVertexPointerToElement( pVert, &pFloat );
                        dataNode->SetAttribute( "u", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "v", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "w", StringUtil::toString( *pFloat++ ).c_str() );
                        dataNode->SetAttribute( "x", StringUtil::toString( *pFloat++ ).c_str() );
                        break;
                    case VertexElementType::VET_SHORT1:
                        elem->baseVertexPointerToElement( pVert, &pShort );
                        dataNode->SetAttribute( "u",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        break;
                    case VertexElementType::VET_SHORT2:
                        elem->baseVertexPointerToElement( pVert, &pShort );
                        dataNode->SetAttribute( "u",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "v",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        break;
                    case VertexElementType::VET_SHORT3:
                        elem->baseVertexPointerToElement( pVert, &pShort );
                        dataNode->SetAttribute( "u",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "v",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "w",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        break;
                    case VertexElementType::VET_SHORT4:
                        elem->baseVertexPointerToElement( pVert, &pShort );
                        dataNode->SetAttribute( "u",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "v",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "w",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        dataNode->SetAttribute( "x",
                                                StringUtil::toString( *pShort++ / 65535.0f ).c_str() );
                        break;
                    case VertexElementType::VET_UBYTE4_NORM:
                        //case VertexElementType::VET_COLOUR:
                        //case VertexElementType::VET_COLOUR_ARGB:
                        //case VertexElementType::VET_COLOUR_ABGR:
                        elem->baseVertexPointerToElement( pVert, &pColour );
                        {
                            ColourF cv;
                            elem->getType() == VertexElementType::VET_COLOUR_ARGB
                                ? cv.setAsARGB( *pColour )
                                : cv.setAsABGR( *pColour );
                            dataNode->SetAttribute( "u", StringUtil::toString( cv ).c_str() );
                        }
                        break;
                    case VertexElementType::VET_UBYTE4:
                        elem->baseVertexPointerToElement( pVert, &pChar );
                        dataNode->SetAttribute( "u", StringUtil::toString( *pChar++ / 255.0f ).c_str() );
                        dataNode->SetAttribute( "v", StringUtil::toString( *pChar++ / 255.0f ).c_str() );
                        dataNode->SetAttribute( "w", StringUtil::toString( *pChar++ / 255.0f ).c_str() );
                        dataNode->SetAttribute( "x", StringUtil::toString( *pChar++ / 255.0f ).c_str() );
                        break;
                    default:
                        WP_ASSERT( false );  // "Unsupported VET"
                        break;
                    }
                    break;
                default:
                    break;
                }
            }

            pVert += decl->getSize();
        }

        vbuf->unlock();
    }

    void XMLMeshSerializer::writeSkeletonLink( TiXmlElement *mMeshNode, const String &skelName )
    {
        TiXmlElement *skelNode = appendElement( mMeshNode, "skeletonlink" );
        skelNode->SetAttribute( "name", skelName.c_str() );
    }

    void XMLMeshSerializer::writeBoneAssignment( TiXmlElement *mBoneAssignNode,
                                                 const IVertexBoneAssignment *assign )
    {
        /*
        TiXmlElement *assignNode = appendElement( mBoneAssignNode, "vertexboneassignment" );

        assignNode->SetAttribute( "vertexindex", StringConverter::toString(assign->vertexIndex).c_str() );
        assignNode->SetAttribute( "boneindex", StringConverter::toString(assign->boneIndex).c_str() );
        assignNode->SetAttribute( "weight", StringConverter::toString(assign->weight).c_str() );

    */
    }

    void XMLMeshSerializer::writeTextureAliases( TiXmlElement *mSubmeshesNode,
                                                 const SmartPtr<SubMesh> subMesh )
    {
        /*
        if( !subMesh->hasTextureAliases() )
            return;  // do nothing

        TiXmlElement *textureAliasesNode = appendElement( mSubmeshesNode, "textures" );

        // use ogre map iterator
        SubMesh::AliasTextureIterator aliasIterator = subMesh->getAliasTextureIterator();

        while( aliasIterator.hasMoreElements() )
        {
            TiXmlElement *aliasTextureNode = appendElement( textureAliasesNode, "texture" );
            // iterator key is alias and value is texture name
            aliasTextureNode->SetAttribute( "alias", aliasIterator.peekNextKey().c_str() );
            aliasTextureNode->SetAttribute( "name", aliasIterator.peekNextValue().c_str() );
            aliasIterator.moveNext();
        }
        */
    }

    void XMLMeshSerializer::readSubMeshes( TiXmlElement *mSubmeshesNode )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading submeshes..." );
        assert( mMesh->getNumSubMeshes() == 0 );
        for( TiXmlElement *smElem = mSubmeshesNode->FirstChildElement(); smElem;
             smElem = smElem->NextSiblingElement() )
        {
            // All children should be submeshes
            SubMesh *sm = mMesh->createSubMesh();

            const char *mat = smElem->Attribute( "material" );
            if( mat && mat[0] != '\0' )
            {
                // we do not load any materials - so create a dummy here to just store the name
                sm->setMaterial( MaterialManager::getSingleton().create( mat, RGN_DEFAULT ) );
            }
            else
            {
                LogManager::getSingleton().logError(
                    "empty material name encountered. This violates the specs and can lead to "
                    "crashes." );
            }

            // Read operation type
            bool readFaces = true;
            const char *optype = smElem->Attribute( "operationtype" );
            if( optype )
            {
                if( !strcmp( optype, "triangle_list" ) )
                {
                    sm->operationType = RenderOperation::OT_TRIANGLE_LIST;
                }
                else if( !strcmp( optype, "triangle_fan" ) )
                {
                    sm->operationType = RenderOperation::OT_TRIANGLE_FAN;
                }
                else if( !strcmp( optype, "triangle_strip" ) )
                {
                    sm->operationType = RenderOperation::OT_TRIANGLE_STRIP;
                }
                else if( !strcmp( optype, "line_strip" ) )
                {
                    sm->operationType = RenderOperation::OT_LINE_STRIP;
                    readFaces = false;
                }
                else if( !strcmp( optype, "line_list" ) )
                {
                    sm->operationType = RenderOperation::OT_LINE_LIST;
                    readFaces = false;
                }
                else if( !strcmp( optype, "point_list" ) )
                {
                    sm->operationType = RenderOperation::OT_POINT_LIST;
                    readFaces = false;
                }
                else if( !strcmp( optype, "triangle_list_adj" ) )
                {
                    sm->operationType = RenderOperation::OT_TRIANGLE_LIST_ADJ;
                }
                else if( !strcmp( optype, "triangle_strip_adj" ) )
                {
                    sm->operationType = RenderOperation::OT_TRIANGLE_STRIP_ADJ;
                }
                else if( !strcmp( optype, "line_strip_adj" ) )
                {
                    sm->operationType = RenderOperation::OT_LINE_STRIP_ADJ;
                    readFaces = false;
                }
                else if( !strcmp( optype, "line_list_adj" ) )
                {
                    sm->operationType = RenderOperation::OT_LINE_LIST_ADJ;
                    readFaces = false;
                }
            }

            sm->useSharedVertices =
                StringConverter::parseBool( getAttribute( smElem, "usesharedvertices" ) );
            bool use32BitIndexes =
                StringConverter::parseBool( getAttribute( smElem, "use32bitindexes" ) );

            // Faces
            if( readFaces )
            {
                TiXmlElement *faces = smElem->FirstChildElement( "faces" );
                int actualCount = std::distance( faces.begin(), faces.end() );
                const char *claimedCount_ = getAttribute( faces, "count" );
                if( StringConverter::parseInt( claimedCount_ ) != actualCount )
                {
                    LogManager::getSingleton().stream( LML_WARNING )
                        << "WARNING: face count (" << actualCount << ") " << "is not as claimed ("
                        << claimedCount_ << ")";
                }

                if( actualCount > 0 )
                {
                    // Faces
                    switch( sm->operationType )
                    {
                    case RenderOperation::OT_TRIANGLE_LIST:
                        // tri list
                        sm->indexData->indexCount = actualCount * 3;

                        break;
                    case RenderOperation::OT_LINE_LIST:
                        sm->indexData->indexCount = actualCount * 2;

                        break;
                    case RenderOperation::OT_TRIANGLE_FAN:
                    case RenderOperation::OT_TRIANGLE_STRIP:
                        // triangle fan or triangle strip
                        sm->indexData->indexCount = actualCount + 2;

                        break;
                    default:
                        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "operationType not implemented" );
                    }

                    // Allocate space
                    HardwareIndexBufferSharedPtr ibuf =
                        HardwareBufferManager::getSingleton().createIndexBuffer(
                            use32BitIndexes ? HardwareIndexBuffer::IT_32BIT
                                            : HardwareIndexBuffer::IT_16BIT,
                            sm->indexData->indexCount, HardwareBuffer::HBU_DYNAMIC, false );
                    sm->indexData->indexBuffer = ibuf;
                    unsigned int *pInt = 0;
                    unsigned short *pShort = 0;
                    if( use32BitIndexes )
                    {
                        pInt = static_cast<unsigned int *>( ibuf->lock( HardwareBuffer::HBL_DISCARD ) );
                    }
                    else
                    {
                        pShort =
                            static_cast<unsigned short *>( ibuf->lock( HardwareBuffer::HBL_DISCARD ) );
                    }

                    bool firstTri = true;
                    for( TiXmlElement *faceElem = faces->FirstChildElement(); faceElem;
             faceElem = faceElem->NextSiblingElement() )
                    {
                        if( use32BitIndexes )
                        {
                            *pInt++ = StringConverter::parseInt( getAttribute( faceElem, "v1" ) );
                            if( sm->operationType == RenderOperation::OT_LINE_LIST )
                            {
                                *pInt++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v2" ) );
                            }
                            // only need all 3 vertices if it's a trilist or first tri
                            else if( sm->operationType == RenderOperation::OT_TRIANGLE_LIST || firstTri )
                            {
                                *pInt++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v2" ) );
                                *pInt++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v3" ) );
                            }
                        }
                        else
                        {
                            *pShort++ = StringConverter::parseInt( getAttribute( faceElem, "v1" ) );
                            if( sm->operationType == RenderOperation::OT_LINE_LIST )
                            {
                                *pShort++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v2" ) );
                            }
                            // only need all 3 vertices if it's a trilist or first tri
                            else if( sm->operationType == RenderOperation::OT_TRIANGLE_LIST || firstTri )
                            {
                                *pShort++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v2" ) );
                                *pShort++ =
                                    StringConverter::parseInt( getAttribute( faceElem, "v3" ) );
                            }
                        }
                        firstTri = false;
                    }
                    ibuf->unlock();
                }
            }

            // Geometry
            if( !sm->useSharedVertices )
            {
                TiXmlElement *geomNode = smElem->FirstChildElement( "geometry" );
                if( geomNode )
                {
                    sm->vertexData = new VertexData();
                    readGeometry( geomNode, sm->vertexData );
                }
            }

            // texture aliases
            TiXmlElement *textureAliasesNode = smElem->FirstChildElement( "textures" );
            if( textureAliasesNode )
                readTextureAliases( textureAliasesNode, sm );

            // Bone assignments
            TiXmlElement *boneAssigns = smElem->FirstChildElement( "boneassignments" );
            if( boneAssigns )
                readBoneAssignments( boneAssigns, sm );
        }
        LogManager::getSingleton().logMessage( "Submeshes done." );
        */
    }

    void XMLMeshSerializer::readGeometry( TiXmlElement *mGeometryNode, IVertexBuffer *vertexData )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading geometry..." );
        unsigned char *pVert;
        float *pFloat;
        uint16 *pShort;
        uint8 *pChar;
        ARGB *pCol;

        ptrdiff_t claimedVertexCount =
            StringConverter::parseInt( getAttribute( mGeometryNode, "vertexcount" ) );

        // Skip empty
        if( claimedVertexCount <= 0 )
            return;

        VertexDeclaration *decl = vertexData->vertexDeclaration;
        VertexBufferBinding *bind = vertexData->vertexBufferBinding;
        unsigned short bufCount = 0;
        unsigned short totalTexCoords = 0;  // across all buffers

        // Information for calculating bounds
        Vector3 min = Vector3::ZERO, max = Vector3::UNIT_SCALE, pos = Vector3::ZERO;
        Real maxSquaredRadius = -1;
        bool first = true;

        // Iterate over all children (vertexbuffer entries)
        for( TiXmlElement *vbElem = mGeometryNode->FirstChildElement( "vertexbuffer" ); vbElem;
             vbElem = vbElem->NextSiblingElement( "vertexbuffer" ) )
        {
            size_t offset = 0;
            if( StringConverter::parseBool( getAttribute( vbElem, "positions" ) ) )
            {
                offset += decl->addElement( bufCount, offset, VET_FLOAT3, VES_POSITION ).getSize();
            }
            if( StringConverter::parseBool( getAttribute( vbElem, "normals" ) ) )
            {
                offset += decl->addElement( bufCount, offset, VET_FLOAT3, VES_NORMAL ).getSize();
            }
            if( StringConverter::parseBool( getAttribute( vbElem, "tangents" ) ) )
            {
                VertexElementType tangentType = VET_FLOAT3;
                unsigned int dims = StringConverter::parseUnsignedInt(
                    getAttribute( vbElem, "tangent_dimensions" ) );
                if( dims == 4 )
                    tangentType = VET_FLOAT4;

                offset += decl->addElement( bufCount, offset, tangentType, VES_TANGENT ).getSize();
            }
            if( StringConverter::parseBool( getAttribute( vbElem, "binormals" ) ) )
            {
                offset += decl->addElement( bufCount, offset, VET_FLOAT3, VES_BINORMAL ).getSize();
            }
            if( StringConverter::parseBool( getAttribute( vbElem, "colours_diffuse" ) ) )
            {
                offset +=
                    decl->addElement( bufCount, offset, mColourElementType, VES_DIFFUSE ).getSize();
            }
            if( StringConverter::parseBool( getAttribute( vbElem, "colours_specular" ) ) )
            {
                // Add element
                offset +=
                    decl->addElement( bufCount, offset, mColourElementType, VES_SPECULAR ).getSize();
            }
            if( StringConverter::parseInt( getAttribute( vbElem, "texture_coords" ) ) )
            {
                unsigned short numTexCoords =
                    StringConverter::parseInt( getAttribute( vbElem, "texture_coords" ) );
                for( unsigned short tx = 0; tx < numTexCoords; ++tx )
                {
                    // NB set is local to this buffer, but will be translated into a
                    // global set number across all vertex buffers
                    StringStream str;
                    str << "texture_coord_dimensions_" << tx;
                    auto attrib = vbElem->Attribute( str.str().c_str() );
                    VertexElementType vtype = VET_FLOAT2;  // Default
                    if( attrib )
                    {
                        if( !::strcmp( attrib, "1" ) )
                            vtype = VET_FLOAT1;
                        else if( !::strcmp( attrib, "2" ) )
                            vtype = VET_FLOAT2;
                        else if( !::strcmp( attrib, "3" ) )
                            vtype = VET_FLOAT3;
                        else if( !::strcmp( attrib, "4" ) )
                            vtype = VET_FLOAT4;
                        else if( !::strcmp( attrib, "float1" ) )
                            vtype = VET_FLOAT1;
                        else if( !::strcmp( attrib, "float2" ) )
                            vtype = VET_FLOAT2;
                        else if( !::strcmp( attrib, "float3" ) )
                            vtype = VET_FLOAT3;
                        else if( !::strcmp( attrib, "float4" ) )
                            vtype = VET_FLOAT4;
                        else if( !::strcmp( attrib, "short1" ) )
                            vtype = VET_SHORT1;
                        else if( !::strcmp( attrib, "short2" ) )
                            vtype = VET_SHORT2;
                        else if( !::strcmp( attrib, "short3" ) )
                            vtype = VET_SHORT3;
                        else if( !::strcmp( attrib, "short4" ) )
                            vtype = VET_SHORT4;
                        else if( !::strcmp( attrib, "ubyte4" ) )
                            vtype = VET_UBYTE4;
                        else if( !::strcmp( attrib, "colour" ) )
                            vtype = VET_COLOUR;
                        else if( !::strcmp( attrib, "colour_argb" ) )
                            vtype = VET_COLOUR_ARGB;
                        else if( !::strcmp( attrib, "colour_abgr" ) )
                            vtype = VET_COLOUR_ABGR;
                        else
                        {
                            auto err = LogManager::getSingleton().stream( LML_CRITICAL );
                            err << "Did not recognise texture_coord_dimensions value of \"" << attrib
                                << "\"\n";
                            err << "Falling back to default of VET_FLOAT2\n";
                        }
                    }
                    offset += decl->addElement( bufCount, offset, vtype, VES_TEXTURE_COORDINATES,
                                                totalTexCoords++ )
                                  .getSize();
                }
            }

            // calculate how many vertexes there actually are
            int actualVertexCount = std::distance( vbElem.begin(), vbElem.end() );
            if( actualVertexCount != claimedVertexCount )
            {
                LogManager::getSingleton().stream( LML_WARNING )
                    << "WARNING: vertex count (" << actualVertexCount << ") is not as claimed ("
                    << claimedVertexCount << ")";
            }

            vertexData->vertexCount = actualVertexCount;
            // Now create the vertex buffer
            HardwareVertexBufferSharedPtr vbuf =
                HardwareBufferManager::getSingleton().createVertexBuffer(
                    offset, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false );
            // Bind it
            bind->setBinding( bufCount, vbuf );
            // Lock it
            pVert = static_cast<unsigned char *>( vbuf->lock( HardwareBuffer::HBL_DISCARD ) );

            // Get the element list for this buffer alone
            VertexDeclaration::VertexElementList elems = decl->findElementsBySource( bufCount );
            // Now the buffer is set up, parse all the vertices
            for( TiXmlElement *vertexElem = vbElem->FirstChildElement(); vertexElem;
             vertexElem = vertexElem->NextSiblingElement() )
            {
                // Now parse the elements, ensure they are all matched
                VertexDeclaration::VertexElementList::const_iterator ielem, ielemend;
                TiXmlElement *xmlElem;
                TiXmlElement *texCoordElem;
                ielemend = elems.end();
                for( ielem = elems.begin(); ielem != ielemend; ++ielem )
                {
                    const VertexElement &elem = *ielem;
                    // Find child for this element
                    switch( elem.getSemantic() )
                    {
                    case VES_POSITION:
                        xmlElem = vertexElem->FirstChildElement( "position" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Missing <position> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pFloat );

                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "y" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "z" ) );

                        pos.x = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                        pos.y = StringConverter::parseReal( getAttribute( xmlElem, "y" ) );
                        pos.z = StringConverter::parseReal( getAttribute( xmlElem, "z" ) );

                        if( first )
                        {
                            min = max = pos;
                            maxSquaredRadius = pos.squaredLength();
                            first = false;
                        }
                        else
                        {
                            min.makeFloor( pos );
                            max.makeCeil( pos );
                            maxSquaredRadius = std::max( pos.squaredLength(), maxSquaredRadius );
                        }
                        break;
                    case VES_NORMAL:
                        xmlElem = vertexElem->FirstChildElement( "normal" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Missing <normal> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pFloat );

                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "y" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "z" ) );
                        break;
                    case VES_TANGENT:
                        xmlElem = vertexElem->FirstChildElement( "tangent" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Missing <tangent> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pFloat );

                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "y" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "z" ) );
                        if( elem.getType() == VET_FLOAT4 )
                        {
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "w" ) );
                        }
                        break;
                    case VES_BINORMAL:
                        xmlElem = vertexElem->FirstChildElement( "binormal" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Missing <binormal> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pFloat );

                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "y" ) );
                        *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "z" ) );
                        break;
                    case VES_DIFFUSE:
                        xmlElem = vertexElem->FirstChildElement( "colour_diffuse" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                         "Missing <colour_diffuse> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pCol );
                        {
                            ColourValue cv;
                            cv = StringConverter::parseColourValue(
                                getAttribute( xmlElem, "value" ) );
                            *pCol++ = VertexElement::convertColourValue( cv, mColourElementType );
                        }
                        break;
                    case VES_SPECULAR:
                        xmlElem = vertexElem->FirstChildElement( "colour_specular" );
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                         "Missing <colour_specular> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        elem.baseVertexPointerToElement( pVert, &pCol );
                        {
                            ColourValue cv;
                            cv = StringConverter::parseColourValue(
                                getAttribute( xmlElem, "value" ) );
                            *pCol++ = VertexElement::convertColourValue( cv, mColourElementType );
                        }
                        break;
                    case VES_TEXTURE_COORDINATES:
                        if( !texCoordElem )
                        {
                            // Get first texcoord
                            xmlElem = vertexElem->FirstChildElement( "texcoord" );
                        }
                        else
                        {
                            // Get next texcoord
                            xmlElem = texCoordElem->NextSiblingElement( "texcoord" );
                        }
                        if( !xmlElem )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Missing <texcoord> element.",
                                         "XMLSerializer::readGeometry" );
                        }
                        // Record the latest texture coord entry
                        texCoordElem = xmlElem;

                        if( !xmlElem->Attribute( "u" ) )
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                         "Texcoord 'u' attribute not found.",
                                         "XMLMeshSerializer::readGeometry" );

                        // depending on type, pack appropriately, can process colour channels separately which is a bonus
                        switch( elem.getType() )
                        {
                        case VET_FLOAT1:
                            elem.baseVertexPointerToElement( pVert, &pFloat );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "u" ) );
                            break;

                        case VET_FLOAT2:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pFloat );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "u" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "v" ) );
                            break;

                        case VET_FLOAT3:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "w" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'w' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pFloat );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "u" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "v" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "w" ) );
                            break;

                        case VET_FLOAT4:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "w" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'w' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "x" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'x' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pFloat );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "u" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "v" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "w" ) );
                            *pFloat++ = StringConverter::parseReal( getAttribute( xmlElem, "x" ) );
                            break;

                        case VET_SHORT1:
                            elem.baseVertexPointerToElement( pVert, &pShort );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "u" ) ) );
                            break;

                        case VET_SHORT2:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pShort );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "u" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "v" ) ) );
                            break;

                        case VET_SHORT3:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "w" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'w' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pShort );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "u" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "v" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "w" ) ) );
                            break;

                        case VET_SHORT4:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "w" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'w' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "x" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'x' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pShort );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "u" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "v" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "w" ) ) );
                            *pShort++ =
                                static_cast<uint16>( 65535.0f * StringConverter::parseReal(
                                                                    getAttribute( xmlElem, "x" ) ) );
                            break;

                        case VET_UBYTE4:
                            if( !xmlElem->Attribute( "v" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'v' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "w" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'w' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            if( !xmlElem->Attribute( "x" ) )
                                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                             "Texcoord 'x' attribute not found.",
                                             "XMLMeshSerializer::readGeometry" );
                            elem.baseVertexPointerToElement( pVert, &pChar );
                            // round off instead of just truncating -- avoids magnifying rounding errors
                            *pChar++ = static_cast<uint8>(
                                0.5f + 255.0f * StringConverter::parseReal(
                                                    getAttribute( xmlElem, "u" ) ) );
                            *pChar++ = static_cast<uint8>(
                                0.5f + 255.0f * StringConverter::parseReal(
                                                    getAttribute( xmlElem, "v" ) ) );
                            *pChar++ = static_cast<uint8>(
                                0.5f + 255.0f * StringConverter::parseReal(
                                                    getAttribute( xmlElem, "w" ) ) );
                            *pChar++ = static_cast<uint8>(
                                0.5f + 255.0f * StringConverter::parseReal(
                                                    getAttribute( xmlElem, "x" ) ) );
                            break;

                        case VET_COLOUR:
                        {
                            elem.baseVertexPointerToElement( pVert, &pCol );
                            ColourValue cv =
                                StringConverter::parseColourValue( getAttribute( xmlElem, "u" ) );
                            *pCol++ = VertexElement::convertColourValue( cv, mColourElementType );
                        }
                        break;

                        case VET_COLOUR_ARGB:
                        case VET_COLOUR_ABGR:
                        {
                            elem.baseVertexPointerToElement( pVert, &pCol );
                            ColourValue cv =
                                StringConverter::parseColourValue( getAttribute( xmlElem, "u" ) );
                            *pCol++ = VertexElement::convertColourValue( cv, elem.getType() );
                        }
                        break;
                        default:
                            OgreAssert( false, "Unsupported VET" );
                            break;
                        }

                        break;
                    default:
                        break;
                    }
                }  // semantic
                pVert += vbuf->getVertexSize();
            }  // vertex
            bufCount++;
            vbuf->unlock();
        }  // vertexbuffer

        // Set bounds
        const AxisAlignedBox &currBox = mMesh->getBounds();
        Real currRadius = mMesh->getBoundingSphereRadius();
        if( currBox.isNull() )
        {
            //do not pad the bounding box
            mMesh->_setBounds( AxisAlignedBox( min, max ), false );
            mMesh->_setBoundingSphereRadius( Math::Sqrt( maxSquaredRadius ) );
        }
        else
        {
            AxisAlignedBox newBox( min, max );
            newBox.merge( currBox );
            //do not pad the bounding box
            mMesh->_setBounds( newBox, false );
            mMesh->_setBoundingSphereRadius( std::max( Math::Sqrt( maxSquaredRadius ), currRadius ) );
        }

        LogManager::getSingleton().logMessage( "Geometry done..." );
        */
    }

    void XMLMeshSerializer::readSkeletonLink( TiXmlElement *mSkelNode )
    {
        mMesh->setSkeletonName( getAttribute( mSkelNode, "name" ) );
    }

    void XMLMeshSerializer::readBoneAssignments( TiXmlElement *mBoneAssignmentsNode )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading bone assignments..." );

        // Iterate over all children (vertexboneassignment entries)
        for( TiXmlElement *elem = mBoneAssignmentsNode->FirstChildElement(); elem;
             elem = elem->NextSiblingElement() )
        {
            VertexBoneAssignment vba;
            vba.vertexIndex = StringConverter::parseInt( getAttribute( elem, "vertexindex" ) );
            vba.boneIndex = StringConverter::parseInt( getAttribute( elem, "boneindex" ) );
            vba.weight = StringConverter::parseReal( getAttribute( elem, "weight" ) );

            mMesh->addBoneAssignment( vba );
        }

        LogManager::getSingleton().logMessage( "Bone assignments done." );
        */
    }

    void XMLMeshSerializer::readTextureAliases( TiXmlElement *mTextureAliasesNode, SubMesh *subMesh )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading sub mesh texture aliases..." );

        // Iterate over all children (texture entries)
        for( TiXmlElement *elem = mTextureAliasesNode->FirstChildElement(); elem;
             elem = elem->NextSiblingElement() )
        {
            // pass alias and texture name to submesh
            // read attribute "alias"
            String alias = getAttribute( elem, "alias" );
            // read attribute "name"
            String name = getAttribute( elem, "name" );

            subMesh->addTextureAlias( alias, name );
        }

        LogManager::getSingleton().logMessage( "Texture aliases done." );
        */
    }

    void XMLMeshSerializer::readSubMeshNames( TiXmlElement *mMeshNamesNode, Mesh *sm )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading mesh names..." );

        // Iterate over all children (vertexboneassignment entries)
        for( TiXmlElement *elem = mMeshNamesNode->FirstChildElement(); elem;
             elem = elem->NextSiblingElement() )
        {
            String meshName = getAttribute( elem, "name" );
            int index = StringConverter::parseInt( getAttribute( elem, "index" ) );

            sm->nameSubMesh( meshName, index );
        }

        LogManager::getSingleton().logMessage( "Mesh names done." );
        */
    }

    void XMLMeshSerializer::readBoneAssignments( TiXmlElement *mBoneAssignmentsNode, SubMesh *sm )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading bone assignments..." );
        // Iterate over all children (vertexboneassignment entries)
        for( TiXmlElement *elem = mBoneAssignmentsNode->FirstChildElement(); elem;
             elem = elem->NextSiblingElement() )
        {
            VertexBoneAssignment vba;
            vba.vertexIndex = StringConverter::parseInt( getAttribute( elem, "vertexindex" ) );
            vba.boneIndex = StringConverter::parseInt( getAttribute( elem, "boneindex" ) );
            vba.weight = StringConverter::parseReal( getAttribute( elem, "weight" ) );

            sm->addBoneAssignment( vba );
        }
        LogManager::getSingleton().logMessage( "Bone assignments done." );
        */
    }

    void XMLMeshSerializer::writeLodInfo( TiXmlElement *mMeshNode, const Mesh *pMesh )
    {
        /*
        TiXmlElement *lodNode = appendElement( mMeshNode, "levelofdetail" );

        const LodStrategy *strategy = pMesh->getLodStrategy();
        unsigned short numLvls = pMesh->getNumLodLevels();
        bool manual = pMesh->hasManualLodLevel();
        lodNode->SetAttribute( "strategy", strategy->getName().c_str() );
        lodNode->SetAttribute( "numlevels", StringConverter::toString( numLvls ).c_str() );
        lodNode->SetAttribute( "manual", StringConverter::toString( manual ).c_str() );

        // Iterate from level 1, not 0 (full detail)
        for( unsigned short i = 1; i < numLvls; ++i )
        {
            const MeshLodUsage &usage = pMesh->getLodLevel( i );
            if( pMesh->_isManualLodLevel( i ) )
            {
                writeLodUsageManual( lodNode, i, usage );
            }
            else
            {
                writeLodUsageGenerated( lodNode, i, usage, pMesh );
            }
        }
        */
    }

    void XMLMeshSerializer::writeSubMeshNames( TiXmlElement *mMeshNode, const Mesh *m )
    {
        /*
                const Mesh::SubMeshNameMap &nameMap = m->getSubMeshNameMap();
                if( nameMap.empty() )
                    return;  // do nothing

                TiXmlElement *namesNode = appendElement( mMeshNode, "submeshnames" );
                Mesh::SubMeshNameMap::const_iterator i, iend;
                iend = nameMap.end();
                for( i = nameMap.begin(); i != iend; ++i )
                {
                    TiXmlElement *subNameNode = appendElement( namesNode, "submeshname" );

                    subNameNode->SetAttribute( "name", i->first.c_str() );
                    subNameNode->SetAttribute( "index", StringConverter::toString( i->second ).c_str() );
                }
                */
    }

    void XMLMeshSerializer::writeLodUsageManual( TiXmlElement *usageNode, unsigned short levelNum,
                                                 const MeshLodUsage &usage )
    {
        //TiXmlElement *manualNode = appendElement( usageNode, "lodmanual" );

        //manualNode->SetAttribute( "value", StringConverter::toString( usage.userValue ).c_str() );
        //manualNode->SetAttribute( "meshname", usage.manualName.c_str() );
    }

    void XMLMeshSerializer::writeLodUsageGenerated( TiXmlElement *usageNode, unsigned short levelNum,
                                                    const MeshLodUsage &usage, const Mesh *pMesh )
    {
        /*
        TiXmlElement *generatedNode = appendElement( usageNode, "lodgenerated" );
        generatedNode->SetAttribute( "value", StringConverter::toString( usage.userValue ).c_str() );

        // Iterate over submeshes at this level
        size_t numsubs = pMesh->getNumSubMeshes();

        for( size_t subi = 0; subi < numsubs; ++subi )
        {
            TiXmlElement *subNode = appendElement( generatedNode, "lodfacelist" );
            SubMesh *sub = pMesh->getSubMesh( subi );
            subNode->SetAttribute( "submeshindex", StringConverter::toString( subi ).c_str() );
            // NB level - 1 because SubMeshes don't store the first index in geometry
            IndexData *facedata = sub->mLodFaceList[levelNum - 1];
            subNode->SetAttribute( "numfaces", StringConverter::toString( facedata->indexCount / 3 ).c_str() );

            if( facedata->indexCount > 0 )
            {
                // Write each face in turn
                bool use32BitIndexes =
                    ( facedata->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT );

                // Write each face in turn
                unsigned int *pInt = 0;
                unsigned short *pShort = 0;
                HardwareIndexBufferSharedPtr ibuf = facedata->indexBuffer;
                if( use32BitIndexes )
                {
                    pInt = static_cast<unsigned int *>( ibuf->lock( HardwareBuffer::HBL_READ_ONLY ) );
                    pInt += facedata->indexStart;
                }
                else
                {
                    pShort =
                        static_cast<unsigned short *>( ibuf->lock( HardwareBuffer::HBL_READ_ONLY ) );
                    pShort += facedata->indexStart;
                }

                for( size_t f = 0; f < facedata->indexCount; f += 3 )
                {
                    TiXmlElement *faceNode = appendElement( subNode, "face" );
                    if( use32BitIndexes )
                    {
                        faceNode->SetAttribute( "v1", StringConverter::toString( *pInt++ ).c_str() );
                        faceNode->SetAttribute( "v2", StringConverter::toString( *pInt++ ).c_str() );
                        faceNode->SetAttribute( "v3", StringConverter::toString( *pInt++ ).c_str() );
                    }
                    else
                    {
                        faceNode->SetAttribute( "v1", StringConverter::toString( *pShort++ ).c_str() );
                        faceNode->SetAttribute( "v2", StringConverter::toString( *pShort++ ).c_str() );
                        faceNode->SetAttribute( "v3", StringConverter::toString( *pShort++ ).c_str() );
                    }
                }

                ibuf->unlock();
            }
        }
        */
    }

    void XMLMeshSerializer::writeExtremes( TiXmlElement *mMeshNode, const Mesh *m )
    {
        /*
        TiXmlElement *extremesNode = nullptr;
        size_t submeshCount = m->getNumSubMeshes();
        for( size_t idx = 0; idx < submeshCount; ++idx )
        {
            SubMesh *sm = m->getSubMesh( idx );
            if( sm->extremityPoints.empty() )
                continue;  // do nothing

            if( !extremesNode )
                extremesNode = appendElement( mMeshNode, "extremes" );

            TiXmlElement *submeshNode = appendElement( extremesNode, "submesh_extremes" );

            submeshNode->SetAttribute( "index", StringConverter::toString( idx ).c_str() );

            for( std::vector<Vector3>::const_iterator v = sm->extremityPoints.begin();
                 v != sm->extremityPoints.end(); ++v )
            {
                TiXmlElement *vert = appendElement( submeshNode, "position" );
                vert->SetAttribute( "x", StringConverter::toString( v->x ).c_str() );
                vert->SetAttribute( "y", StringConverter::toString( v->y ).c_str() );
                vert->SetAttribute( "z", StringConverter::toString( v->z ).c_str() );
            }
        }
        */
    }

    void XMLMeshSerializer::readLodInfo( TiXmlElement *lodNode )
    {
        /*
        LogManager::getSingleton().logMessage( "Parsing LOD information..." );

        const char *strategyAttr = lodNode->Attribute( "strategy" );
        // This attribute is optional to maintain backwards compatibility
        if( attrValue )
        {
            String strategyName = strategyAttr;
            LodStrategy *strategy = LodStrategyManager::getSingleton().getStrategy( strategyName );
            mMesh->setLodStrategy( strategy );
        }

        attrValue = getAttribute( lodNode, "numlevels" );
        unsigned short numLevels =
            static_cast<unsigned short>( StringConverter::parseUnsignedInt( attrValue ) );

        attrValue = getAttribute( lodNode, "manual" );
        StringConverter::parseBool( attrValue );

        // Set up the basic structures
        mMesh->_setLodInfo( numLevels );

        // Parse the detail, start from 1 (the first sub-level of detail)
        unsigned short i = 1;
        for( TiXmlElement *usageElem = lodNode->FirstChildElement(); usageElem;
             usageElem = usageElem->NextSiblingElement() )
        {
            if( usageElem->Value() == String( "lodmanual" ) )
            {
                readLodUsageManual( usageElem, i );
            }
            else if( usageElem->Value() == String( "lodgenerated" ) )
            {
                readLodUsageGenerated( usageElem, i );
            }
            ++i;
        }

        LogManager::getSingleton().logMessage( "LOD information done." );
        */
    }

    void XMLMeshSerializer::readLodUsageManual( TiXmlElement *manualNode, unsigned short index )
    {
        /*
        MeshLodUsage usage;
        const char *attrValue = manualNode->Attribute( "value" );

        // If value attribute not found check for old name
        if( !attrValue )
        {
            attrValue = manualNode->Attribute( "fromdepthsquared" );
            if( attrValue )
                LogManager::getSingleton().logWarning(
                    "'fromdepthsquared' attribute has been renamed to 'value'." );
            // user values are non-squared
            usage.userValue = Math::Sqrt( StringConverter::parseReal( attrValue ) );
        }
        else
        {
            usage.userValue = StringConverter::parseReal( attrValue );
        }
        usage.value = mMesh->getLodStrategy()->transformUserValue( usage.userValue );
        usage.manualName = getAttribute( manualNode, "meshname" );
        usage.edgeData = NULL;

        // Generate for mixed
        size_t numSubs, i;
        numSubs = mMesh->getNumSubMeshes();
        for( i = 0; i < numSubs; ++i )
        {
            SubMesh *sm = mMesh->getSubMesh( i );
            sm->mLodFaceList[index - 1] = OGRE_NEW IndexData();
        }
        mMesh->_setLodUsage( index, usage );
        */
    }

    void XMLMeshSerializer::readLodUsageGenerated( TiXmlElement *genNode, unsigned short index )
    {
        /*
        MeshLodUsage usage;
        const char *attrValue = genNode->Attribute( "value" );

        // If value attribute not found check for old name
        if( !attrValue )
        {
            attrValue = getAttribute( genNode, "fromdepthsquared" );
            if( attrValue )
                LogManager::getSingleton().logWarning(
                    "'fromdepthsquared' attribute has been renamed to 'value'." );
            // user values are non-squared
            usage.userValue = Math::Sqrt( StringConverter::parseReal( attrValue ) );
        }
        else
        {
            usage.userValue = StringConverter::parseReal( attrValue );
        }
        usage.value = mMesh->getLodStrategy()->transformUserValue( usage.userValue );
        usage.manualMesh.reset();
        usage.manualName = "";
        usage.edgeData = NULL;

        mMesh->_setLodUsage( index, usage );

        // Read submesh face lists

        HardwareIndexBufferSharedPtr ibuf;
        for( TiXmlElement *faceListElem = genNode->FirstChildElement( "lodfacelist" ); faceListElem;
             faceListElem = faceListElem->NextSiblingElement( "lodfacelist" ) )
        {
            attrValue = getAttribute( faceListElem, "submeshindex" );
            unsigned short subidx = StringConverter::parseUnsignedInt( attrValue );
            attrValue = getAttribute( faceListElem, "numfaces" );
            unsigned short numFaces = StringConverter::parseUnsignedInt( attrValue );
            if( numFaces )
            {
                // use of 32bit indexes depends on submesh
                HardwareIndexBuffer::IndexType itype =
                    mMesh->getSubMesh( subidx )->indexData->indexBuffer->getType();
                bool use32bitindexes = ( itype == HardwareIndexBuffer::IT_32BIT );

                // Assign memory: this will be deleted by the submesh
                ibuf = HardwareBufferManager::getSingleton().createIndexBuffer(
                    itype, numFaces * 3, HardwareBuffer::HBU_STATIC_WRITE_ONLY );

                unsigned short *pShort = 0;
                unsigned int *pInt = 0;
                if( use32bitindexes )
                {
                    pInt = static_cast<unsigned int *>( ibuf->lock( HardwareBuffer::HBL_DISCARD ) );
                }
                else
                {
                    pShort = static_cast<unsigned short *>( ibuf->lock( HardwareBuffer::HBL_DISCARD ) );
                }
                TiXmlElement *faceElem = faceListElem->FirstChildElement( "face" );
                for( unsigned int face = 0; face < numFaces; ++face, faceElem = faceElem->NextSiblingElement() )
                {
                    if( use32bitindexes )
                    {
                        attrValue = getAttribute( faceElem, "v1" );
                        *pInt++ = StringConverter::parseUnsignedInt( attrValue );
                        attrValue = getAttribute( faceElem, "v2" );
                        *pInt++ = StringConverter::parseUnsignedInt( attrValue );
                        attrValue = getAttribute( faceElem, "v3" );
                        *pInt++ = StringConverter::parseUnsignedInt( attrValue );
                    }
                    else
                    {
                        attrValue = getAttribute( faceElem, "v1" );
                        *pShort++ = StringConverter::parseUnsignedInt( attrValue );
                        attrValue = getAttribute( faceElem, "v2" );
                        *pShort++ = StringConverter::parseUnsignedInt( attrValue );
                        attrValue = getAttribute( faceElem, "v3" );
                        *pShort++ = StringConverter::parseUnsignedInt( attrValue );
                    }
                }

                ibuf->unlock();
            }
            IndexData *facedata = new IndexData();  // will be deleted by SubMesh
            facedata->indexCount = numFaces * 3;
            facedata->indexStart = 0;
            facedata->indexBuffer = ibuf;
            mMesh->_setSubMeshLodFaceList( subidx, index, facedata );
        }
        */
    }

    void XMLMeshSerializer::readExtremes( TiXmlElement *extremesNode, Mesh *m )
    {
        /*
        LogManager::getSingleton().logMessage( "Reading extremes..." );

        // Iterate over all children (submesh_extreme list)
        for( TiXmlElement *elem = extremesNode->FirstChildElement(); elem;
             elem = elem->NextSiblingElement() )
        {
            int index = StringConverter::parseInt( getAttribute( elem, "index" ) );

            SubMesh *sm = m->getSubMesh( index );
            sm->extremityPoints.clear();
            for( TiXmlElement *vert = elem->FirstChildElement(); vert;
             vert = vert->NextSiblingElement() )
            {
                Vector3 v;
                v.x = StringConverter::parseReal( getAttribute( vert, "x" ) );
                v.y = StringConverter::parseReal( getAttribute( vert, "y" ) );
                v.z = StringConverter::parseReal( getAttribute( vert, "z" ) );
                sm->extremityPoints.push_back( v );
            }
        }

        LogManager::getSingleton().logMessage( "Extremes done." );
        */
    }

    void XMLMeshSerializer::readPoses( TiXmlElement *posesNode, Mesh *m )
    {
        /*
        for( TiXmlElement *poseNode = posesNode->FirstChildElement( "pose" ); poseNode;
             poseNode = poseNode->NextSiblingElement( "pose" ) )
        {
            const char *target = poseNode->Attribute( "target" );
            if( !target )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Required attribute 'target' missing on pose",
                             "XMLMeshSerializer::readPoses" );
            }
            unsigned short targetID;
            if( String( target ) == "mesh" )
            {
                targetID = 0;
            }
            else
            {
                // submesh, get index
                const char *attrValue = poseNode->Attribute( "index" );
                if( !attrValue )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Required attribute 'index' missing on pose",
                                 "XMLMeshSerializer::readPoses" );
                }
                unsigned short submeshIndex =
                    static_cast<unsigned short>( StringConverter::parseUnsignedInt( attrValue ) );

                targetID = submeshIndex + 1;
            }

            String name;
            const char *attrValue = poseNode->Attribute( "name" );
            if( attrValue )
                name = attrValue;
            Pose *pose = m->createPose( targetID, name );

            for( TiXmlElement *poseOffsetNode = poseNode->FirstChildElement( "poseoffset" ); poseOffsetNode;
             poseOffsetNode = poseOffsetNode->NextSiblingElement( "poseoffset" ) )
            {
                uint index =
                    StringConverter::parseUnsignedInt( getAttribute( poseOffsetNode, "index" ) );
                Vector3 offset;
                offset.x = StringConverter::parseReal( getAttribute( poseOffsetNode, "x" ) );
                offset.y = StringConverter::parseReal( getAttribute( poseOffsetNode, "y" ) );
                offset.z = StringConverter::parseReal( getAttribute( poseOffsetNode, "z" ) );

                if( getAttribute( poseOffsetNode, "nx" ) &&
                    getAttribute( poseOffsetNode, "ny" ) &&
                    getAttribute( poseOffsetNode, "nz" ) )
                {
                    Vector3 normal;
                    normal.x = StringConverter::parseReal( getAttribute( poseOffsetNode, "nx" ) );
                    normal.y = StringConverter::parseReal( getAttribute( poseOffsetNode, "ny" ) );
                    normal.z = StringConverter::parseReal( getAttribute( poseOffsetNode, "nz" ) );
                    pose->addVertex( index, offset, normal );
                }
                else
                {
                    pose->addVertex( index, offset );
                }
            }
        }
        */
    }

    void XMLMeshSerializer::readAnimations( TiXmlElement *mAnimationsNode, Mesh *pMesh )
    {
        /*
        for( TiXmlElement *animElem = mAnimationsNode->FirstChildElement( "animation" ); animElem;
             animElem = animElem->NextSiblingElement( "animation" ) )
        {
            String name = getAttribute( animElem, "name" );
            Real len = StringConverter::parseReal( getAttribute( animElem, "length" ) );

            Animation *anim = pMesh->createAnimation( name, len );

            TiXmlElement *baseInfoNode = animElem->FirstChildElement( "baseinfo" );
            if( baseInfoNode )
            {
                String baseName = getAttribute( baseInfoNode, "baseanimationname" );
                Real baseTime =
                    StringConverter::parseReal( getAttribute( baseInfoNode, "basekeyframetime" ) );
                anim->setUseBaseKeyFrame( true, baseTime, baseName );
            }

            TiXmlElement *tracksNode = animElem->FirstChildElement( "tracks" );
            if( tracksNode )
            {
                readTracks( tracksNode, pMesh, anim );
            }
        }
        */
    }

    void XMLMeshSerializer::readTracks( TiXmlElement *tracksNode, Mesh *m, IAnimation *anim )
    {
        /*
        for( TiXmlElement *trackNode = tracksNode->FirstChildElement( "track" ); trackNode;
             trackNode = trackNode->NextSiblingElement( "track" ) )
        {
            String target = getAttribute( trackNode, "target" );
            unsigned short targetID;
            VertexData *vertexData = 0;
            if( target == "mesh" )
            {
                targetID = 0;
                vertexData = m->sharedVertexData;
            }
            else
            {
                // submesh, get index
                const char *attrValue = trackNode->Attribute( "index" );
                if( !attrValue )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Required attribute 'index' missing on submesh track",
                                 "XMLMeshSerializer::readTracks" );
                }
                unsigned short submeshIndex =
                    static_cast<unsigned short>( StringConverter::parseUnsignedInt( attrValue ) );

                targetID = submeshIndex + 1;
                vertexData = m->getSubMesh( submeshIndex )->vertexData;
            }

            if( !vertexData )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Track cannot be created for " + target +
                                 " since VertexData "
                                 "does not exist at the specified index",
                             "XMLMeshSerializer::readTracks" );
            }

            // Get type
            VertexAnimationType animType = VAT_NONE;
            String strAnimType = getAttribute( trackNode, "type" );
            if( strAnimType == "morph" )
            {
                animType = VAT_MORPH;
            }
            else if( strAnimType == "pose" )
            {
                animType = VAT_POSE;
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Unrecognised animation track type '" + strAnimType + "'",
                             "XMLMeshSerializer::readTracks" );
            }

            // Create track
            VertexAnimationTrack *track = anim->createVertexTrack( targetID, vertexData, animType );

            TiXmlElement *keyframesNode = trackNode->FirstChildElement( "keyframes" );
            if( keyframesNode )
            {
                if( track->getAnimationType() == VAT_MORPH )
                {
                    readMorphKeyFrames( keyframesNode, track, vertexData->vertexCount );
                }
                else  // VAT_POSE
                {
                    readPoseKeyFrames( keyframesNode, track );
                }
            }
        }
        */
    }

    void XMLMeshSerializer::readMorphKeyFrames( TiXmlElement *keyframesNode,
                                                IAnimationVertexTrack *track, size_t vertexCount )
    {
        /*
        for( TiXmlElement *keyNode = keyframesNode->FirstChildElement( "keyframe" ); keyNode;
             keyNode = keyNode->NextSiblingElement( "keyframe" ) )
        {
            const char *attrValue = keyNode->Attribute( "time" );
            if( !attrValue )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Required attribute 'time' missing on keyframe",
                             "XMLMeshSerializer::readKeyFrames" );
            }
            Real time = StringConverter::parseReal( attrValue );

            VertexMorphKeyFrame *kf = track->createVertexMorphKeyFrame( time );

            bool includesNormals = keyNode->FirstChildElement( "normal" );

            size_t vertexSize = sizeof( float ) * ( includesNormals ? 6 : 3 );
            // create a vertex buffer
            HardwareVertexBufferSharedPtr vbuf =
                HardwareBufferManager::getSingleton().createVertexBuffer(
                    vertexSize, vertexCount, HardwareBuffer::HBU_STATIC, true );

            float *pFloat = static_cast<float *>( vbuf->lock( HardwareBuffer::HBL_DISCARD ) );

            TiXmlElement *posNode = keyNode->FirstChildElement( "position" );
            TiXmlElement *normNode = keyNode->FirstChildElement( "normal" );
            for( size_t v = 0; v < vertexCount; ++v )
            {
                if( !posNode )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Not enough 'position' elements under keyframe",
                                 "XMLMeshSerializer::readKeyFrames" );
                }

                *pFloat++ = StringConverter::parseReal( getAttribute( posNode, "x" ) );
                *pFloat++ = StringConverter::parseReal( getAttribute( posNode, "y" ) );
                *pFloat++ = StringConverter::parseReal( getAttribute( posNode, "z" ) );

                if( includesNormals )
                {
                    if( !normNode )
                    {
                        OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                     "Not enough 'normal' elements under keyframe",
                                     "XMLMeshSerializer::readKeyFrames" );
                    }

                    *pFloat++ = StringConverter::parseReal( getAttribute( normNode, "x" ) );
                    *pFloat++ = StringConverter::parseReal( getAttribute( normNode, "y" ) );
                    *pFloat++ = StringConverter::parseReal( getAttribute( normNode, "z" ) );
                    normNode = normNode->NextSiblingElement( "normal" );
                }

                posNode = posNode->NextSiblingElement( "position" );
            }

            vbuf->unlock();

            kf->setVertexBuffer( vbuf );
        }
        */
    }

    void XMLMeshSerializer::readPoseKeyFrames( TiXmlElement *keyframesNode,
                                               IAnimationVertexTrack *track )
    {
        /*
        for( TiXmlElement *keyNode = keyframesNode->FirstChildElement( "keyframe" ); keyNode;
             keyNode = keyNode->NextSiblingElement( "keyframe" ) )
        {
            const char *attrValue = keyNode->Attribute( "time" );
            if( !attrValue )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Required attribute 'time' missing on keyframe",
                             "XMLMeshSerializer::readKeyFrames" );
            }
            Real time = StringConverter::parseReal( attrValue );

            VertexPoseKeyFrame *kf = track->createVertexPoseKeyFrame( time );

            // Read all pose references
            for( TiXmlElement *poseRefNode = keyNode->FirstChildElement( "poseref" ); poseRefNode;
             poseRefNode = poseRefNode->NextSiblingElement( "poseref" ) )
            {
                const char *attr = poseRefNode->Attribute( "poseindex" );
                if( !attr )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Required attribute 'poseindex' missing on poseref",
                                 "XMLMeshSerializer::readPoseKeyFrames" );
                }
                unsigned short poseIndex = StringConverter::parseUnsignedInt( attr );
                Real influence = 1.0f;
                attr = poseRefNode->Attribute( "influence" );
                if( attr )
                {
                    influence = StringConverter::parseReal( attr );
                }

                kf->addPoseReference( poseIndex, influence );
            }
        }
        */
    }

    void XMLMeshSerializer::writePoses( TiXmlElement *meshNode, const Mesh *m )
    {
        /*
        if( m->getPoseList().empty() )
            return;

        TiXmlElement *posesNode = appendElement( meshNode, "poses" );

        PoseList::const_iterator it;
        for( it = m->getPoseList().begin(); it != m->getPoseList().end(); ++it )
        {
            const Pose *pose = *it;
            TiXmlElement *poseNode = appendElement( posesNode, "pose" );
            unsigned short target = pose->getTarget();
            if( target == 0 )
            {
                // Main mesh
                poseNode->SetAttribute( "target", "mesh" );
            }
            else
            {
                // Submesh - rebase index
                poseNode->SetAttribute( "target", "submesh" );
                poseNode->SetAttribute( "index", StringConverter::toString( target - 1 ).c_str() );
            }
            poseNode->SetAttribute( "name", pose->getName().c_str() );

            bool includesNormals = !pose->getNormals().empty();
            auto nit = pose->getNormals().begin();
            for( const auto &vit : pose->getVertexOffsets() )
            {
                TiXmlElement *poseOffsetElement = appendElement( poseNode, "poseoffset" );

                poseOffsetElement->SetAttribute( "index", StringConverter::toString( vit.first ).c_str() );

                const Vector3 &offset = vit.second;
                poseOffsetElement->SetAttribute( "x", StringConverter::toString( offset.x ).c_str() );
                poseOffsetElement->SetAttribute( "y", StringConverter::toString( offset.y ).c_str() );
                poseOffsetElement->SetAttribute( "z", StringConverter::toString( offset.z ).c_str() );

                if( includesNormals )
                {
                    const Vector3 &normal = nit->second;
                    poseOffsetElement->SetAttribute( "nx", StringConverter::toString( normal.x ).c_str() );
                    poseOffsetElement->SetAttribute( "ny", StringConverter::toString( normal.y ).c_str() );
                    poseOffsetElement->SetAttribute( "nz", StringConverter::toString( normal.z ).c_str() );
                    nit++;
                }
            }
        }
        */
    }

    void XMLMeshSerializer::writeAnimations( TiXmlElement *meshNode, const Mesh *m )
    {
        /*
        // Skip if no animation
        if( !m->hasVertexAnimation() )
            return;

        TiXmlElement *animationsNode = appendElement( meshNode, "animations" );

        for( unsigned short a = 0; a < m->getNumAnimations(); ++a )
        {
            Animation *anim = m->getAnimation( a );

            TiXmlElement *animNode = appendElement( animationsNode, "animation" );
            animNode->SetAttribute( "name", anim->getName().c_str() );
            animNode->SetAttribute( "length", StringConverter::toString( anim->getLength() ).c_str() );

            // Optional base keyframe information
            if( anim->getUseBaseKeyFrame() )
            {
                TiXmlElement *baseInfoNode = appendElement( animNode, "baseinfo" );
                baseInfoNode->SetAttribute( "baseanimationname", anim->getBaseKeyFrameAnimationName().c_str() );
                baseInfoNode->SetAttribute( "basekeyframetime", StringConverter::toString( anim->getBaseKeyFrameTime() ).c_str() );
            }

            TiXmlElement *tracksNode = appendElement( animNode, "tracks" );
            for( const auto &trackIt : anim->_getVertexTrackList() )
            {
                const VertexAnimationTrack *track = trackIt.second;
                TiXmlElement *trackNode = appendElement( tracksNode, "track" );

                unsigned short targetID = trackIt.first;
                if( targetID == 0 )
                {
                    trackNode->SetAttribute( "target", "mesh" );
                }
                else
                {
                    trackNode->SetAttribute( "target", "submesh" );
                    trackNode->SetAttribute( "index", StringConverter::toString( targetID - 1 ).c_str() );
                }

                if( track->getAnimationType() == VAT_MORPH )
                {
                    trackNode->SetAttribute( "type", "morph" );
                    writeMorphKeyFrames( trackNode, track );
                }
                else
                {
                    trackNode->SetAttribute( "type", "pose" );
                    writePoseKeyFrames( trackNode, track );
                }
            }
        }
        */
    }

    void XMLMeshSerializer::writeMorphKeyFrames( TiXmlElement *trackNode,
                                                 const IAnimationVertexTrack *track )
    {
        /*
        TiXmlElement *keyframesNode = appendElement( trackNode, "keyframes" );

        size_t vertexCount = track->getAssociatedVertexData()->vertexCount;

        for( unsigned short k = 0; k < track->getNumKeyFrames(); ++k )
        {
            VertexMorphKeyFrame *kf = track->getVertexMorphKeyFrame( k );
            TiXmlElement *keyNode = appendElement( keyframesNode, "keyframe" );
            keyNode->SetAttribute( "time", StringConverter::toString( kf->getTime() ).c_str() );

            HardwareVertexBufferSharedPtr vbuf = kf->getVertexBuffer();

            bool includesNormals = vbuf->getVertexSize() > ( sizeof( float ) * 3 );

            float *pFloat = static_cast<float *>( vbuf->lock( HardwareBuffer::HBL_READ_ONLY ) );

            for( size_t v = 0; v < vertexCount; ++v )
            {
                TiXmlElement *posNode = appendElement( keyNode, "position" );
                posNode->SetAttribute( "x", StringConverter::toString( *pFloat++ ).c_str() );
                posNode->SetAttribute( "y", StringConverter::toString( *pFloat++ ).c_str() );
                posNode->SetAttribute( "z", StringConverter::toString( *pFloat++ ).c_str() );

                if( includesNormals )
                {
                    TiXmlElement *normNode = appendElement( keyNode, "normal" );
                    normNode->SetAttribute( "x", StringConverter::toString( *pFloat++ ).c_str() );
                    normNode->SetAttribute( "y", StringConverter::toString( *pFloat++ ).c_str() );
                    normNode->SetAttribute( "z", StringConverter::toString( *pFloat++ ).c_str() );
                }
            }
        }
        */
    }

    void XMLMeshSerializer::writePoseKeyFrames( TiXmlElement *trackNode,
                                                const IAnimationVertexTrack *track )
    {
        /*
        TiXmlElement *keyframesNode = appendElement( trackNode, "keyframes" );

        for( unsigned short k = 0; k < track->getNumKeyFrames(); ++k )
        {
            VertexPoseKeyFrame *kf = track->getVertexPoseKeyFrame( k );
            TiXmlElement *keyNode = appendElement( keyframesNode, "keyframe" );
            keyNode->SetAttribute( "time", StringConverter::toString( kf->getTime() ).c_str() );

            VertexPoseKeyFrame::PoseRefList::const_iterator poseIt = kf->getPoseReferences().begin();
            for( ; poseIt != kf->getPoseReferences().end(); ++poseIt )
            {
                const VertexPoseKeyFrame::PoseRef &poseRef = *poseIt;
                TiXmlElement *poseRefNode = appendElement( keyNode, "poseref" );

                poseRefNode->SetAttribute( "poseindex", StringConverter::toString( poseRef.poseIndex ).c_str() );
                poseRefNode->SetAttribute( "influence", StringConverter::toString( poseRef.influence ).c_str() );
            }
        }
        */
    }

}  // namespace workphone
