#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::animation
{
    namespace
    {
        constexpr f32 Epsilon = std::numeric_limits<f32>::epsilon();

        f32 normalizedTime( f32 time, f32 duration )
        {
            return duration > Epsilon ? std::clamp( time / duration, 0.0f, 1.0f ) : 0.0f;
        }

        bool visitedContains( const Array<String> &visited, const String &id )
        {
            return std::find( visited.begin(), visited.end(), id ) != visited.end();
        }
    }  // namespace

    f32 AnimationGraphClip::getDuration() const
    {
        return animation ? std::max( 0.0f, animation->getLength() ) : 0.0f;
    }

    bool AnimationGraphClip::isValid() const
    {
        return !id.empty() && animation && getDuration() > Epsilon && std::isfinite( speed ) &&
               speed >= 0.0f && eventTimeline.isValid() && syncTrack.isValid();
    }

    bool AnimationGraphState::isValid() const
    {
        return !id.empty() && ( !clipId.empty() || !poseNodeId.empty() ) && std::isfinite( speed ) &&
               speed >= 0.0f;
    }

    bool AnimationGraphTransition::isValid() const
    {
        return !fromState.empty() && !toState.empty() && fromState != toState &&
               std::isfinite( duration ) && duration >= 0.0f && std::isfinite( exitTime ) &&
               exitTime <= 1.0f;
    }

    bool AnimationGraphDefinition::addClip( const AnimationGraphClip &clip )
    {
        if( !clip.isValid() || findClip( clip.id ) )
        {
            return false;
        }
        m_clips.push_back( clip );
        return true;
    }

    bool AnimationGraphDefinition::addState( const AnimationGraphState &state )
    {
        if( !state.isValid() || findState( state.id ) )
        {
            return false;
        }
        if( !state.clipId.empty() && !findClip( state.clipId ) )
        {
            return false;
        }
        if( !state.poseNodeId.empty() && !findPoseNode( state.poseNodeId ) )
        {
            return false;
        }
        m_states.push_back( state );
        if( m_initialState.empty() )
        {
            m_initialState = state.id;
        }
        return true;
    }

    bool AnimationGraphDefinition::addTransition( const AnimationGraphTransition &transition )
    {
        if( !transition.isValid() || !findState( transition.fromState ) ||
            !findState( transition.toState ) )
        {
            return false;
        }
        m_transitions.push_back( transition );
        std::stable_sort( m_transitions.begin(), m_transitions.end(),
                          []( const AnimationGraphTransition &a, const AnimationGraphTransition &b ) {
                              return a.priority > b.priority;
                          } );
        return true;
    }

    bool AnimationGraphDefinition::removeClip( const String &id )
    {
        if( std::any_of( m_states.begin(), m_states.end(),
                         [&id]( const AnimationGraphState &state ) { return state.clipId == id; } ) )
        {
            return false;
        }
        if( std::any_of( m_poseNodes.begin(), m_poseNodes.end(),
                         [&id]( const AnimationGraphPoseNodeDefinition &node ) {
                             return node.type == AnimationGraphPoseNodeType::Clip && node.clipId == id;
                         } ) )
        {
            return false;
        }

        const auto previousSize = m_clips.size();
        m_clips.erase(
            std::remove_if( m_clips.begin(), m_clips.end(),
                            [&id]( const AnimationGraphClip &clip ) { return clip.id == id; } ),
            m_clips.end() );
        return previousSize != m_clips.size();
    }
    bool AnimationGraphDefinition::removeState( const String &id )
    {
        const auto previousSize = m_states.size();
        m_states.erase(
            std::remove_if( m_states.begin(), m_states.end(),
                            [&id]( const AnimationGraphState &state ) { return state.id == id; } ),
            m_states.end() );
        if( previousSize == m_states.size() )
        {
            return false;
        }

        m_transitions.erase( std::remove_if( m_transitions.begin(), m_transitions.end(),
                                             [&id]( const AnimationGraphTransition &transition ) {
                                                 return transition.fromState == id ||
                                                        transition.toState == id;
                                             } ),
                             m_transitions.end() );

        if( m_initialState == id )
        {
            m_initialState = m_states.empty() ? String() : m_states.front().id;
        }
        return true;
    }

    void AnimationGraphDefinition::clear()
    {
        m_clips.clear();
        m_states.clear();
        m_transitions.clear();
        m_poseNodes.clear();
        m_initialState.clear();
    }

    const AnimationGraphClip *AnimationGraphDefinition::findClip( const String &id ) const
    {
        const auto found =
            std::find_if( m_clips.begin(), m_clips.end(),
                          [&id]( const AnimationGraphClip &clip ) { return clip.id == id; } );
        return found != m_clips.end() ? &( *found ) : nullptr;
    }

    const AnimationGraphState *AnimationGraphDefinition::findState( const String &id ) const
    {
        const auto found =
            std::find_if( m_states.begin(), m_states.end(),
                          [&id]( const AnimationGraphState &state ) { return state.id == id; } );
        return found != m_states.end() ? &( *found ) : nullptr;
    }

    const Array<AnimationGraphClip> &AnimationGraphDefinition::getClips() const
    {
        return m_clips;
    }

    const Array<AnimationGraphState> &AnimationGraphDefinition::getStates() const
    {
        return m_states;
    }

    const Array<AnimationGraphTransition> &AnimationGraphDefinition::getTransitions() const
    {
        return m_transitions;
    }

    void AnimationGraphDefinition::setInitialState( const String &stateId )
    {
        if( findState( stateId ) )
        {
            m_initialState = stateId;
        }
    }

    const String &AnimationGraphDefinition::getInitialState() const
    {
        return m_initialState;
    }

    bool AnimationGraphDefinition::isValid() const
    {
        if( m_states.empty() || !findState( m_initialState ) )
        {
            return false;
        }
        return std::all_of( m_clips.begin(), m_clips.end(),
                            []( const AnimationGraphClip &clip ) { return clip.isValid(); } ) &&
               std::all_of( m_states.begin(), m_states.end(),
                            [this]( const AnimationGraphState &state ) {
                                if( !state.isValid() )
                                {
                                    return false;
                                }
                                if( !state.poseNodeId.empty() )
                                {
                                    const auto pose = findPoseNode( state.poseNodeId );
                                    return pose != nullptr && pose->isValid();
                                }
                                return findClip( state.clipId ) != nullptr;
                            } ) &&
               std::all_of( m_transitions.begin(), m_transitions.end(),
                            [this]( const AnimationGraphTransition &transition ) {
                                return transition.isValid() && findState( transition.fromState ) &&
                                       findState( transition.toState );
                            } ) &&
               std::all_of(
                   m_poseNodes.begin(), m_poseNodes.end(),
                   []( const AnimationGraphPoseNodeDefinition &node ) { return node.isValid(); } );
    }

    bool AnimationGraphDefinition::addPoseNode( const AnimationGraphPoseNodeDefinition &node )
    {
        if( !node.isValid() || findPoseNode( node.id ) )
        {
            return false;
        }
        m_poseNodes.push_back( node );
        return true;
    }

    bool AnimationGraphDefinition::removePoseNode( const String &id )
    {
        if( std::any_of( m_states.begin(), m_states.end(),
                         [&id]( const AnimationGraphState &state ) { return state.poseNodeId == id; } ) )
        {
            return false;
        }
        const auto previousSize = m_poseNodes.size();
        m_poseNodes.erase( std::remove_if( m_poseNodes.begin(), m_poseNodes.end(),
                                           [&id]( const AnimationGraphPoseNodeDefinition &node ) {
                                               return node.id == id;
                                           } ),
                           m_poseNodes.end() );
        return previousSize != m_poseNodes.size();
    }

    const AnimationGraphPoseNodeDefinition *AnimationGraphDefinition::findPoseNode(
        const String &id ) const
    {
        const auto found = std::find_if(
            m_poseNodes.begin(), m_poseNodes.end(),
            [&id]( const AnimationGraphPoseNodeDefinition &node ) { return node.id == id; } );
        return found != m_poseNodes.end() ? &( *found ) : nullptr;
    }

    const Array<AnimationGraphPoseNodeDefinition> &AnimationGraphDefinition::getPoseNodes() const
    {
        return m_poseNodes;
    }
    AnimationGraphInstance::AnimationGraphInstance( const AnimationGraphDefinition &definition )
    {
        setDefinition( definition );
    }

    bool AnimationGraphInstance::setDefinition( const AnimationGraphDefinition &definition )
    {
        if( !definition.isValid() )
        {
            return false;
        }
        m_definition = definition;
        reset();
        return true;
    }

    const AnimationGraphDefinition &AnimationGraphInstance::getDefinition() const
    {
        return m_definition;
    }

    void AnimationGraphInstance::reset()
    {
        m_currentState = m_definition.getInitialState();
        m_destinationState.clear();
        m_currentTime = 0.0f;
        m_destinationTime = 0.0f;
        m_transitionTime = 0.0f;
        m_transitionDuration = 0.0f;
        m_sampledEvents.clear();
        rebuildSamples();
    }

    bool AnimationGraphInstance::forceState( const String &stateId, f32 targetNormalizedTime )
    {
        const auto state = m_definition.findState( stateId );
        if( !state )
        {
            return false;
        }

        const auto duration = getStateEffectiveDuration( stateId );
        if( duration <= Epsilon )
        {
            return false;
        }

        m_currentState = stateId;
        m_currentTime = duration * std::clamp( targetNormalizedTime, 0.0f, 1.0f );
        m_destinationState.clear();
        m_destinationTime = 0.0f;
        m_transitionTime = 0.0f;
        m_transitionDuration = 0.0f;
        m_sampledEvents.clear();
        rebuildSamples();
        return true;
    }

    void AnimationGraphInstance::update( f32 deltaTime )
    {
        m_sampledEvents.clear();
        if( !isValid() || !std::isfinite( deltaTime ) || deltaTime <= 0.0f )
        {
            rebuildSamples();
            return;
        }

        const auto previousCurrentTime = m_currentTime;
        const auto currentAdvance = advanceState( m_currentState, m_currentTime, deltaTime );
        m_currentTime = currentAdvance.time;
        sampleEvents( m_currentState, previousCurrentTime, currentAdvance );

        if( isTransitioning() )
        {
            const auto previousDestinationTime = m_destinationTime;
            const auto destinationAdvance =
                advanceState( m_destinationState, m_destinationTime, deltaTime );
            m_destinationTime = destinationAdvance.time;
            sampleEvents( m_destinationState, previousDestinationTime, destinationAdvance );

            m_transitionTime += deltaTime;
            if( m_transitionDuration <= Epsilon || m_transitionTime >= m_transitionDuration )
            {
                m_currentState = m_destinationState;
                m_currentTime = m_destinationTime;
                m_destinationState.clear();
                m_destinationTime = 0.0f;
                m_transitionTime = 0.0f;
                m_transitionDuration = 0.0f;
            }
        }
        else if( const auto transition = findTransition() )
        {
            beginTransition( *transition );
        }

        rebuildSamples();
    }

    void AnimationGraphInstance::apply( ISkeleton *skeleton, f32 scale, f32 globalWeight ) const
    {
        globalWeight = std::clamp( globalWeight, 0.0f, 1.0f );
        for( const auto &sample : m_samples )
        {
            const auto effectiveWeight = sample.weight * globalWeight;
            if( !sample.animation || effectiveWeight <= 0.0f )
            {
                continue;
            }

            // SmartPtr intentionally exposes a const pointee through a const owner. Sampling mutates
            // the animation target, not the graph-owned pointer, so take a local mutable reference.
            auto animation = sample.animation;

            if( skeleton )
            {
                animation->apply( skeleton, sample.time, effectiveWeight, scale );
            }
            else
            {
                animation->apply( sample.time, effectiveWeight, scale );
            }
        }
    }

    void AnimationGraphInstance::setBoolParameter( const String &name, bool value )
    {
        m_boolParameters[name] = value;
    }

    bool AnimationGraphInstance::getBoolParameter( const String &name, bool defaultValue ) const
    {
        const auto found = m_boolParameters.find( name );
        return found != m_boolParameters.end() ? found->second : defaultValue;
    }

    void AnimationGraphInstance::setFloatParameter( const String &name, f32 value )
    {
        if( std::isfinite( value ) )
        {
            m_floatParameters[name] = value;
        }
    }

    f32 AnimationGraphInstance::getFloatParameter( const String &name, f32 defaultValue ) const
    {
        const auto found = m_floatParameters.find( name );
        return found != m_floatParameters.end() ? found->second : defaultValue;
    }

    const String &AnimationGraphInstance::getCurrentState() const
    {
        return m_currentState;
    }

    bool AnimationGraphInstance::isTransitioning() const
    {
        return !m_destinationState.empty();
    }

    f32 AnimationGraphInstance::getTransitionProgress() const
    {
        return isTransitioning() && m_transitionDuration > Epsilon
                   ? std::clamp( m_transitionTime / m_transitionDuration, 0.0f, 1.0f )
                   : 0.0f;
    }

    const Array<AnimationGraphSample> &AnimationGraphInstance::getSamples() const
    {
        return m_samples;
    }

    const Array<AnimationGraphEvent> &AnimationGraphInstance::getSampledEvents() const
    {
        return m_sampledEvents;
    }

    bool AnimationGraphInstance::isValid() const
    {
        return m_definition.isValid() && m_definition.findState( m_currentState ) != nullptr;
    }
    bool AnimationGraphInstance::conditionsMet( const AnimationGraphTransition &transition ) const
    {
        for( const auto &condition : transition.conditions )
        {
            if( condition.isBoolean )
            {
                const auto value = getBoolParameter( condition.parameter );
                const auto equal = value == condition.booleanValue;
                if( ( condition.comparison == AnimationGraphComparison::Equal && !equal ) ||
                    ( condition.comparison == AnimationGraphComparison::NotEqual && equal ) ||
                    ( condition.comparison != AnimationGraphComparison::Equal &&
                      condition.comparison != AnimationGraphComparison::NotEqual ) )
                {
                    return false;
                }
                continue;
            }

            const auto value = getFloatParameter( condition.parameter );
            bool result = false;
            switch( condition.comparison )
            {
            case AnimationGraphComparison::Equal:
                result = std::abs( value - condition.threshold ) <= Epsilon;
                break;
            case AnimationGraphComparison::NotEqual:
                result = std::abs( value - condition.threshold ) > Epsilon;
                break;
            case AnimationGraphComparison::Greater:
                result = value > condition.threshold;
                break;
            case AnimationGraphComparison::GreaterOrEqual:
                result = value >= condition.threshold;
                break;
            case AnimationGraphComparison::Less:
                result = value < condition.threshold;
                break;
            case AnimationGraphComparison::LessOrEqual:
                result = value <= condition.threshold;
                break;
            }
            if( !result )
            {
                return false;
            }
        }
        return true;
    }

    const AnimationGraphTransition *AnimationGraphInstance::findTransition() const
    {
        const auto duration = getStateEffectiveDuration( m_currentState );
        const auto currentNormalizedTime =
            duration > Epsilon ? normalizedTime( m_currentTime, duration ) : 0.0f;
        for( const auto &transition : m_definition.getTransitions() )
        {
            if( transition.fromState == m_currentState &&
                ( transition.exitTime < 0.0f || currentNormalizedTime >= transition.exitTime ) &&
                conditionsMet( transition ) )
            {
                return &transition;
            }
        }
        return nullptr;
    }

    const AnimationGraphClip *AnimationGraphInstance::getClipForState( const String &stateId ) const
    {
        const auto state = m_definition.findState( stateId );
        return state ? m_definition.findClip( state->clipId ) : nullptr;
    }

    AnimationGraphInstance::AdvanceResult AnimationGraphInstance::advanceState( const String &stateId,
                                                                                f32 time,
                                                                                f32 deltaTime ) const
    {
        AdvanceResult result;
        const auto state = m_definition.findState( stateId );
        if( !state )
        {
            return result;
        }

        f32 duration = 0.0f;
        bool loops = false;
        f32 speedScale = state->speed;

        const auto pose = resolveStatePose( stateId );
        if( pose )
        {
            Array<String> durationVisited;
            duration = getPoseTreeDuration( pose->id, durationVisited );
            Array<String> loopVisited;
            loops = getPoseTreeLoops( pose->id, loopVisited );
        }
        else
        {
            const auto clip = getClipForState( stateId );
            if( !clip )
            {
                return result;
            }
            duration = clip->getDuration();
            loops = clip->loop;
            speedScale = clip->speed * state->speed;
        }

        if( duration <= Epsilon )
        {
            return result;
        }

        auto newTime = time + deltaTime * speedScale;
        if( loops )
        {
            result.looped = newTime >= duration;
            newTime = std::fmod( newTime, duration );
        }
        else
        {
            newTime = std::clamp( newTime, 0.0f, duration );
        }
        result.time = newTime;
        return result;
    }

    void AnimationGraphInstance::sampleEvents( const String &stateId, f32 previousTime,
                                               const AdvanceResult &advance )
    {
        const auto pose = resolveStatePose( stateId );
        if( pose )
        {
            const auto duration = getStateEffectiveDuration( stateId );
            if( duration <= Epsilon )
            {
                return;
            }
            const auto previousNormalized = normalizedTime( previousTime, duration );
            const auto currentNormalized = normalizedTime( advance.time, duration );
            Array<String> visited;
            samplePoseTreeEvents( stateId, pose->id, previousNormalized, currentNormalized,
                                  advance.looped, visited );
            return;
        }

        const auto clip = getClipForState( stateId );
        if( !clip || clip->getDuration() <= Epsilon )
        {
            return;
        }

        const auto previousNormalized = normalizedTime( previousTime, clip->getDuration() );
        const auto currentNormalized = normalizedTime( advance.time, clip->getDuration() );
        for( const auto &sample :
             clip->eventTimeline.sampleRange( previousNormalized, currentNormalized, advance.looped ) )
        {
            m_sampledEvents.push_back( { stateId, sample } );
        }
    }

    void AnimationGraphInstance::beginTransition( const AnimationGraphTransition &transition )
    {
        m_destinationState = transition.toState;
        m_transitionTime = 0.0f;
        m_transitionDuration = transition.duration;
        m_destinationTime = 0.0f;

        const auto sourcePose = resolveStatePose( m_currentState );
        const auto destinationPose = resolveStatePose( m_destinationState );
        const auto sourceClip = getClipForState( m_currentState );
        const auto destinationClip = getClipForState( m_destinationState );
        if( transition.synchronize && !sourcePose && !destinationPose && sourceClip && destinationClip )
        {
            const auto sourceNormalized = normalizedTime( m_currentTime, sourceClip->getDuration() );
            const auto syncTime = sourceClip->syncTrack.getTime( sourceNormalized );
            m_destinationTime = destinationClip->syncTrack.getNormalizedTime( syncTime ) *
                                destinationClip->getDuration();
        }

        if( m_transitionDuration <= Epsilon )
        {
            m_currentState = m_destinationState;
            m_currentTime = m_destinationTime;
            m_destinationState.clear();
            m_transitionDuration = 0.0f;
        }
    }
    void AnimationGraphInstance::rebuildSamples()
    {
        m_samples.clear();
        if( isTransitioning() )
        {
            const auto progress = getTransitionProgress();
            collectStateSamples( m_currentState, m_currentTime, 1.0f - progress, m_samples );
            collectStateSamples( m_destinationState, m_destinationTime, progress, m_samples );
        }
        else
        {
            collectStateSamples( m_currentState, m_currentTime, 1.0f, m_samples );
        }
    }

    const AnimationGraphPoseNodeDefinition *AnimationGraphInstance::resolveStatePose(
        const String &stateId ) const
    {
        const auto state = m_definition.findState( stateId );
        if( !state || state->poseNodeId.empty() )
        {
            return nullptr;
        }
        return m_definition.findPoseNode( state->poseNodeId );
    }

    f32 AnimationGraphInstance::getStateEffectiveDuration( const String &stateId ) const
    {
        const auto pose = resolveStatePose( stateId );
        if( pose )
        {
            Array<String> visited;
            return getPoseTreeDuration( pose->id, visited );
        }
        const auto clip = getClipForState( stateId );
        return clip ? clip->getDuration() : 0.0f;
    }

    bool AnimationGraphInstance::getStateLoops( const String &stateId ) const
    {
        const auto pose = resolveStatePose( stateId );
        if( pose )
        {
            Array<String> visited;
            return getPoseTreeLoops( pose->id, visited );
        }
        const auto clip = getClipForState( stateId );
        return clip ? clip->loop : false;
    }

    f32 AnimationGraphInstance::getPoseTreeDuration( const String &nodeId, Array<String> &visited ) const
    {
        if( visitedContains( visited, nodeId ) )
        {
            return 0.0f;
        }
        visited.push_back( nodeId );
        const auto node = m_definition.findPoseNode( nodeId );
        if( !node )
        {
            return 0.0f;
        }

        switch( node->type )
        {
        case AnimationGraphPoseNodeType::Clip:
        {
            const auto clip = m_definition.findClip( node->clipId );
            return clip ? clip->getDuration() : 0.0f;
        }
        case AnimationGraphPoseNodeType::Blend1D:
        case AnimationGraphPoseNodeType::Selector:
        {
            f32 maxDuration = 0.0f;
            for( const auto &childId : node->childNodeIds )
            {
                Array<String> childVisited = visited;
                maxDuration = std::max( maxDuration, getPoseTreeDuration( childId, childVisited ) );
            }
            return maxDuration;
        }
        default:
            return 0.0f;
        }
    }

    bool AnimationGraphInstance::getPoseTreeLoops( const String &nodeId, Array<String> &visited ) const
    {
        if( visitedContains( visited, nodeId ) )
        {
            return false;
        }
        visited.push_back( nodeId );
        const auto node = m_definition.findPoseNode( nodeId );
        if( !node )
        {
            return false;
        }

        switch( node->type )
        {
        case AnimationGraphPoseNodeType::Clip:
        {
            const auto clip = m_definition.findClip( node->clipId );
            return clip ? clip->loop : false;
        }
        case AnimationGraphPoseNodeType::Blend1D:
        case AnimationGraphPoseNodeType::Selector:
        {
            f32 bestDuration = -1.0f;
            bool bestLoop = false;
            for( const auto &childId : node->childNodeIds )
            {
                Array<String> durationVisited = visited;
                const auto childDuration = getPoseTreeDuration( childId, durationVisited );
                Array<String> loopVisited = visited;
                const auto childLoop = getPoseTreeLoops( childId, loopVisited );
                if( childDuration > bestDuration )
                {
                    bestDuration = childDuration;
                    bestLoop = childLoop;
                }
            }
            return bestLoop;
        }
        default:
            return false;
        }
    }

    void AnimationGraphInstance::collectStateSamples( const String &stateId, f32 time, f32 weight,
                                                      Array<AnimationGraphSample> &outSamples ) const
    {
        const auto clampedWeight = std::clamp( weight, 0.0f, 1.0f );
        if( clampedWeight <= 0.0f )
        {
            return;
        }

        const auto pose = resolveStatePose( stateId );
        if( pose )
        {
            const auto duration = getStateEffectiveDuration( stateId );
            const auto nt = normalizedTime( time, duration );
            Array<String> visited;
            evaluatePoseTree( stateId, pose->id, nt, clampedWeight, outSamples, visited );
            return;
        }

        const auto clip = getClipForState( stateId );
        if( !clip || !clip->animation )
        {
            return;
        }
        outSamples.push_back( { stateId, clip->animation, time,
                                normalizedTime( time, clip->getDuration() ), clampedWeight } );
    }

    void AnimationGraphInstance::evaluatePoseTree( const String &stateId, const String &nodeId,
                                                   f32 normalizedTimeValue, f32 weight,
                                                   Array<AnimationGraphSample> &outSamples,
                                                   Array<String> &visited ) const
    {
        if( visitedContains( visited, nodeId ) || weight <= 0.0f )
        {
            return;
        }
        visited.push_back( nodeId );
        const auto node = m_definition.findPoseNode( nodeId );
        if( !node )
        {
            return;
        }

        switch( node->type )
        {
        case AnimationGraphPoseNodeType::Clip:
        {
            const auto clip = m_definition.findClip( node->clipId );
            if( !clip || !clip->animation || clip->getDuration() <= Epsilon )
            {
                return;
            }
            const auto duration = clip->getDuration();
            auto leafTime = normalizedTimeValue * duration * node->speed;
            if( clip->loop )
            {
                leafTime = std::fmod( leafTime, duration );
            }
            else
            {
                leafTime = std::clamp( leafTime, 0.0f, duration );
            }
            outSamples.push_back( { stateId, clip->animation, leafTime,
                                    normalizedTime( leafTime, duration ),
                                    std::clamp( weight, 0.0f, 1.0f ) } );
            return;
        }
        case AnimationGraphPoseNodeType::Blend1D:
        {
            if( node->childNodeIds.size() < 2 || node->blendRanges.empty() )
            {
                return;
            }
            const auto parameterValue = getFloatParameter( node->parameter, 0.0f );
            const auto lastChildIndex = static_cast<u32>( node->childNodeIds.size() - 1 );

            const AnimationGraphBlendRange *range = nullptr;
            for( const auto &candidate : node->blendRanges )
            {
                if( candidate.contains( parameterValue ) )
                {
                    range = &candidate;
                    break;
                }
            }

            if( !range )
            {
                const auto idx = parameterValue <= node->blendRanges.front().parameterLow
                                     ? node->blendRanges.front().inputIndex0
                                     : node->blendRanges.back().inputIndex1;
                const auto childIdx = static_cast<u32>(
                    std::clamp( static_cast<s32>( idx ), 0, static_cast<s32>( lastChildIndex ) ) );
                Array<String> childVisited = visited;
                evaluatePoseTree( stateId, node->childNodeIds[childIdx], normalizedTimeValue, weight,
                                  outSamples, childVisited );
                return;
            }

            const auto blendWeight = range->blendWeight( parameterValue );
            const auto i0 = static_cast<u32>( std::clamp( static_cast<s32>( range->inputIndex0 ), 0,
                                                          static_cast<s32>( lastChildIndex ) ) );
            const auto i1 = static_cast<u32>( std::clamp( static_cast<s32>( range->inputIndex1 ), 0,
                                                          static_cast<s32>( lastChildIndex ) ) );
            Array<String> visited0 = visited;
            evaluatePoseTree( stateId, node->childNodeIds[i0], normalizedTimeValue,
                              weight * ( 1.0f - blendWeight ), outSamples, visited0 );
            Array<String> visited1 = visited;
            evaluatePoseTree( stateId, node->childNodeIds[i1], normalizedTimeValue, weight * blendWeight,
                              outSamples, visited1 );
            return;
        }
        case AnimationGraphPoseNodeType::Selector:
        {
            if( node->childNodeIds.empty() )
            {
                return;
            }
            const auto lastChildIndex = static_cast<u32>( node->childNodeIds.size() - 1 );
            u32 childIdx = 0;
            const auto isBool = m_boolParameters.find( node->parameter ) != m_boolParameters.end();
            if( isBool && node->childNodeIds.size() == 2 )
            {
                childIdx = getBoolParameter( node->parameter, false ) ? 1u : 0u;
            }
            else
            {
                const auto parameterValue = getFloatParameter( node->parameter, 0.0f );
                const auto rawIndex = static_cast<s32>( std::floor( parameterValue + 0.5f ) );
                childIdx =
                    static_cast<u32>( std::clamp( rawIndex, 0, static_cast<s32>( lastChildIndex ) ) );
            }
            Array<String> childVisited = visited;
            evaluatePoseTree( stateId, node->childNodeIds[childIdx], normalizedTimeValue, weight,
                              outSamples, childVisited );
            return;
        }
        default:
            return;
        }
    }

    void AnimationGraphInstance::samplePoseTreeEvents( const String &stateId, const String &nodeId,
                                                       f32 previousNormalized, f32 currentNormalized,
                                                       bool looped, Array<String> &visited )
    {
        if( visitedContains( visited, nodeId ) )
        {
            return;
        }
        visited.push_back( nodeId );
        const auto node = m_definition.findPoseNode( nodeId );
        if( !node )
        {
            return;
        }

        switch( node->type )
        {
        case AnimationGraphPoseNodeType::Clip:
        {
            const auto clip = m_definition.findClip( node->clipId );
            if( clip && clip->getDuration() > Epsilon )
            {
                for( const auto &sample :
                     clip->eventTimeline.sampleRange( previousNormalized, currentNormalized, looped ) )
                {
                    m_sampledEvents.push_back( { stateId, sample } );
                }
            }
            return;
        }
        case AnimationGraphPoseNodeType::Blend1D:
        case AnimationGraphPoseNodeType::Selector:
        {
            for( const auto &childId : node->childNodeIds )
            {
                Array<String> childVisited = visited;
                samplePoseTreeEvents( stateId, childId, previousNormalized, currentNormalized, looped,
                                      childVisited );
            }
            return;
        }
        default:
            return;
        }
    }
}  // namespace workphone::animation
