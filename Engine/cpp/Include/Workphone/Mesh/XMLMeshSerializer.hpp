#ifndef __XMLMeshSerializer_H__
#define __XMLMeshSerializer_H__

#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include "Mesh.hpp"

class TiXmlElement;

namespace workphone
{

    /** Class for serializing a Mesh to/from XML.
    @remarks
        This class behaves the same way as MeshSerializer in the main project,
        but is here to allow conversions to / from XML. This class is
        deliberately not included in the main project because <UL>
        <LI>Dependence on Xerces would unnecessarily bloat the main library</LI>
        <LI>Runtime use of XML is discouraged because of the parsing overhead</LI></UL>
        This class gives people the option of saving out a Mesh as XML for examination
        and possible editing. It can then be converted back to the native format
        for maximum runtime efficiency.
    */
    class XMLMeshSerializer
    {
    public:
        XMLMeshSerializer();
        virtual ~XMLMeshSerializer();

        /** Imports a Mesh from the given XML file.
        @param filename The name of the file to import, expected to be in XML format.
        @param colourElementType The vertex element to use for packed colours
        @param pMesh The pre-created Mesh object to be populated.
        */
        void importMesh( const String &filename, VertexElementType colourElementType, Mesh *pMesh );

        /** Exports a mesh to the named XML file. */
        void exportMesh( const Mesh *pMesh, const String &filename );

    protected:
        // State for import
        Mesh *mMesh;
        VertexElementType mColourElementType;

        // Internal methods
        void writeMesh( const Mesh *pMesh, TiXmlElement *rootNode );
        void writeSubMesh( TiXmlElement *mSubmeshesNode, const SmartPtr<SubMesh> s );
        void writeGeometry( TiXmlElement *mParentNode, const SmartPtr<IVertexBuffer> pData );
        void writeSkeletonLink( TiXmlElement *mMeshNode, const String &skelName );
        void writeBoneAssignment( TiXmlElement *mBoneAssignNode, const IVertexBoneAssignment *assign );
        void writeTextureAliases( TiXmlElement *mSubmeshesNode, const SmartPtr<SubMesh> s );
        void writeLodInfo( TiXmlElement *mMeshNode, const Mesh *pMesh );
        void writeLodUsageManual( TiXmlElement *usageNode, unsigned short levelNum,
                                  const MeshLodUsage &usage );
        void writeLodUsageGenerated( TiXmlElement *usageNode, unsigned short levelNum,
                                     const MeshLodUsage &usage, const Mesh *pMesh );
        void writeSubMeshNames( TiXmlElement *mMeshNode, const Mesh *m );
        void writePoses( TiXmlElement *meshNode, const Mesh *m );
        void writeAnimations( TiXmlElement *meshNode, const Mesh *m );
        void writeMorphKeyFrames( TiXmlElement *trackNode, const IAnimationVertexTrack *track );
        void writePoseKeyFrames( TiXmlElement *trackNode, const IAnimationVertexTrack *track );
        void writeExtremes( TiXmlElement *mMeshNode, const Mesh *m );

        void readSubMeshes( TiXmlElement *mSubmeshesNode );
        void readGeometry( TiXmlElement *mGeometryNode, IVertexBuffer *pData );
        void readSkeletonLink( TiXmlElement *mSkelNode );
        void readBoneAssignments( TiXmlElement *mBoneAssignmentsNode );
        void readBoneAssignments( TiXmlElement *mBoneAssignmentsNode, SubMesh *sm );
        void readTextureAliases( TiXmlElement *mTextureAliasesNode, SubMesh *sm );
        void readLodInfo( TiXmlElement *lodNode );
        void readLodUsageManual( TiXmlElement *manualNode, unsigned short index );
        void readLodUsageGenerated( TiXmlElement *genNode, unsigned short index );
        void readSubMeshNames( TiXmlElement *mMeshNamesNode, Mesh *sm );
        void readPoses( TiXmlElement *posesNode, Mesh *m );
        void readAnimations( TiXmlElement *mAnimationsNode, Mesh *m );
        void readTracks( TiXmlElement *tracksNode, Mesh *m, IAnimation *anim );
        void readMorphKeyFrames( TiXmlElement *keyframesNode, IAnimationVertexTrack *track,
                                 size_t vertexCount );
        void readPoseKeyFrames( TiXmlElement *keyframesNode, IAnimationVertexTrack *track );
        void readExtremes( TiXmlElement *extremesNode, Mesh *m );
    };
}  // namespace workphone

#endif
