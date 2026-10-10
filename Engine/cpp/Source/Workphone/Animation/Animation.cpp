#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Interface/Animation/IAnimationTrack.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Animation/ActorAnimationTrack.hpp>
#include <Workphone/Animation/AnimationVertexTrack.hpp>
#include <Workphone/Animation/AnimationNumericTrack.hpp>
#include <Workphone/Animation/AnimationTimeIndex.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Animation, IAnimation );

    namespace
    {
        template <class T>
        void eraseOwnedTrack( Array<SmartPtr<T>> &tracks, T *track )
        {
            tracks.erase( std::remove_if( tracks.begin(), tracks.end(),
                                          [track]( const SmartPtr<T> &ownedTrack ) {
                                              return ownedTrack.get() == track;
                                          } ),
                          tracks.end() );
        }

        template <class T>
        void eraseOwnedTrackByShared( Array<SmartPtr<T>> &tracks,
                                      const SmartPtr<IAnimationTrack> &track )
        {
            tracks.erase( std::remove_if( tracks.begin(), tracks.end(),
                                          [&track]( const SmartPtr<T> &ownedTrack ) {
                                              return ownedTrack && track &&
                                                     static_cast<IAnimationTrack *>(
                                                         ownedTrack.get() ) == track.get();
                                          } ),
                          tracks.end() );
        }

        f32 normaliseTime( f32 timePos, f32 length )
        {
            if( length <= std::numeric_limits<f32>::epsilon() )
            {
                return std::max( 0.0f, timePos );
            }

            return std::clamp( timePos, 0.0f, length );
        }
    }  // namespace

    Animation::~Animation()
    {
        destroyAllTracks();
    }

    Animation::Animation() :
        m_interpolationMode( InterpolationMode::LINEAR ),
        m_rotationInterpolationMode( RotationInterpolationMode::LINEAR ),
        m_length( 0.0f ),
        m_useBaseKeyFrame( false ),
        m_baseKeyFrameTime( 0.0f ),
        m_container( nullptr )
    {
    }

    void Animation::setInterpolationMode( InterpolationMode im )
    {
        m_interpolationMode = im;
    }

    InterpolationMode Animation::getInterpolationMode() const
    {
        return m_interpolationMode;
    }

    void Animation::setRotationInterpolationMode( RotationInterpolationMode im )
    {
        m_rotationInterpolationMode = im;
    }

    RotationInterpolationMode Animation::getRotationInterpolationMode() const
    {
        return m_rotationInterpolationMode;
    }

    void Animation::removeTrack( SmartPtr<IAnimationTrack> track )
    {
        if( !track )
            return;

        // Try to find and remove from node tracks
        for( auto it = m_nodeTracks.begin(); it != m_nodeTracks.end(); ++it )
        {
            if( it->second == track.get() )
            {
                eraseOwnedTrackByShared( m_ownedNodeTracks, track );
                m_nodeTracks.erase( it );
                return;
            }
        }

        // Try to find and remove from vertex tracks
        for( auto it = m_vertexTracks.begin(); it != m_vertexTracks.end(); ++it )
        {
            if( it->second == track.get() )
            {
                eraseOwnedTrackByShared( m_ownedVertexTracks, track );
                m_vertexTracks.erase( it );
                return;
            }
        }
    }

    SmartPtr<IAnimationTrack> Animation::addTrack( hash_type type, const String &name, u16 handle )
    {
        const auto actorTrackType = ActorAnimationTrack::typeInfo();
        const auto numericTrackType = AnimationNumericTrack::typeInfo();
        const auto vertexTrackType = AnimationVertexTrack::typeInfo();
        const auto actorTrackNameType = StringUtil::getHash( "ActorAnimationTrack" );
        const auto numericTrackNameType = StringUtil::getHash( "AnimationNumericTrack" );
        const auto vertexTrackNameType = StringUtil::getHash( "AnimationVertexTrack" );

        if( type == actorTrackType || type == actorTrackNameType )
        {
            auto actorTrack = workphone::make_ptr<ActorAnimationTrack>( this );
            actorTrack->setAnimationHandle( handle );
            actorTrack->setPropertyName( name );
            m_nodeTracks[handle] = actorTrack.get();
            m_ownedNodeTracks.push_back( actorTrack );
            return workphone::static_pointer_cast<IAnimationTrack>( actorTrack );
        }

        if( type == numericTrackType || type == numericTrackNameType )
        {
            auto numericTrack = workphone::make_ptr<AnimationNumericTrack>();
            m_numericTracks[handle] = numericTrack.get();
            m_ownedNumericTracks.push_back( numericTrack );
            return nullptr;
        }

        if( type == vertexTrackType || type == vertexTrackNameType )
        {
            auto vertexTrack =
                workphone::make_ptr<AnimationVertexTrack>( VertexAnimationType::VAT_NONE, this );
            vertexTrack->setAnimationHandle( handle );
            m_vertexTracks[handle] = vertexTrack.get();
            m_ownedVertexTracks.push_back( vertexTrack );
            return workphone::static_pointer_cast<IAnimationTrack>( vertexTrack );
        }

        return nullptr;
    }

    SmartPtr<IAnimationTrack> Animation::addTrack( hash_type type, u16 handle, SmartPtr<IBone> bone )
    {
        const auto actorTrackType = ActorAnimationTrack::typeInfo();
        const auto actorTrackNameType = StringUtil::getHash( "ActorAnimationTrack" );

        if( type == actorTrackType || type == actorTrackNameType )
        {
            auto actorTrack = workphone::make_ptr<ActorAnimationTrack>( this );
            actorTrack->setAnimationHandle( handle );
            actorTrack->setBone( bone );
            m_nodeTracks[handle] = actorTrack.get();
            m_ownedNodeTracks.push_back( actorTrack );
            return workphone::static_pointer_cast<IAnimationTrack>( actorTrack );
        }

        return nullptr;
    }

    f32 Animation::getLength() const
    {
        return m_length;
    }

    void Animation::setLength( f32 length )
    {
        m_length = std::max( 0.0f, length );
    }

    unsigned short Animation::getNumNodeTracks( void ) const
    {
        return static_cast<unsigned short>( m_nodeTracks.size() );
    }

    IActorAnimationTrack *Animation::getNodeTrack( unsigned short handle ) const
    {
        auto it = m_nodeTracks.find( handle );
        return ( it != m_nodeTracks.end() ) ? it->second : nullptr;
    }

    bool Animation::hasNodeTrack( unsigned short handle ) const
    {
        return m_nodeTracks.find( handle ) != m_nodeTracks.end();
    }

    unsigned short Animation::getNumNumericTracks( void ) const
    {
        return static_cast<unsigned short>( m_numericTracks.size() );
    }

    IAnimationNumericTrack *Animation::getNumericTrack( unsigned short handle ) const
    {
        auto it = m_numericTracks.find( handle );
        return ( it != m_numericTracks.end() ) ? it->second : nullptr;
    }

    bool Animation::hasNumericTrack( unsigned short handle ) const
    {
        return m_numericTracks.find( handle ) != m_numericTracks.end();
    }

    unsigned short Animation::getNumVertexTracks( void ) const
    {
        return static_cast<unsigned short>( m_vertexTracks.size() );
    }

    IAnimationVertexTrack *Animation::getVertexTrack( unsigned short handle ) const
    {
        auto it = m_vertexTracks.find( handle );
        return ( it != m_vertexTracks.end() ) ? it->second : nullptr;
    }

    bool Animation::hasVertexTrack( unsigned short handle ) const
    {
        return m_vertexTracks.find( handle ) != m_vertexTracks.end();
    }

    void Animation::destroyNodeTrack( unsigned short handle )
    {
        auto it = m_nodeTracks.find( handle );
        if( it != m_nodeTracks.end() )
        {
            eraseOwnedTrack( m_ownedNodeTracks, it->second );
            m_nodeTracks.erase( it );
        }
    }

    void Animation::destroyNumericTrack( unsigned short handle )
    {
        auto it = m_numericTracks.find( handle );
        if( it != m_numericTracks.end() )
        {
            eraseOwnedTrack( m_ownedNumericTracks, it->second );
            m_numericTracks.erase( it );
        }
    }

    void Animation::destroyVertexTrack( unsigned short handle )
    {
        auto it = m_vertexTracks.find( handle );
        if( it != m_vertexTracks.end() )
        {
            eraseOwnedTrack( m_ownedVertexTracks, it->second );
            m_vertexTracks.erase( it );
        }
    }

    void Animation::destroyAllTracks( void )
    {
        destroyAllNodeTracks();
        destroyAllNumericTracks();
        destroyAllVertexTracks();
    }

    void Animation::destroyAllNodeTracks( void )
    {
        m_nodeTracks.clear();
        m_ownedNodeTracks.clear();
    }

    void Animation::destroyAllNumericTracks( void )
    {
        m_numericTracks.clear();
        m_ownedNumericTracks.clear();
    }

    void Animation::destroyAllVertexTracks( void )
    {
        m_vertexTracks.clear();
        m_ownedVertexTracks.clear();
    }

    void Animation::apply( f32 timePos, f32 weight /*= 1.0*/, f32 scale /*= 1.0f */ )
    {
        if( weight <= 0.0f )
        {
            return;
        }

        auto timeIndex = _getTimeIndex( timePos );

        for( const auto &pair : m_nodeTracks )
        {
            if( pair.second )
            {
                pair.second->apply( timeIndex, weight, scale );
            }
        }

        for( const auto &pair : m_vertexTracks )
        {
            if( pair.second )
            {
                pair.second->apply( timeIndex, weight, scale );
            }
        }
    }

    void Animation::applyToAnimable( const IAnimableValue *anim, f32 timePos, f32 weight /*= 1.0*/,
                                     f32 scale /*= 1.0f */ )
    {
        (void)anim;
        (void)timePos;
        (void)weight;
        (void)scale;
    }

    const IAnimation::NodeTrackList &Animation::_getNodeTrackList( void ) const
    {
        return m_nodeTracks;
    }

    const IAnimation::NumericTrackList &Animation::_getNumericTrackList( void ) const
    {
        return m_numericTracks;
    }

    const IAnimation::VertexTrackList &Animation::_getVertexTrackList( void ) const
    {
        return m_vertexTracks;
    }

    void Animation::optimise( bool discardIdentityNodeTracks /*= true */ )
    {
        for( auto &track : m_ownedNodeTracks )
        {
            if( track )
            {
                track->optimise();
            }
        }

        for( auto &track : m_ownedVertexTracks )
        {
            if( track )
            {
                track->optimise();
            }
        }

        if( discardIdentityNodeTracks )
        {
            TrackHandleList tracksToDestroy;
            _collectIdentityNodeTracks( tracksToDestroy );
            _destroyNodeTracks( tracksToDestroy );
        }
    }

    void Animation::_collectIdentityNodeTracks( TrackHandleList &tracks ) const
    {
        for( const auto &pair : m_nodeTracks )
        {
            if( pair.second && !pair.second->hasNonZeroKeyFrames() )
            {
                tracks.insert( pair.first );
            }
        }
    }

    void Animation::_destroyNodeTracks( const TrackHandleList &tracks )
    {
        for( u16 handle : tracks )
        {
            destroyNodeTrack( handle );
        }
    }

    IAnimation *Animation::clone( const String &newName ) const
    {
        (void)newName;

        auto clonedAnimation = new Animation();
        clonedAnimation->setLength( m_length );
        clonedAnimation->setInterpolationMode( m_interpolationMode );
        clonedAnimation->setRotationInterpolationMode( m_rotationInterpolationMode );
        clonedAnimation->setUseBaseKeyFrame( m_useBaseKeyFrame, m_baseKeyFrameTime,
                                             m_baseKeyFrameAnimationName );
        clonedAnimation->setNodeTracks( getNodeTracks() );
        clonedAnimation->setNumericTracks( getNumericTracks() );
        clonedAnimation->setVertexTracks( getVertexTracks() );

        return clonedAnimation;
    }

    void Animation::_keyFrameListChanged( void )
    {
    }

    SmartPtr<IAnimationTimeIndex> Animation::_getTimeIndex( f32 timePos ) const
    {
        // Tracks have independent key grids. A merged clip-wide index is not a
        // valid index into a sparse track (and gathering it allocates every tick).
        // Each track resolves the common time against its own sorted key times.
        return workphone::make_ptr<AnimationTimeIndex>( normaliseTime( timePos, m_length ) );
    }

    void Animation::setUseBaseKeyFrame( bool useBaseKeyFrame, f32 keyframeTime /*= 0.0f*/,
                                        const String &baseAnimName /*= StringUtil::EmptyString */ )
    {
        m_useBaseKeyFrame = useBaseKeyFrame;
        m_baseKeyFrameTime = normaliseTime( keyframeTime, m_length );
        m_baseKeyFrameAnimationName = baseAnimName;
    }

    bool Animation::getUseBaseKeyFrame() const
    {
        return m_useBaseKeyFrame;
    }

    f32 Animation::getBaseKeyFrameTime() const
    {
        return m_baseKeyFrameTime;
    }

    const String &Animation::getBaseKeyFrameAnimationName() const
    {
        return m_baseKeyFrameAnimationName;
    }

    void Animation::_applyBaseKeyFrame()
    {
        if( !m_useBaseKeyFrame )
        {
            return;
        }

        auto timeIndex = _getTimeIndex( m_baseKeyFrameTime );
        for( auto &track : m_ownedNodeTracks )
        {
            if( track )
            {
                SmartPtr<IAnimationKeyFrame> baseKeyFrame;
                track->getInterpolatedKeyFrame( timeIndex, baseKeyFrame );
                track->_applyBaseKeyFrame( baseKeyFrame );
            }
        }

        for( auto &track : m_ownedVertexTracks )
        {
            if( track )
            {
                SmartPtr<IAnimationKeyFrame> baseKeyFrame;
                track->getInterpolatedKeyFrame( timeIndex, baseKeyFrame );
                track->_applyBaseKeyFrame( baseKeyFrame );
            }
        }
    }

    void Animation::_notifyContainer( IAnimationContainer *c )
    {
        m_container = c;
    }

    IAnimationContainer *Animation::getContainer()
    {
        return m_container;
    }

    void Animation::apply( scene::Mesh *entity, f32 timePos, f32 weight, bool software, bool hardware )
    {
        (void)entity;
        (void)software;
        (void)hardware;

        auto timeIndex = _getTimeIndex( timePos );
        for( const auto &pair : m_vertexTracks )
        {
            if( pair.second )
            {
                pair.second->apply( timeIndex, weight, 1.0f );
            }
        }
    }

    void Animation::apply( ISkeleton *skeleton, f32 timePos, float weight,
                           const IAnimationState::BoneBlendMask *blendMask, f32 scale )
    {
        (void)blendMask;

        auto timeIndex = _getTimeIndex( timePos );
        for( const auto &pair : m_nodeTracks )
        {
            if( pair.second )
            {
                if( skeleton )
                {
                    auto actorTrack = dynamic_cast<ActorAnimationTrack *>( pair.second );
                    if( actorTrack )
                    {
                        auto boneName = actorTrack->getPropertyName();
                        if( StringUtil::isNullOrEmpty( boneName ) )
                            if( const auto importedBone = actorTrack->getBone() )
                                boneName = importedBone->getName();
                        if( !StringUtil::isNullOrEmpty( boneName ) )
                        {
                            actorTrack->applyToBone( skeleton->getBone( boneName ), timeIndex, weight, scale );
                        }
                        // A missing binding must not animate the original imported skeleton.
                        continue;
                    }
                }

                pair.second->apply( timeIndex, weight, scale );
            }
        }
    }

    void Animation::applyToNode( scene::IGameActor *node, f32 timePos, f32 weight /*= 1.0*/,
                                 f32 scale /*= 1.0f */ )
    {
        auto timeIndex = _getTimeIndex( timePos );
        for( const auto &pair : m_nodeTracks )
        {
            if( pair.second )
            {
                auto previousActor = pair.second->getActor();
                if( node )
                {
                    pair.second->setActor( SmartPtr<scene::IGameActor>( node ) );
                }

                pair.second->apply( timeIndex, weight, scale );

                if( node )
                {
                    pair.second->setActor( previousActor );
                }
            }
        }
    }

    void Animation::apply( ISkeleton *skeleton, f32 timePos, f32 weight /*= 1.0*/,
                           f32 scale /*= 1.0f */ )
    {
        apply( skeleton, timePos, weight, nullptr, scale );
    }

    Array<SmartPtr<IAnimationVertexTrack>> Animation::getVertexTracks() const
    {
        return m_ownedVertexTracks;
    }

    void Animation::setVertexTracks( const Array<SmartPtr<IAnimationVertexTrack>> &vertexTracks )
    {
        destroyAllVertexTracks();

        u16 handle = 0;
        for( const auto &track : vertexTracks )
        {
            if( track )
            {
                m_vertexTracks[handle++] = track.get();
                m_ownedVertexTracks.push_back( track );
            }
        }
    }

    Array<SmartPtr<IActorAnimationTrack>> Animation::getNodeTracks() const
    {
        return m_ownedNodeTracks;
    }

    void Animation::setNodeTracks( const Array<SmartPtr<IActorAnimationTrack>> &nodeTracks )
    {
        destroyAllNodeTracks();

        u16 handle = 0;
        for( const auto &track : nodeTracks )
        {
            if( track )
            {
                m_nodeTracks[handle++] = track.get();
                m_ownedNodeTracks.push_back( track );
            }
        }
    }

    Array<SmartPtr<IAnimationNumericTrack>> Animation::getNumericTracks() const
    {
        return m_ownedNumericTracks;
    }

    void Animation::setNumericTracks( const Array<SmartPtr<IAnimationNumericTrack>> &numericTracks )
    {
        destroyAllNumericTracks();

        u16 handle = 0;
        for( const auto &track : numericTracks )
        {
            if( track )
            {
                m_numericTracks[handle++] = track.get();
                m_ownedNumericTracks.push_back( track );
            }
        }
    }
}  // namespace workphone
