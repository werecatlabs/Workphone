#ifndef __XMLSkeletonSerializer_H__
#define __XMLSkeletonSerializer_H__

#include <pugixml.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>

namespace workphone
{

    /** Class for serializing a Skeleton to/from XML.
    @remarks
        This class behaves the same way as SkeletonSerializer in the main project,
        but is here to allow conversions to / from XML. This class is
        deliberately not included in the main project because <UL>
        <LI>Dependence on Xerces would unnecessarily bloat the main library</LI>
        <LI>Runtime use of XML is discouraged because of the parsing overhead</LI></UL>
        This class gives people the option of saving out a Skeleton as XML for examination
        and possible editing. It can then be converted back to the native format
        for maximum runtime efficiency.
    */
    class WPCore_API XMLSkeletonSerializer
    {
    public:
        XMLSkeletonSerializer();
        virtual ~XMLSkeletonSerializer();

        /** Imports a Skeleton from the given XML file.
        @param filename The name of the file to import, expected to be in XML format.
        @param pSkeleton The pre-created Skeleton object to be populated.
        */
        void importSkeleton( const String &filename, ISkeleton *pSkeleton );

        /** Exports a skeleton to the named XML file. */
        void exportSkeleton( const ISkeleton *pSkeleton, const String &filename );

    private:
        void writeSkeleton( const ISkeleton *pSkel, pugi::xml_node &root );
        void writeBone( pugi::xml_node &bonesElement, const IBone *pBone );
        void writeBoneParent( pugi::xml_node &boneHierarchyNode, String boneName, String parentName );
        void writeAnimation( pugi::xml_node &animsNode, const IAnimation *anim );
        void writeAnimationTrack( pugi::xml_node &tracksNode, const IActorAnimationTrack *track );
        void writeKeyFrame( pugi::xml_node &keysNode, const KeyFrameTransform3 *key );
        void writeSkeletonAnimationLink( pugi::xml_node &linksNode,
                                         const LinkedSkeletonAnimationSource &link );

        void readBones( MeshSkeleton *skel, pugi::xml_node &mBonesNode );
        void readBones2( MeshSkeleton *skel, pugi::xml_node &mBonesNode );
        void createHierarchy( MeshSkeleton *skel, pugi::xml_node &mHierNode );
        void readKeyFrames( IActorAnimationTrack *track, const pugi::xml_node &mKeyfNode );
        void readAnimations( MeshSkeleton *skel, pugi::xml_node &mAnimNode );
        void readSkeletonAnimationLinks( MeshSkeleton *skel, pugi::xml_node &linksNode );
    };

}  // namespace workphone

#endif
