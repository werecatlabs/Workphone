#ifndef AnimationGraph_h__
#define AnimationGraph_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>
#include <Workphone/Animation/AnimationSyncTrack.hpp>
#include <Workphone/Animation/AnimationGraphNodes.hpp>
#include <map>

namespace workphone
{
    class IAnimation;
    class ISkeleton;
}  // namespace workphone

namespace workphone::animation
{
    struct WPCore_API AnimationGraphClip
    {
        String id;
        SmartPtr<IAnimation> animation;
        AnimationEventTimeline eventTimeline;
        AnimationSyncTrack syncTrack;
        f32 speed = 1.0f;
        bool loop = true;

        f32 getDuration() const;
        bool isValid() const;
    };

    struct WPCore_API AnimationGraphState
    {
        String id;
        String clipId;
        f32 speed = 1.0f;

        /**
         * Optional pose node tree that drives this state instead of a single clip. When
         * set, the state evaluates the referenced blend/selector tree; clipId may be
         * empty. When empty, the state keeps the legacy single-clip behaviour.
         */
        String poseNodeId;

        bool isValid() const;
    };

    enum class AnimationGraphComparison
    {
        Equal,
        NotEqual,
        Greater,
        GreaterOrEqual,
        Less,
        LessOrEqual
    };

    struct WPCore_API AnimationGraphCondition
    {
        String parameter;
        AnimationGraphComparison comparison = AnimationGraphComparison::Equal;
        f32 threshold = 0.0f;
        bool booleanValue = false;
        bool isBoolean = false;
    };

    struct WPCore_API AnimationGraphTransition
    {
        String fromState;
        String toState;
        Array<AnimationGraphCondition> conditions;
        f32 duration = 0.15f;
        f32 exitTime = -1.0f;
        s32 priority = 0;
        bool synchronize = true;

        bool isValid() const;
    };

    /** Serializable/editor-facing graph definition with no UI dependency. */
    class WPCore_API AnimationGraphDefinition
    {
    public:
        bool addClip( const AnimationGraphClip &clip );
        bool addState( const AnimationGraphState &state );
        bool addTransition( const AnimationGraphTransition &transition );

        bool removeClip( const String &id );
        bool removeState( const String &id );
        void clear();

        const AnimationGraphClip *findClip( const String &id ) const;
        const AnimationGraphState *findState( const String &id ) const;
        const Array<AnimationGraphClip> &getClips() const;
        const Array<AnimationGraphState> &getStates() const;
        const Array<AnimationGraphTransition> &getTransitions() const;

        void setInitialState( const String &stateId );
        const String &getInitialState() const;
        bool isValid() const;

        /** Pose node tree authoring API (Esoterica-derived blend/selector nodes). */
        bool addPoseNode( const AnimationGraphPoseNodeDefinition &node );
        bool removePoseNode( const String &id );
        const AnimationGraphPoseNodeDefinition *findPoseNode( const String &id ) const;
        const Array<AnimationGraphPoseNodeDefinition> &getPoseNodes() const;

    private:
        Array<AnimationGraphClip> m_clips;
        Array<AnimationGraphState> m_states;
        Array<AnimationGraphTransition> m_transitions;
        Array<AnimationGraphPoseNodeDefinition> m_poseNodes;
        String m_initialState;
    };

    struct WPCore_API AnimationGraphSample
    {
        String stateId;
        SmartPtr<IAnimation> animation;
        f32 time = 0.0f;
        f32 normalizedTime = 0.0f;
        f32 weight = 0.0f;
    };

    struct WPCore_API AnimationGraphEvent
    {
        String stateId;
        SampledAnimationEvent sampledEvent;
    };

    /** Runtime instance that evaluates state transitions and applies weighted clips. */
    class WPCore_API AnimationGraphInstance
    {
    public:
        AnimationGraphInstance() = default;
        explicit AnimationGraphInstance( const AnimationGraphDefinition &definition );

        bool setDefinition( const AnimationGraphDefinition &definition );
        const AnimationGraphDefinition &getDefinition() const;

        void reset();
        bool forceState( const String &stateId, f32 normalizedTime = 0.0f );
        void update( f32 deltaTime );
        void apply( ISkeleton *skeleton = nullptr, f32 scale = 1.0f, f32 globalWeight = 1.0f ) const;

        void setBoolParameter( const String &name, bool value );
        bool getBoolParameter( const String &name, bool defaultValue = false ) const;
        void setFloatParameter( const String &name, f32 value );
        f32 getFloatParameter( const String &name, f32 defaultValue = 0.0f ) const;

        const String &getCurrentState() const;
        bool isTransitioning() const;
        f32 getTransitionProgress() const;
        const Array<AnimationGraphSample> &getSamples() const;
        const Array<AnimationGraphEvent> &getSampledEvents() const;
        bool isValid() const;

    private:
        struct AdvanceResult
        {
            f32 time = 0.0f;
            bool looped = false;
        };

        bool conditionsMet( const AnimationGraphTransition &transition ) const;
        const AnimationGraphTransition *findTransition() const;
        const AnimationGraphClip *getClipForState( const String &stateId ) const;
        AdvanceResult advanceState( const String &stateId, f32 time, f32 deltaTime ) const;
        void sampleEvents( const String &stateId, f32 previousTime, const AdvanceResult &advance );
        void beginTransition( const AnimationGraphTransition &transition );
        void rebuildSamples();

        // Pose tree evaluation helpers (Esoterica-derived node graph).
        const AnimationGraphPoseNodeDefinition *resolveStatePose( const String &stateId ) const;
        f32 getStateEffectiveDuration( const String &stateId ) const;
        bool getStateLoops( const String &stateId ) const;
        f32 getPoseTreeDuration( const String &nodeId, Array<String> &visited ) const;
        bool getPoseTreeLoops( const String &nodeId, Array<String> &visited ) const;
        void collectStateSamples( const String &stateId, f32 time, f32 weight,
                                  Array<AnimationGraphSample> &outSamples ) const;
        void evaluatePoseTree( const String &stateId, const String &nodeId, f32 normalizedTime,
                               f32 weight, Array<AnimationGraphSample> &outSamples,
                               Array<String> &visited ) const;
        void samplePoseTreeEvents( const String &stateId, const String &nodeId, f32 previousNormalized,
                                   f32 currentNormalized, bool looped, Array<String> &visited );

        AnimationGraphDefinition m_definition;
        std::map<String, bool> m_boolParameters;
        std::map<String, f32> m_floatParameters;
        String m_currentState;
        String m_destinationState;
        f32 m_currentTime = 0.0f;
        f32 m_destinationTime = 0.0f;
        f32 m_transitionTime = 0.0f;
        f32 m_transitionDuration = 0.0f;
        Array<AnimationGraphSample> m_samples;
        Array<AnimationGraphEvent> m_sampledEvents;
    };
}  // namespace workphone::animation

#endif  // AnimationGraph_h__
