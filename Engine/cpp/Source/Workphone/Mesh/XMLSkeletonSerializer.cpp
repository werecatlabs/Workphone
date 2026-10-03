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

namespace workphone
{
    // Helper functions to replace StringConverter functionality
    namespace
    {
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

        real_Num getRealAttribute( const pugi::xml_node &node, const char *name,
                                   real_Num defaultValue = real_Num( 0 ) )
        {
            auto attr = node.attribute( name );
            return attr ? static_cast<real_Num>( attr.as_double() ) : defaultValue;
        }

        Vector3<real_Num> readVector3( const pugi::xml_node &node,
                                       const Vector3<real_Num> &defaultValue )
        {
            if( !node )
            {
                return defaultValue;
            }

            return Vector3<real_Num>( getRealAttribute( node, "x", defaultValue.x ),
                                      getRealAttribute( node, "y", defaultValue.y ),
                                      getRealAttribute( node, "z", defaultValue.z ) );
        }

        Quaternion<real_Num> readRotation( const pugi::xml_node &node )
        {
            if( !node )
            {
                return Quaternion<real_Num>::identity();
            }

            if( node.attribute( "qw" ) || node.attribute( "qx" ) || node.attribute( "qy" ) ||
                node.attribute( "qz" ) )
            {
                return Quaternion<real_Num>(
                    getRealAttribute( node, "qw", real_Num( 1 ) ), getRealAttribute( node, "qx" ),
                    getRealAttribute( node, "qy" ), getRealAttribute( node, "qz" ) );
            }

            if( node.attribute( "w" ) || node.attribute( "x" ) || node.attribute( "y" ) ||
                node.attribute( "z" ) )
            {
                return Quaternion<real_Num>(
                    getRealAttribute( node, "w", real_Num( 1 ) ), getRealAttribute( node, "x" ),
                    getRealAttribute( node, "y" ), getRealAttribute( node, "z" ) );
            }

            if( node.attribute( "angle" ) )
            {
                const auto axis =
                    readVector3( node.child( "axis" ), Vector3<real_Num>( 0.0f, 0.0f, 1.0f ) );
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
        WP_LOG( "XMLSkeletonSerializer: reading XML data from " + filename + "..." );

        pugi::xml_document mXMLDoc;
        mXMLDoc.load_file( filename.c_str() );

        pugi::xml_node elem;

        pugi::xml_node rootElem = mXMLDoc.document_element();

        // Optional blend mode
        const char *blendModeStr = rootElem.attribute( "blendmode" ).as_string( NULL );
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
        elem = rootElem.child( "bones" );
        if( elem )
        {
            readBones( meshSkeleton, elem );

            auto hierarchyElem = rootElem.child( "bonehierarchy" );
            if( hierarchyElem )
            {
                createHierarchy( meshSkeleton, hierarchyElem );
            }

            readBones2( meshSkeleton, elem );

            auto animationsElem = rootElem.child( "animations" );
            if( animationsElem )
            {
                readAnimations( meshSkeleton, animationsElem );
            }

            auto linksElem = rootElem.child( "animationlinks" );
            if( linksElem )
            {
                readSkeletonAnimationLinks( meshSkeleton, linksElem );
            }
        }

        WP_LOG( "XMLSkeletonSerializer: Finished. Running SkeletonSerializer..." );
    }

    // sets names
    void XMLSkeletonSerializer::readBones( MeshSkeleton *skel, pugi::xml_node &mBonesNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Bones name..." );

        int max_id = -1;

        for( pugi::xml_node &bonElem : mBonesNode.children() )
        {
            String name = bonElem.attribute( "name" ).value();
            int id = parseInt( bonElem.attribute( "id" ).value() );
            skel->createBone( name, static_cast<u32>( id ) );

            max_id = std::max( id, max_id );
        }

        // Note: Removed OgreAssert - add engine-specific assertion if needed
    }

    // set positions and orientations.
    void XMLSkeletonSerializer::readBones2( MeshSkeleton *skel, pugi::xml_node &mBonesNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Bones data..." );

        for( pugi::xml_node &bonElem : mBonesNode.children() )
        {
            String name = bonElem.attribute( "name" ).value();

            pugi::xml_node posElem = bonElem.child( "position" );
            pugi::xml_node rotElem = bonElem.child( "rotation" );
            pugi::xml_node scaleElem = bonElem.child( "scale" );

            Vector3<real_Num> scale;

            auto pos = readVector3( posElem, Vector3<real_Num>::zero() );
            auto orientation = readRotation( rotElem );

            // Optional scale
            if( scaleElem )
            {
                // Uniform scale or per axis?
                const char *factorAttrib = scaleElem.attribute( "factor" ).as_string( NULL );
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
                    const char *factorString = scaleElem.attribute( "x" ).as_string( NULL );
                    if( factorString )
                    {
                        scale.X() = parseReal( factorString );
                    }
                    factorString = scaleElem.attribute( "y" ).value();
                    if( factorString )
                    {
                        scale.Y() = parseReal( factorString );
                    }
                    factorString = scaleElem.attribute( "z" ).value();
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

    void XMLSkeletonSerializer::createHierarchy( MeshSkeleton *skel, pugi::xml_node &mHierNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Hierarchy data..." );

        for( pugi::xml_node &hierElem : mHierNode.children() )
        {
            String boneName = hierElem.attribute( "bone" ).value();
            String parentName = hierElem.attribute( "parent" ).value();

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

    void XMLSkeletonSerializer::readAnimations( MeshSkeleton *skel, pugi::xml_node &mAnimNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Animations data..." );

        for( pugi::xml_node &animElem : mAnimNode.children( "animation" ) )
        {
            String name = animElem.attribute( "name" ).value();
            real_Num length = parseReal( animElem.attribute( "length" ).value() );

            auto anim = skel->createAnimation( name, static_cast<f32>( length ) );
            if( !anim )
            {
                WP_LOG( "XMLSkeletonSerializer: Failed to create animation: " + name );
                continue;
            }

            // Note: setInterpolationMode may need to be added to IAnimation interface
            // anim->setInterpolationMode( InterpolationMode::LINEAR );

            pugi::xml_node baseInfoNode = animElem.child( "baseinfo" );
            if( baseInfoNode )
            {
                String baseName = baseInfoNode.attribute( "baseanimationname" ).value();
                real_Num baseTime = parseReal( baseInfoNode.attribute( "basekeyframetime" ).value() );
                // Note: setUseBaseKeyFrame may need to be added to IAnimation interface
                // anim->setUseBaseKeyFrame( true, static_cast<f32>( baseTime ), baseName );
            }

            pugi::xml_node tracksNode = animElem.child( "tracks" );

            for( pugi::xml_node &trackElem : tracksNode.children( "track" ) )
            {
                String boneName = trackElem.attribute( "bone" ).value();
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
                    readKeyFrames( track.get(), trackElem.child( "keyframes" ) );
                    WP_LOG( "XMLSkeletonSerializer: Processing track for bone: " + boneName );
                }
            }
        }
    }

    void XMLSkeletonSerializer::readKeyFrames( IActorAnimationTrack *track,
                                               const pugi::xml_node &mKeyfNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading keyframes..." );

        if( !track )
        {
            return;
        }

        for( pugi::xml_node &keyfElem : mKeyfNode.children( "keyframe" ) )
        {
            const auto time = static_cast<f32>( getRealAttribute( keyfElem, "time", real_Num( 0 ) ) );
            auto keyFrame =
                workphone::dynamic_pointer_cast<KeyFrameTransform3>( track->createKeyFrame( time ) );
            if( !keyFrame )
            {
                continue;
            }

            // Optional translate
            pugi::xml_node transElem = keyfElem.child( "translate" );
            keyFrame->setPosition( readVector3( transElem, Vector3<real_Num>::zero() ) );

            // Optional rotate
            pugi::xml_node rotElem = keyfElem.child( "rotation" );
            if( !rotElem )
            {
                rotElem = keyfElem.child( "rotate" );
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
            pugi::xml_node scaleElem = keyfElem.child( "scale" );
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

        pugi::xml_document doc;
        auto rootNode = doc.append_child( "skeleton" );
        writeSkeleton( pSkeleton, rootNode );

        if( !doc.save_file( filename.c_str(), "    " ) )
        {
            WP_LOG_ERROR( "XMLSkeletonSerializer failed writing the XML file: " + filename );
            return;
        }

        WP_LOG( "XMLSkeletonSerializer export successful: " + filename );
    }

    void XMLSkeletonSerializer::writeSkeleton( const ISkeleton *pSkel, pugi::xml_node &rootNode )
    {
        auto meshSkeleton = dynamic_cast<const MeshSkeleton *>( pSkel );
        if( !meshSkeleton )
        {
            return;
        }

        rootNode.append_attribute( "blendmode" ) =
            pSkel->getBlendMode() == SkeletonAnimationBlendMode::ANIMBLEND_CUMULATIVE ? "cumulative"
                                                                                      : "average";

        auto bonesNode = rootNode.append_child( "bones" );
        for( const auto &bone : meshSkeleton->getBones() )
        {
            if( bone )
            {
                writeBone( bonesNode, bone.get() );
            }
        }

        auto hierarchyNode = rootNode.append_child( "bonehierarchy" );
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

        auto animationsNode = rootNode.append_child( "animations" );
        for( const auto &animation : meshSkeleton->getAnimations() )
        {
            if( animation )
            {
                writeAnimation( animationsNode, animation.get() );
            }
        }
    }

    void XMLSkeletonSerializer::writeBone( pugi::xml_node &bonesElement, const IBone *pBone )
    {
        if( !pBone )
        {
            return;
        }

        auto boneNode = bonesElement.append_child( "bone" );
        boneNode.append_attribute( "id" ) = static_cast<unsigned int>( pBone->getBoneHandle() );
        boneNode.append_attribute( "name" ) = pBone->getName().c_str();

        auto position = pBone->getPosition();
        auto positionNode = boneNode.append_child( "position" );
        positionNode.append_attribute( "x" ) = position.x;
        positionNode.append_attribute( "y" ) = position.y;
        positionNode.append_attribute( "z" ) = position.z;

        auto orientation = pBone->getOrientation();
        auto rotationNode = boneNode.append_child( "rotation" );
        rotationNode.append_attribute( "qw" ) = orientation.w;
        rotationNode.append_attribute( "qx" ) = orientation.x;
        rotationNode.append_attribute( "qy" ) = orientation.y;
        rotationNode.append_attribute( "qz" ) = orientation.z;
    }

    void XMLSkeletonSerializer::writeBoneParent( pugi::xml_node &boneHierarchyNode, String boneName,
                                                 String parentName )
    {
        pugi::xml_node boneParentNode = boneHierarchyNode.append_child( "boneparent" );
        boneParentNode.append_attribute( "bone" ) = boneName.c_str();
        boneParentNode.append_attribute( "parent" ) = parentName.c_str();
    }

    void XMLSkeletonSerializer::writeAnimation( pugi::xml_node &animsNode, const IAnimation *anim )
    {
        if( !anim )
        {
            return;
        }

        auto animationNode = animsNode.append_child( "animation" );
        animationNode.append_attribute( "name" ) = anim->getName().c_str();
        animationNode.append_attribute( "length" ) = anim->getLength();

        auto tracksNode = animationNode.append_child( "tracks" );
        const auto &nodeTracks = anim->_getNodeTrackList();
        for( const auto &trackPair : nodeTracks )
        {
            if( trackPair.second )
            {
                writeAnimationTrack( tracksNode, trackPair.second );
            }
        }
    }

    void XMLSkeletonSerializer::writeAnimationTrack( pugi::xml_node &tracksNode,
                                                     const IActorAnimationTrack *track )
    {
        if( !track )
        {
            return;
        }

        auto trackNode = tracksNode.append_child( "track" );
        trackNode.append_attribute( "bone" ) = track->getPropertyName().c_str();
        trackNode.append_attribute( "handle" ) =
            static_cast<unsigned int>( track->getAnimationHandle() );

        auto keyFramesNode = trackNode.append_child( "keyframes" );
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

    void XMLSkeletonSerializer::writeKeyFrame( pugi::xml_node &keysNode, const KeyFrameTransform3 *key )
    {
        if( !key )
        {
            return;
        }

        auto keyNode = keysNode.append_child( "keyframe" );
        keyNode.append_attribute( "time" ) = key->getTime();

        auto position = key->getPosition();
        auto translateNode = keyNode.append_child( "translate" );
        translateNode.append_attribute( "x" ) = position.x;
        translateNode.append_attribute( "y" ) = position.y;
        translateNode.append_attribute( "z" ) = position.z;

        auto orientation = key->getOrientation();
        auto rotationNode = keyNode.append_child( "rotation" );
        rotationNode.append_attribute( "qw" ) = orientation.w;
        rotationNode.append_attribute( "qx" ) = orientation.x;
        rotationNode.append_attribute( "qy" ) = orientation.y;
        rotationNode.append_attribute( "qz" ) = orientation.z;

        auto scale = key->getScale();
        auto scaleNode = keyNode.append_child( "scale" );
        scaleNode.append_attribute( "x" ) = scale.x;
        scaleNode.append_attribute( "y" ) = scale.y;
        scaleNode.append_attribute( "z" ) = scale.z;
    }

    void XMLSkeletonSerializer::writeSkeletonAnimationLink( pugi::xml_node &linksNode,
                                                            const LinkedSkeletonAnimationSource &link )
    {
        // Note: Write functionality can be implemented later if needed
    }

    void XMLSkeletonSerializer::readSkeletonAnimationLinks( MeshSkeleton *skel,
                                                            pugi::xml_node &linksNode )
    {
        WP_LOG( "XMLSkeletonSerializer: Reading Animation links..." );

        for( pugi::xml_node &linkElem : linksNode.children( "animationlink" ) )
        {
            String skelName = linkElem.attribute( "skeletonName" ).value();
            const char *strScale = linkElem.attribute( "scale" ).as_string( NULL );
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
