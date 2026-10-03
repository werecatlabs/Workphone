#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Cutscene.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>

namespace workphone::scene
{
    const String Cutscene::nameStr = String( "name" );
    const String Cutscene::lengthStr = String( "length" );
    const String Cutscene::loopingStr = String( "looping" );
    const String Cutscene::tracksStr = String( "tracks" );
    const String Cutscene::trackStr = String( "track" );
    const String Cutscene::targetActorStr = String( "targetActor" );
    const String Cutscene::trackTypeStr = String( "trackType" );
    const String Cutscene::keyframesStr = String( "keyframes" );
    const String Cutscene::keyframeStr = String( "keyframe" );
    const String Cutscene::timeStr = String( "time" );
    const String Cutscene::vectorValueStr = String( "vectorValue" );
    const String Cutscene::scalarValueStr = String( "scalarValue" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Cutscene, Resource<IResource> );

    Cutscene::Cutscene()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
    }

    Cutscene::~Cutscene() = default;

    void Cutscene::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            if( data )
            {
                if( data->isDerived<Properties>() )
                {
                    setProperties( workphone::static_pointer_cast<Properties>( data ) );
                }
            }
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Cutscene::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );
                m_tracks.clear();
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Cutscene::saveToFile( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto fileSystem = applicationManager->getFileSystemPtr();
            if( !fileSystem )
            {
                return;
            }

            auto properties = getProperties();
            if( properties )
            {
                auto data = DataUtil::toString( properties.get(), true );
                fileSystem->writeAllText( filePath, data );
                setFilePath( filePath );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Cutscene::loadFromFile( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto fileSystem = applicationManager->getFileSystemPtr();
            if( !fileSystem )
            {
                return;
            }

            auto data = fileSystem->readAllText( filePath );
            auto properties = workphone::make_ptr<Properties>();
            DataUtil::parse( data, properties.get() );
            setProperties( properties );
            setFilePath( filePath );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Cutscene::save()
    {
        auto path = getFilePath();
        if( !path.empty() )
        {
            saveToFile( path );
        }
    }

    SmartPtr<Properties> Cutscene::getProperties() const
    {
        auto properties = Resource<IResource>::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( lengthStr, m_length );
        properties->setProperty( loopingStr, m_looping );

        auto tracksNode = workphone::make_ptr<Properties>();
        tracksNode->setName( tracksStr );

        for( const auto &track : m_tracks )
        {
            auto trackNode = workphone::make_ptr<Properties>();
            trackNode->setName( trackStr );
            trackNode->setProperty( targetActorStr, track.targetActorName );
            trackNode->setProperty( trackTypeStr, trackTypeToString( track.type ) );

            auto keyframesNode = workphone::make_ptr<Properties>();
            keyframesNode->setName( keyframesStr );

            for( const auto &keyframe : track.keyframes )
            {
                auto keyframeNode = workphone::make_ptr<Properties>();
                keyframeNode->setName( keyframeStr );
                keyframeNode->setProperty( timeStr, keyframe.time );
                keyframeNode->setProperty(
                    vectorValueStr,
                    Vector3D( keyframe.vectorValue.x, keyframe.vectorValue.y, keyframe.vectorValue.z ) );
                keyframeNode->setProperty( scalarValueStr, keyframe.scalarValue );
                keyframesNode->addChild( keyframeNode );
            }

            trackNode->addChild( keyframesNode );
            tracksNode->addChild( trackNode );
        }

        properties->addChild( tracksNode );

        return properties;
    }

    void Cutscene::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        Resource<IResource>::setProperties( properties );

        properties->getPropertyValue( lengthStr, m_length );
        properties->getPropertyValue( loopingStr, m_looping );

        m_tracks.clear();

        if( auto tracksNode = properties->getChild( tracksStr ) )
        {
            for( auto &trackNode : tracksNode->getChildrenByName( trackStr ) )
            {
                if( !trackNode )
                {
                    continue;
                }

                Track track;
                trackNode->getPropertyValue( targetActorStr, track.targetActorName );

                String typeStr;
                if( trackNode->getPropertyValue( trackTypeStr, typeStr ) )
                {
                    track.type = stringToTrackType( typeStr );
                }

                if( auto keyframesNode = trackNode->getChild( keyframesStr ) )
                {
                    for( auto &keyframeNode : keyframesNode->getChildrenByName( keyframeStr ) )
                    {
                        if( !keyframeNode )
                        {
                            continue;
                        }

                        Keyframe keyframe;
                        keyframeNode->getPropertyValue( timeStr, keyframe.time );

                        Vector3D vectorValue;
                        if( keyframeNode->getPropertyValue( vectorValueStr, vectorValue ) )
                        {
                            keyframe.vectorValue =
                                Vector3<real_Num>( static_cast<real_Num>( vectorValue.x ),
                                                   static_cast<real_Num>( vectorValue.y ),
                                                   static_cast<real_Num>( vectorValue.z ) );
                        }

                        keyframeNode->getPropertyValue( scalarValueStr, keyframe.scalarValue );
                        track.keyframes.push_back( keyframe );
                    }
                }

                m_tracks.push_back( track );
            }
        }

        sortKeyframes();
    }

    void Cutscene::setLength( f32 length )
    {
        m_length = std::max( length, 0.0f );
    }

    f32 Cutscene::getLength() const
    {
        return m_length;
    }

    void Cutscene::setLooping( bool looping )
    {
        m_looping = looping;
    }

    bool Cutscene::isLooping() const
    {
        return m_looping;
    }

    s32 Cutscene::addTrack( const Track &track )
    {
        m_tracks.push_back( track );
        sortKeyframes();
        return static_cast<s32>( m_tracks.size() ) - 1;
    }

    void Cutscene::removeTrack( s32 index )
    {
        if( index >= 0 && index < static_cast<s32>( m_tracks.size() ) )
        {
            m_tracks.erase( m_tracks.begin() + index );
        }
    }

    Array<Cutscene::Track> Cutscene::getTracks() const
    {
        return m_tracks;
    }

    void Cutscene::setTracks( const Array<Track> &tracks )
    {
        m_tracks = tracks;
        sortKeyframes();
    }

    namespace
    {
        template <class T>
        T lerp( const T &a, const T &b, f32 t )
        {
            return a + ( b - a ) * static_cast<real_Num>( t );
        }
    }  // namespace

    void Cutscene::evaluate( f32 rawTime, Map<Pair<String, TrackType>, Keyframe> &outValues ) const
    {
        f32 time = rawTime;
        if( m_length > 0.0f && m_looping )
        {
            time = std::fmod( rawTime, m_length );
            if( time < 0.0f )
            {
                time += m_length;
            }
        }
        else
        {
            time = std::clamp( rawTime, 0.0f, m_length );
        }

        for( const auto &track : m_tracks )
        {
            if( track.keyframes.empty() )
            {
                continue;
            }

            Keyframe result;
            result.time = time;

            if( track.keyframes.size() == 1 )
            {
                result.vectorValue = track.keyframes.front().vectorValue;
                result.scalarValue = track.keyframes.front().scalarValue;
            }
            else
            {
                const Keyframe *prev = &track.keyframes.front();
                const Keyframe *next = &track.keyframes.back();

                for( u32 i = 1; i < track.keyframes.size(); ++i )
                {
                    if( track.keyframes[i].time >= time )
                    {
                        next = &track.keyframes[i];
                        prev = &track.keyframes[i - 1];
                        break;
                    }
                }

                f32 t = 0.0f;
                if( next->time > prev->time )
                {
                    t = ( time - prev->time ) / ( next->time - prev->time );
                }

                result.vectorValue = lerp( prev->vectorValue, next->vectorValue, t );
                result.scalarValue = prev->scalarValue + ( next->scalarValue - prev->scalarValue ) * t;
            }

            outValues[Pair<String, TrackType>( track.targetActorName, track.type )] = result;
        }
    }

    String Cutscene::trackTypeToString( TrackType type )
    {
        switch( type )
        {
        case TrackType::Position:
            return String( "Position" );
        case TrackType::Rotation:
            return String( "Rotation" );
        case TrackType::Scale:
            return String( "Scale" );
        case TrackType::CameraFOV:
            return String( "CameraFOV" );
        }
        return String( "Position" );
    }

    Cutscene::TrackType Cutscene::stringToTrackType( const String &str )
    {
        if( str == String( "Rotation" ) )
        {
            return TrackType::Rotation;
        }
        if( str == String( "Scale" ) )
        {
            return TrackType::Scale;
        }
        if( str == String( "CameraFOV" ) )
        {
            return TrackType::CameraFOV;
        }
        return TrackType::Position;
    }

    void Cutscene::sortKeyframes()
    {
        for( auto &track : m_tracks )
        {
            std::sort( track.keyframes.begin(), track.keyframes.end(),
                       []( const Keyframe &a, const Keyframe &b ) { return a.time < b.time; } );
        }
    }
}  // namespace workphone::scene
