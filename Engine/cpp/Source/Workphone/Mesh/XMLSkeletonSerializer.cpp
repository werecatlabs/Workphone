#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/XMLSkeletonSerializer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Animation/IActorAnimationTrack.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <map>
#include <cstdlib>
#include <tinyxml.h>

namespace workphone
{
    // Helper functions to replace StringConverter functionality
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

        real_Num parseReal( const String &str )
        {
            return static_cast<real_Num>( std::stof( str.c_str() ) );
        }

        int parseInt( const String &str )
        {
            return std::stoi( str.c_str() );
        }

        String toString( real_Num value )
        {
            return std::to_string( value ).c_str();
        }

        String toString( int value )
        {
            return std::to_string( value ).c_str();
        }

        real_Num getRealAttribute( const TiXmlElement *node, const char *name,
                                   real_Num defaultValue = real_Num( 0 ) )
        {
            const char *attr = node ? node->Attribute( name ) : nullptr;
            return attr ? static_cast<real_Num>( std::strtod( attr, nullptr ) ) : defaultValue;
        }

        Vector3<real_Num> readVector3( const TiXmlElement *node, const Vector3<real_Num> &defaultValue )
        {
            if( !node )
            {
                return defaultValue;
            }

            return Vector3<real_Num>( getRealAttribute( node, "x", defaultValue.x ),
                                      getRealAttribute( node, "y", defaultValue.y ),
                                      getRealAttribute( node, "z", defaultValue.z ) );
        }

