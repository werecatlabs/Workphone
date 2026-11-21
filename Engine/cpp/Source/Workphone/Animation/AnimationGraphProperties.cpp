#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationGraphProperties.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace workphone::animation
{
    namespace
    {
        constexpr u32 currentVersion = 1;

        void require( bool valid )
        {
            if( !valid )
            {
                throw std::invalid_argument( "Invalid authored animation graph" );
            }
        }

        bool validText( const String &value )
        {
            return value.size() <= 4096 && value.find( '\0' ) == String::npos;
        }

        struct Reader
        {
            const Properties &properties;
            u32 readFields = 0;

            void field( const char *name, String &value )
            {
                require( properties.getPropertyValue( name, value ) && validText( value ) );
                ++readFields;
            }

            void field( const char *name, bool &value )
            {
                String raw;
                field( name, raw );
                require( raw == "true" || raw == "false" || raw == "1" || raw == "0" );
                value = raw == "true" || raw == "1";
            }

            template <class Number>
            void field( const char *name, Number &value )
            {
                String raw;
                field( name, raw );
                const auto end = raw.data() + raw.size();
                const auto parsed = std::from_chars( raw.data(), end, value );
                require( parsed.ec == std::errc{} && parsed.ptr == end );
                if constexpr( std::is_floating_point_v<Number> )
                {
                    require( std::isfinite( value ) );
                }
            }

            template <class... Values>
            void fields( Values &&...values )
            {
                ( field( values.first, values.second ), ... );
            }

            void finish( bool leaf = true ) const
            {
                require( readFields == properties.getNumProperties() );
                if( leaf )
                {
                    require( properties.getChildren().empty() );
                }
            }
        };

        struct Writer
        {
            SmartPtr<Properties> properties;

            explicit Writer( const char *name ) : properties( make_ptr<Properties>() )
            {
                properties->setName( name );
            }

            void field( const char *name, const String &value )
            {
                require( validText( value ) );
                properties->setProperty( name, value );
            }

            void field( const char *name, f32 value )
            {
                require( std::isfinite( value ) );
                char buffer[32];
                const auto result = std::to_chars( buffer, buffer + sizeof( buffer ), value );
                require( result.ec == std::errc{} );
                properties->setProperty( name, String( buffer, size_t( result.ptr - buffer ) ) );
            }

            template <class Value>
            void field( const char *name, const Value &value )
            {
                properties->setProperty( name, value );
            }

            template <class... Values>
            void fields( Values &&...values )
            {
                ( field( values.first, values.second ), ... );
            }

            template <class WriteChild>
            void child( const char *name, WriteChild writeChild )
            {
                Writer result( name );
                writeChild( result );
                properties->addChild( result.properties );
            }
        };

        template <class Value>
        auto member( const char *name, Value &value )
        {
            return std::pair<const char *, Value &>( name, value );
        }

        template <class Record, class Archive>
        void parameterFields( Archive &archive, Record &parameter )
        {
            archive.fields( member( "name", parameter.name ), member( "boolean", parameter.isBoolean ),
                            member( "floatValue", parameter.floatValue ),
                            member( "booleanValue", parameter.booleanValue ) );
        }

        template <class Record, class Archive>
        void stateFields( Archive &archive, Record &state )
        {
            archive.fields( member( "id", state.id ), member( "clip", state.clipId ),
                            member( "pose", state.poseNodeId ), member( "speed", state.speed ),
                            member( "hasPosition", state.hasEditorPosition ) );
            if( state.hasEditorPosition )
            {
                archive.fields( member( "x", state.editorX ), member( "y", state.editorY ) );
            }
        }

        template <class Record, class Archive>
        void clipFields( Archive &archive, Record &clip )
        {
            archive.fields( member( "id", clip.id ), member( "speed", clip.speed ),
                            member( "loop", clip.loop ) );
        }

        template <class Record, class Archive>
        void transitionFields( Archive &archive, Record &transition )
        {
            archive.fields( member( "from", transition.fromState ), member( "to", transition.toState ),
                            member( "duration", transition.duration ),
                            member( "exitTime", transition.exitTime ),
                            member( "priority", transition.priority ),
                            member( "synchronize", transition.synchronize ) );
        }

        template <class Record, class Archive>
        void conditionFields( Archive &archive, Record &condition )
        {
            archive.fields( member( "parameter", condition.parameter ),
                            member( "threshold", condition.threshold ),
                            member( "boolean", condition.isBoolean ),
                            member( "booleanValue", condition.booleanValue ) );
        }

        template <class Record, class Archive>
        void poseFields( Archive &archive, Record &pose )
        {
            archive.fields( member( "id", pose.id ), member( "clip", pose.clipId ),
                            member( "speed", pose.speed ), member( "loop", pose.loop ),
                            member( "parameter", pose.parameter ) );
        }

        template <class Record, class Archive>
        void rangeFields( Archive &archive, Record &range )
        {
            archive.fields( member( "input0", range.inputIndex0 ), member( "input1", range.inputIndex1 ),
                            member( "low", range.parameterLow ), member( "high", range.parameterHigh ) );
        }

        template <class Record, class Archive>
        void eventFields( Archive &archive, Record &event )
        {
            archive.fields( member( "id", event.id ), member( "payload", event.payload ),
                            member( "start", event.startTime ), member( "duration", event.duration ),
                            member( "sync", event.isSyncMarker ) );
        }

        template <class Record, class Archive>
        void markerFields( Archive &archive, Record &marker )
        {
            archive.fields( member( "id", marker.id ), member( "start", marker.startTime ) );
        }

        void readClip( Reader &reader, AnimationGraphClip &clip, String &sourceName )
        {
            clipFields( reader, clip );
            s32 syncOffset = 0;
            reader.fields( member( "animation", sourceName ), member( "syncOffset", syncOffset ) );
            require( !sourceName.empty() );
            reader.finish( false );

            auto markers = makeSourceBudgetSnapshot<AnimationSyncMarker>();
            for( const auto &child : reader.properties.getChildren() )
            {
                require( bool( child ) );
                Reader record{ *child };
                if( child->getName() == "eventTrack" )
                {
                    String name;
                    bool enabled = true;
                    record.fields( member( "name", name ), member( "enabled", enabled ) );
                    record.finish( false );
                    auto &timeline = clip.eventTimeline;
                    const auto index = timeline.addTrack( name );
                    require( index != AnimationEventTimeline::InvalidIndex );
                    timeline.getTrack( index )->enabled = enabled;
                    for( const auto &eventProperties : child->getChildren() )
                    {
                        require( eventProperties && eventProperties->getName() == "event" );
                        Reader eventReader{ *eventProperties };
                        AnimationEvent event;
                        eventFields( eventReader, event );
                        eventReader.finish();
                        require( timeline.addEvent( index, event ) !=
                                 AnimationEventTimeline::InvalidIndex );
                    }
                }
                else
                {
                    require( child->getName() == "syncMarker" );
                    AnimationSyncMarker marker;
                    markerFields( record, marker );
                    record.finish();
                    markers.push_back( std::move( marker ) );
                }
            }
            require( clip.syncTrack.setMarkers( markers ) );
            clip.syncTrack.setStartEventOffset( syncOffset );
        }

        template <class Resolve>
        AnimationGraphDefinition readGraph( const Properties &properties, Resolve resolve )
        {
            require( properties.getName() == animationGraphPropertiesName );
            Reader header{ properties };
            u32 version = 0;
            String initialState;
            header.fields( member( "version", version ), member( "initialState", initialState ) );
            header.finish( false );
            require( version == currentVersion );

            AnimationGraphDefinition definition;
            const auto records = properties.getChildren();
            auto mask = IAnimationState::BoneBlendMask();
            for( const auto &child : records )
            {
                require( bool( child ) );
                const auto name = child->getName();
                Reader record{ *child };
                if( name == "clip" )
                {
                    String sourceName;
                    AnimationGraphClip clip;
                    readClip( record, clip, sourceName );
                    clip.animation = resolve( sourceName );
                    require( definition.addClip( clip ) );
                }
                else if( name == "parameter" )
                {
                    AnimationGraphParameter parameter;
                    parameterFields( record, parameter );
                    record.finish();
                    require( definition.addParameter( parameter ) );
                }
                else if( name == "pose" )
                {
                    AnimationGraphPoseNodeDefinition pose;
                    u32 type = 0;
                    record.field( "type", type );
                    require( type <= static_cast<u32>( AnimationGraphPoseNodeType::Selector ) );
                    pose.type = static_cast<AnimationGraphPoseNodeType>( type );
                    poseFields( record, pose );
                    record.finish( false );
                    for( const auto &input : child->getChildren() )
                    {
                        require( bool( input ) );
                        Reader inputReader{ *input };
                        if( input->getName() == "input" )
                        {
                            String id;
                            inputReader.field( "id", id );
                            pose.childNodeIds.push_back( std::move( id ) );
                        }
                        else
                        {
                            require( input->getName() == "range" );
                            AnimationGraphBlendRange range;
                            rangeFields( inputReader, range );
                            pose.blendRanges.push_back( range );
                        }
                        inputReader.finish();
                    }
                    require( definition.addPoseNode( pose ) );
                }
                else if( name == "weight" )
                {
                    f32 weight = 0;
                    record.field( "value", weight );
                    record.finish();
                    mask.push_back( weight );
                }
                else
                {
                    require( name == "state" || name == "transition" );
                }
            }
            for( const auto &child : records )
            {
                if( child->getName() == "state" )
                {
                    Reader record{ *child };
                    AnimationGraphState state;
                    stateFields( record, state );
                    record.finish();
                    require( definition.addState( state ) );
                }
            }
            for( const auto &child : records )
            {
                if( child->getName() != "transition" )
                {
                    continue;
                }
                Reader record{ *child };
                AnimationGraphTransition transition;
                transitionFields( record, transition );
                record.finish( false );
                for( const auto &entry : child->getChildren() )
                {
                    require( entry && entry->getName() == "condition" );
                    Reader conditionReader{ *entry };
                    AnimationGraphCondition condition;
                    u32 comparison = 0;
                    conditionReader.field( "comparison", comparison );
                    require( comparison <= static_cast<u32>( AnimationGraphComparison::LessOrEqual ) );
                    condition.comparison = static_cast<AnimationGraphComparison>( comparison );
                    conditionFields( conditionReader, condition );
                    conditionReader.finish();
                    transition.conditions.push_back( std::move( condition ) );
                }
                require( definition.addTransition( transition ) );
            }

            require( definition.findState( initialState ) && definition.setBoneBlendMask( mask ) );
            definition.setInitialState( initialState );
            require( definition.isValid() );
            return definition;
        }
    }  // namespace

    SmartPtr<Properties> writeAnimationGraphProperties( const AnimationGraphDefinition &definition )
    {
        if( !definition.isValid() )
        {
            return nullptr;
        }
        try
        {
            Writer writer( animationGraphPropertiesName );
            writer.field( "version", currentVersion );
            writer.field( "initialState", definition.getInitialState() );
            for( const auto &clip : definition.getClips() )
            {
                writer.child( "clip", [&]( Writer &record ) {
                    clipFields( record, clip );
                    const auto name = clip.animation->getName();
                    require( !name.empty() );
                    record.field( "animation", name );
                    record.field( "syncOffset", clip.syncTrack.getStartEventOffset() );
                    for( const auto &track : clip.eventTimeline.getTracks() )
                    {
                        record.child( "eventTrack", [&]( Writer &trackRecord ) {
                            trackRecord.fields( member( "name", track.name ),
                                                member( "enabled", track.enabled ) );
                            for( const auto &event : track.events )
                            {
                                trackRecord.child( "event", [&]( Writer &eventRecord ) {
                                    eventFields( eventRecord, event );
                                } );
                            }
                        } );
                    }
                    for( const auto &marker : clip.syncTrack.getMarkers() )
                    {
                        record.child( "syncMarker", [&]( Writer &markerRecord ) {
                            markerFields( markerRecord, marker );
                        } );
                    }
                } );
            }
            for( const auto &parameter : definition.getParameters() )
            {
                writer.child( "parameter",
                              [&]( Writer &record ) { parameterFields( record, parameter ); } );
            }
            for( const auto &pose : definition.getPoseNodes() )
            {
                writer.child( "pose", [&]( Writer &record ) {
                    poseFields( record, pose );
                    record.field( "type", static_cast<u32>( pose.type ) );
                    for( const auto &input : pose.childNodeIds )
                    {
                        record.child( "input",
                                      [&]( Writer &inputRecord ) { inputRecord.field( "id", input ); } );
                    }
                    for( const auto &range : pose.blendRanges )
                    {
                        record.child(
                            "range", [&]( Writer &rangeRecord ) { rangeFields( rangeRecord, range ); } );
                    }
                } );
            }
            for( const auto &state : definition.getStates() )
            {
                writer.child( "state", [&]( Writer &record ) { stateFields( record, state ); } );
            }
            for( const auto &transition : definition.getTransitions() )
            {
                writer.child( "transition", [&]( Writer &record ) {
                    transitionFields( record, transition );
                    for( const auto &condition : transition.conditions )
                    {
                        record.child( "condition", [&]( Writer &conditionRecord ) {
                            conditionFields( conditionRecord, condition );
                            conditionRecord.field( "comparison",
                                                   static_cast<u32>( condition.comparison ) );
                        } );
                    }
                } );
            }
            for( const auto weight : definition.getBoneBlendMask() )
            {
                writer.child( "weight", [&]( Writer &record ) { record.field( "value", weight ); } );
            }
            return writer.properties;
        }
        catch( const std::invalid_argument & )
        {
            return nullptr;
        }
    }

    bool validateAnimationGraphProperties( const Properties &properties )
    {
        try
        {
            auto placeholder = make_ptr<Animation>();
            placeholder->setLength( 1.0f );
            readGraph( properties, [&]( const String & ) { return placeholder; } );
            return true;
        }
        catch( const std::invalid_argument & )
        {
            return false;
        }
    }

    bool readAnimationGraphProperties( const Properties &properties,
                                       const Array<SmartPtr<IAnimation>> &clips,
                                       AnimationGraphDefinition &definition )
    {
        try
        {
            definition = readGraph( properties, [&]( const String &name ) {
                SmartPtr<IAnimation> match;
                for( const auto &animation : clips )
                {
                    if( animation && animation->getName() == name )
                    {
                        require( !match || match == animation );
                        match = animation;
                    }
                }
                return match;
            } );
            return true;
        }
        catch( const std::invalid_argument & )
        {
            return false;
        }
    }
}  // namespace workphone::animation