        Quaternion<real_Num> readRotation( const TiXmlElement *node )
        {
            if( !node )
            {
                return Quaternion<real_Num>::identity();
            }

            if( node->Attribute( "qw" ) || node->Attribute( "qx" ) || node->Attribute( "qy" ) ||
                node->Attribute( "qz" ) )
            {
                return Quaternion<real_Num>(
                    getRealAttribute( node, "qw", real_Num( 1 ) ), getRealAttribute( node, "qx" ),
                    getRealAttribute( node, "qy" ), getRealAttribute( node, "qz" ) );
            }

            if( node->Attribute( "w" ) || node->Attribute( "x" ) || node->Attribute( "y" ) ||
                node->Attribute( "z" ) )
            {
                return Quaternion<real_Num>(
                    getRealAttribute( node, "w", real_Num( 1 ) ), getRealAttribute( node, "x" ),
                    getRealAttribute( node, "y" ), getRealAttribute( node, "z" ) );
            }

            if( node->Attribute( "angle" ) )
            {
                const auto axis = readVector3( node->FirstChildElement( "axis" ),
                                               Vector3<real_Num>( 0.0f, 0.0f, 1.0f ) );
                Quaternion<real_Num> q;
                q.fromAngleAxis( getRealAttribute( node, "angle" ), axis );
                return q;
            }

            return Quaternion<real_Num>::identity();
        }
    }  // namespace

    XMLSkeletonSerializer::XMLSkeletonSerializer() = default;

    XMLSkeletonSerializer::~XMLSkeletonSerializer() = default;

    void XMLSkeletonSerializer::importSkeleton( const String &filename, ISkeleton *pSkeleton )
    {
        if( !pSkeleton || StringUtil::isNullOrEmpty( filename ) )
        {
            return;
        }

        WP_LOG( "XMLSkeletonSerializer: reading XML data from " + filename + "..." );

        TiXmlDocument mXMLDoc;
        if( !mXMLDoc.LoadFile( filename.c_str() ) )
        {
            WP_LOG_ERROR( "XMLSkeletonSerializer failed reading the XML file: " +
                          String( mXMLDoc.ErrorDesc() ) );
            return;
        }

        TiXmlElement *elem = nullptr;

        TiXmlElement *rootElem = mXMLDoc.RootElement();
        if( !rootElem )
        {
            WP_LOG_ERROR( "XMLSkeletonSerializer XML file has no root element." );
            return;
        }

        // Optional blend mode
        const char *blendModeStr = rootElem->Attribute( "blendmode" );
        if( blendModeStr )
        {
            if( String( blendModeStr ) == "cumulative" )
                pSkeleton->setBlendMode( SkeletonAnimationBlendMode::ANIMBLEND_CUMULATIVE );
            else
                pSkeleton->setBlendMode( SkeletonAnimationBlendMode::ANIMBLEND_AVERAGE );
        }

        // Cast to MeshSkeleton for helper methods
        MeshSkeleton *meshSkeleton = static_cast<MeshSkeleton *>( pSkeleton );

        // Bones
        elem = rootElem->FirstChildElement( "bones" );
        if( elem )
        {
            readBones( meshSkeleton, elem );

            auto hierarchyElem = rootElem->FirstChildElement( "bonehierarchy" );
            if( hierarchyElem )
            {
                createHierarchy( meshSkeleton, hierarchyElem );
            }

            readBones2( meshSkeleton, elem );

            auto animationsElem = rootElem->FirstChildElement( "animations" );
            if( animationsElem )
            {
                readAnimations( meshSkeleton, animationsElem );
            }

            auto linksElem = rootElem->FirstChildElement( "animationlinks" );
            if( linksElem )
            {
                readSkeletonAnimationLinks( meshSkeleton, linksElem );
            }
        }

        WP_LOG( "XMLSkeletonSerializer: Finished. Running SkeletonSerializer..." );
    }

    // sets names
    void XMLSkeletonSerializer::readBones( MeshSkeleton *skel, const TiXmlElement *mBonesNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Bones name..." );

        int max_id = -1;

        for( const TiXmlElement *bonElem = mBonesNode ? mBonesNode->FirstChildElement() : nullptr;
             bonElem; bonElem = bonElem->NextSiblingElement() )
        {
            String name = getAttribute( bonElem, "name" );
            int id = parseInt( getAttribute( bonElem, "id" ) );
            skel->createBone( name, static_cast<u32>( id ) );

            max_id = std::max( id, max_id );
        }

        // Note: Removed OgreAssert - add engine-specific assertion if needed
    }

    // set positions and orientations.
    void XMLSkeletonSerializer::readBones2( MeshSkeleton *skel, const TiXmlElement *mBonesNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Bones data..." );

        for( const TiXmlElement *bonElem = mBonesNode ? mBonesNode->FirstChildElement() : nullptr;
             bonElem; bonElem = bonElem->NextSiblingElement() )
        {
            String name = getAttribute( bonElem, "name" );

            const TiXmlElement *posElem = bonElem->FirstChildElement( "position" );
            const TiXmlElement *rotElem = bonElem->FirstChildElement( "rotation" );
            const TiXmlElement *scaleElem = bonElem->FirstChildElement( "scale" );

            Vector3<real_Num> scale;

            auto pos = readVector3( posElem, Vector3<real_Num>::zero() );
            auto orientation = readRotation( rotElem );

            // Optional scale
            if( scaleElem )
            {
                // Uniform scale or per axis?
                const char *factorAttrib = scaleElem->Attribute( "factor" );
                if( factorAttrib )
                {
                    // Uniform scale
                    real_Num factor = parseReal( factorAttrib );
                    scale = Vector3<real_Num>( factor, factor, factor );
                }
                else
                {
                    // axis scale
                    scale = Vector3<real_Num>( 1.0f, 1.0f, 1.0f );  // UNIT_SCALE equivalent
                    const char *factorString = scaleElem->Attribute( "x" );
                    if( factorString )
                    {
                        scale.X() = parseReal( factorString );
                    }
                    factorString = scaleElem->Attribute( "y" );
                    if( factorString )
                    {
                        scale.Y() = parseReal( factorString );
                    }
                    factorString = scaleElem->Attribute( "z" );
                    if( factorString )
                    {
                        scale.Z() = parseReal( factorString );
                    }
                }
            }
            else
            {
                scale = Vector3<real_Num>( 1.0f, 1.0f, 1.0f );  // UNIT_SCALE equivalent
            }

            auto bone = skel->getBone( name );
            if( bone )
            {
                bone->setPosition( pos );
                bone->setOrientation( orientation );
                bone->setBindingPose();

                // Note: Scale setting depends on IBone interface - may need to add this method
                // bone->setScale( scale );
            }
        }  // bones
    }

    void XMLSkeletonSerializer::createHierarchy( MeshSkeleton *skel, const TiXmlElement *mHierNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Hierarchy data..." );

        for( const TiXmlElement *hierElem = mHierNode ? mHierNode->FirstChildElement() : nullptr;
             hierElem; hierElem = hierElem->NextSiblingElement() )
        {
            String boneName = getAttribute( hierElem, "bone" );
            String parentName = getAttribute( hierElem, "parent" );

            auto bone = skel->getBone( boneName );
            auto parent = skel->getBone( parentName );

            if( bone && parent )
            {
                // Note: This depends on IBone having an addChild method or similar hierarchy setup
                // You may need to implement this in your IBone interface
                // parent->addChild( bone );
                WP_LOG( "XMLSkeletonSerializer: Setting up bone hierarchy: " + parentName + " -> " +
                        boneName );
            }
        }
    }

    void XMLSkeletonSerializer::readAnimations( MeshSkeleton *skel, const TiXmlElement *mAnimNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Animations data..." );

        for( const TiXmlElement *animElem = mAnimNode ? mAnimNode->FirstChildElement( "animation" )
                                                      : nullptr;
             animElem; animElem = animElem->NextSiblingElement( "animation" ) )
        {
            String name = getAttribute( animElem, "name" );
            real_Num length = parseReal( getAttribute( animElem, "length" ) );

            auto anim = skel->createAnimation( name, static_cast<f32>( length ) );
            if( !anim )
            {
                WP_LOG( "XMLSkeletonSerializer: Failed to create animation: " + name );
                continue;
            }

            // Note: setInterpolationMode may need to be added to IAnimation interface
            // anim->setInterpolationMode( InterpolationMode::LINEAR );

            const TiXmlElement *baseInfoNode = animElem->FirstChildElement( "baseinfo" );
            if( baseInfoNode )
            {
                String baseName = getAttribute( baseInfoNode, "baseanimationname" );
                real_Num baseTime = parseReal( getAttribute( baseInfoNode, "basekeyframetime" ) );
                // Note: setUseBaseKeyFrame may need to be added to IAnimation interface
                // anim->setUseBaseKeyFrame( true, static_cast<f32>( baseTime ), baseName );
            }

            const TiXmlElement *tracksNode = animElem->FirstChildElement( "tracks" );

            for( const TiXmlElement *trackElem = tracksNode ? tracksNode->FirstChildElement( "track" )
                                                            : nullptr;
                 trackElem; trackElem = trackElem->NextSiblingElement( "track" ) )
            {
                String boneName = getAttribute( trackElem, "bone" );
                auto bone = skel->getBone( boneName );

                if( bone )
                {
                    auto trackBase = anim->addTrack( StringUtil::getHash( "ActorAnimationTrack" ),
                                                     bone->getBoneHandle(), bone );
                    auto track = workphone::dynamic_pointer_cast<IActorAnimationTrack>( trackBase );
                    if( !track )
                    {
                        WP_LOG( "XMLSkeletonSerializer: Failed to create track for bone: " + boneName );
                        continue;
                    }

                    track->setPropertyName( boneName );
                    track->setTrackType( IActorAnimationTrack::TrackType::Transform );
                    readKeyFrames( track.get(), trackElem->FirstChildElement( "keyframes" ) );
                    WP_LOG( "XMLSkeletonSerializer: Processing track for bone: " + boneName );
                }
            }
        }
    }

    void XMLSkeletonSerializer::readKeyFrames( IActorAnimationTrack *track,
                                               const TiXmlElement *mKeyfNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading keyframes..." );

        if( !track || !mKeyfNode )
        {
            return;
        }

        for( const TiXmlElement *keyfElem = mKeyfNode ? mKeyfNode->FirstChildElement( "keyframe" )
                                                      : nullptr;
             keyfElem; keyfElem = keyfElem->NextSiblingElement( "keyframe" ) )
        {
            const auto time = static_cast<f32>( getRealAttribute( keyfElem, "time", real_Num( 0 ) ) );
            auto keyFrame =
                workphone::dynamic_pointer_cast<KeyFrameTransform3>( track->createKeyFrame( time ) );
            if( !keyFrame )
            {
                continue;
            }

            // Optional translate
            const TiXmlElement *transElem = keyfElem->FirstChildElement( "translate" );
            keyFrame->setPosition( readVector3( transElem, Vector3<real_Num>::zero() ) );

            // Optional rotate
            const TiXmlElement *rotElem = keyfElem->FirstChildElement( "rotation" );
            if( !rotElem )
            {
                rotElem = keyfElem->FirstChildElement( "rotate" );
            }

            if( rotElem )
            {
                keyFrame->setOrientation( readRotation( rotElem ) );
            }
            else
            {
                keyFrame->setOrientation( Quaternion<real_Num>::identity() );
            }

            // Optional scale
            const TiXmlElement *scaleElem = keyfElem->FirstChildElement( "scale" );
            keyFrame->setScale( readVector3( scaleElem, Vector3<real_Num>::unit() ) );
        }
    }

    void XMLSkeletonSerializer::exportSkeleton( const ISkeleton *pSkeleton, const String &filename )
    {
        if( !pSkeleton || StringUtil::isNullOrEmpty( filename ) )
        {
            return;
        }

        auto folder = Path::getFilePath( filename );
        if( !StringUtil::isNullOrEmpty( folder ) && !Path::isExistingFolder( folder ) )
        {
            Path::createDirectories( folder );
        }

        TiXmlDocument doc;
        auto rootNode = appendElement( &doc, "skeleton" );
        writeSkeleton( pSkeleton, rootNode );

        if( !doc.SaveFile( filename.c_str() ) )
        {
            WP_LOG_ERROR( "XMLSkeletonSerializer failed writing the XML file: " + filename );
            return;
        }

        WP_LOG( "XMLSkeletonSerializer export successful: " + filename );
    }

    void XMLSkeletonSerializer::writeSkeleton( const ISkeleton *pSkel, TiXmlElement *rootNode )
    {
        auto meshSkeleton = dynamic_cast<const MeshSkeleton *>( pSkel );
        if( !meshSkeleton )
        {
            return;
        }

        rootNode->SetAttribute( "blendmode",
                                pSkel->getBlendMode() == SkeletonAnimationBlendMode::ANIMBLEND_CUMULATIVE
                                    ? "cumulative"
                                    : "average" );

        auto bonesNode = appendElement( rootNode, "bones" );
        for( const auto &bone : meshSkeleton->getBones() )
        {
            if( bone )
            {
                writeBone( bonesNode, bone.get() );
            }
        }

        auto hierarchyNode = appendElement( rootNode, "bonehierarchy" );
        for( const auto &bone : meshSkeleton->getBones() )
        {
            if( bone )
            {
                auto parent = bone->getParent();
                if( parent )
                {
                    writeBoneParent( hierarchyNode, bone->getName(), parent->getName() );
                }
            }
        }

        auto animationsNode = appendElement( rootNode, "animations" );
        for( const auto &animation : meshSkeleton->getAnimations() )
        {
            if( animation )
            {
                writeAnimation( animationsNode, animation.get() );
            }
        }
    }

    void XMLSkeletonSerializer::writeBone( TiXmlElement *bonesElement, const IBone *pBone )
    {
        if( !pBone )
        {
            return;
        }

        auto boneNode = appendElement( bonesElement, "bone" );
        boneNode->SetAttribute(
            "id", std::to_string( static_cast<unsigned int>( pBone->getBoneHandle() ) ).c_str() );
        boneNode->SetAttribute( "name", pBone->getName().c_str() );

        auto position = pBone->getPosition();
        auto positionNode = appendElement( boneNode, "position" );
        positionNode->SetDoubleAttribute( "x", position.x );
        positionNode->SetDoubleAttribute( "y", position.y );
        positionNode->SetDoubleAttribute( "z", position.z );

        auto orientation = pBone->getOrientation();
        auto rotationNode = appendElement( boneNode, "rotation" );
        rotationNode->SetDoubleAttribute( "qw", orientation.w );
        rotationNode->SetDoubleAttribute( "qx", orientation.x );
        rotationNode->SetDoubleAttribute( "qy", orientation.y );
        rotationNode->SetDoubleAttribute( "qz", orientation.z );
    }

    void XMLSkeletonSerializer::writeBoneParent( TiXmlElement *boneHierarchyNode, String boneName,
                                                 String parentName )
    {
        TiXmlElement *boneParentNode = appendElement( boneHierarchyNode, "boneparent" );
        boneParentNode->SetAttribute( "bone", boneName.c_str() );
        boneParentNode->SetAttribute( "parent", parentName.c_str() );
    }

    void XMLSkeletonSerializer::writeAnimation( TiXmlElement *animsNode, const IAnimation *anim )
    {
        if( !anim )
        {
            return;
        }

        auto animationNode = appendElement( animsNode, "animation" );
        animationNode->SetAttribute( "name", anim->getName().c_str() );
        animationNode->SetDoubleAttribute( "length", anim->getLength() );

        auto tracksNode = appendElement( animationNode, "tracks" );
        const auto &nodeTracks = anim->_getNodeTrackList();
        for( const auto &trackPair : nodeTracks )
        {
            if( trackPair.second )
            {
                writeAnimationTrack( tracksNode, trackPair.second );
            }
        }
    }

    void XMLSkeletonSerializer::writeAnimationTrack( TiXmlElement *tracksNode,
                                                     const IActorAnimationTrack *track )
    {
        if( !track )
        {
            return;
        }

        auto trackNode = appendElement( tracksNode, "track" );
        trackNode->SetAttribute( "bone", track->getPropertyName().c_str() );
        trackNode->SetAttribute(
            "handle",
            std::to_string( static_cast<unsigned int>( track->getAnimationHandle() ) ).c_str() );

        auto keyFramesNode = appendElement( trackNode, "keyframes" );
        for( u16 i = 0; i < track->getNumKeyFrames(); ++i )
        {
            auto keyFrame =
                workphone::dynamic_pointer_cast<KeyFrameTransform3>( track->getKeyFrame( i ) );
            if( keyFrame )
            {
                writeKeyFrame( keyFramesNode, keyFrame.get() );
            }
        }
    }

    void XMLSkeletonSerializer::writeKeyFrame( TiXmlElement *keysNode, const KeyFrameTransform3 *key )
    {
        if( !key )
        {
            return;
        }

        auto keyNode = appendElement( keysNode, "keyframe" );
        keyNode->SetDoubleAttribute( "time", key->getTime() );

        auto position = key->getPosition();
        auto translateNode = appendElement( keyNode, "translate" );
        translateNode->SetDoubleAttribute( "x", position.x );
        translateNode->SetDoubleAttribute( "y", position.y );
        translateNode->SetDoubleAttribute( "z", position.z );

        auto orientation = key->getOrientation();
        auto rotationNode = appendElement( keyNode, "rotation" );
        rotationNode->SetDoubleAttribute( "qw", orientation.w );
        rotationNode->SetDoubleAttribute( "qx", orientation.x );
        rotationNode->SetDoubleAttribute( "qy", orientation.y );
        rotationNode->SetDoubleAttribute( "qz", orientation.z );

        auto scale = key->getScale();
        auto scaleNode = appendElement( keyNode, "scale" );
        scaleNode->SetDoubleAttribute( "x", scale.x );
        scaleNode->SetDoubleAttribute( "y", scale.y );
        scaleNode->SetDoubleAttribute( "z", scale.z );
    }

    void XMLSkeletonSerializer::writeSkeletonAnimationLink( TiXmlElement *linksNode,
                                                            const LinkedSkeletonAnimationSource &link )
    {
        // Note: Write functionality can be implemented later if needed
    }

    void XMLSkeletonSerializer::readSkeletonAnimationLinks( MeshSkeleton *skel,
                                                            const TiXmlElement *linksNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Animation links..." );

        for( const TiXmlElement *linkElem = linksNode ? linksNode->FirstChildElement( "animationlink" )
                                                      : nullptr;
             linkElem; linkElem = linkElem->NextSiblingElement( "animationlink" ) )
        {
            String skelName = getAttribute( linkElem, "skeletonName" );
            const char *strScale = linkElem->Attribute( "scale" );
            real_Num scale;
            // Scale optional
            if( strScale == 0 )
            {
                scale = 1.0f;
            }
            else
            {
                scale = parseReal( strScale );
            }

            // Note: addLinkedSkeletonAnimationSource may need to be added to MeshSkeleton
            // skel->addLinkedSkeletonAnimationSource( skelName, static_cast<f32>( scale ) );
            WP_LOG( "XMLSkeletonSerializer: Found animation link to skeleton: " + skelName );
        }
    }

}  // namespace workphone
