#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace workphone::animation
{
    namespace
    {
        const String ikConstraintsStr( "ikConstraints" );
        const String constraintStr( "constraint" );
        const String constraintCountStr( "constraintCount" );
        const String idStr( "id" );
        const String effectorBoneStr( "effectorBone" );
        const String targetTransformStr( "targetTransform" );
        const String poleTargetStr( "poleTarget" );
        const String chainRotationWeightStr( "chainRotationWeight" );
        const String blendWeightStr( "blendWeight" );
        const String blendModeStr( "blendMode" );
        const String matchTargetOrientationStr( "matchTargetOrientation" );
        const String enabledStr( "enabled" );

        bool hierarchyDepth( SmartPtr<IBone> bone, size_t &depth )
        {
            auto slow = bone;
            auto fast = bone;
            while( fast )
            {
                fast = fast->getParent();
                if( fast )
                {
                    fast = fast->getParent();
                }
                slow = slow ? slow->getParent() : SmartPtr<IBone>();
                if( fast && fast == slow )
                {
                    return false;
                }
            }
            depth = 0;
            for( auto current = bone; current; current = current->getParent() )
            {
                ++depth;
            }
            return true;
        }
    }  // namespace

    // Transform3/Vector3 retain legacy non-noexcept declarations, but assign
    // only scalar values. The owning strings must move without admission.
    static_assert( std::is_nothrow_move_assignable_v<String> );

    bool TwoBoneIKConstraint::isValid() const
    {
        return !id.empty() && !effectorBone.empty() && targetTransform.isValid() &&
               settings.poleTarget.isFinite() && std::isfinite( settings.blendWeight ) &&
               settings.blendWeight >= 0.0f && settings.blendWeight <= 1.0f &&
               std::isfinite( settings.chainRotationWeight ) && settings.chainRotationWeight >= 0.0f &&
               settings.chainRotationWeight <= 1.0f;
    }

    bool AnimationIKSystem::addConstraint( const TwoBoneIKConstraint &constraint )
    {
        if( !constraint.isValid() || findConstraint( constraint.id ) )
        {
            return false;
        }
        m_constraints.push_back( constraint );
        return true;
    }

    bool AnimationIKSystem::updateConstraint( const TwoBoneIKConstraint &constraint )
    {
        auto existing = findConstraint( constraint.id );
        if( !existing || !constraint.isValid() )
        {
            return false;
        }
        TwoBoneIKConstraint candidate( constraint );
        *existing = std::move( candidate );
        return true;
    }

    bool AnimationIKSystem::removeConstraint( const String &id )
    {
        const auto found = std::find_if(
            m_constraints.begin(), m_constraints.end(),
            [&id]( const TwoBoneIKConstraint &constraint ) { return constraint.id == id; } );
        if( found == m_constraints.end() )
        {
            return false;
        }
        std::move( found + 1, m_constraints.end(), found );
        m_constraints.pop_back();
        return true;
    }

    void AnimationIKSystem::clear()
    {
        m_constraints = makeSourceBudgetSnapshot<TwoBoneIKConstraint>();
    }

    TwoBoneIKConstraint *AnimationIKSystem::findConstraint( const String &id )
    {
        const auto found = std::find_if(
            m_constraints.begin(), m_constraints.end(),
            [&id]( const TwoBoneIKConstraint &constraint ) { return constraint.id == id; } );
        return found != m_constraints.end() ? &( *found ) : nullptr;
    }

    const TwoBoneIKConstraint *AnimationIKSystem::findConstraint( const String &id ) const
    {
        const auto found = std::find_if(
            m_constraints.begin(), m_constraints.end(),
            [&id]( const TwoBoneIKConstraint &constraint ) { return constraint.id == id; } );
        return found != m_constraints.end() ? &( *found ) : nullptr;
    }

    const Array<TwoBoneIKConstraint> &AnimationIKSystem::getConstraints() const
    {
        return m_constraints;
    }

    bool AnimationIKSystem::setTarget( const String &id, const Transform3<real_Num> &targetTransform )
    {
        auto constraint = findConstraint( id );
        if( !constraint || !targetTransform.isValid() )
        {
            return false;
        }
        constraint->targetTransform = targetTransform;
        return true;
    }

    bool AnimationIKSystem::setPoleTarget( const String &id, const Vector3<real_Num> &poleTarget )
    {
        auto constraint = findConstraint( id );
        if( !constraint || !poleTarget.isFinite() )
        {
            return false;
        }
        constraint->settings.poleTarget = poleTarget;
        return true;
    }

    bool AnimationIKSystem::setWeight( const String &id, f32 weight )
    {
        auto constraint = findConstraint( id );
        if( !constraint || !std::isfinite( weight ) )
        {
            return false;
        }
        constraint->settings.blendWeight = std::clamp( weight, 0.0f, 1.0f );
        return true;
    }

    SmartPtr<Properties> AnimationIKSystem::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        properties->setName( ikConstraintsStr );
        properties->setProperty( constraintCountStr, static_cast<u32>( m_constraints.size() ), true );

        for( const auto &constraint : m_constraints )
        {
            auto child = workphone::make_ptr<Properties>();
            child->setName( constraintStr );
            child->setProperty( idStr, constraint.id );
            child->setProperty( effectorBoneStr, constraint.effectorBone );
            child->setProperty( targetTransformStr, constraint.targetTransform );
            child->setProperty( poleTargetStr, constraint.settings.poleTarget );
            child->setProperty( chainRotationWeightStr, constraint.settings.chainRotationWeight );
            child->setProperty( blendWeightStr, constraint.settings.blendWeight );
            child->setProperty( blendModeStr, static_cast<u32>( constraint.settings.blendMode ) );
            child->setProperty( matchTargetOrientationStr, constraint.settings.matchTargetOrientation );
            child->setProperty( enabledStr, constraint.enabled );
            properties->addChild( child );
        }

        return properties;
    }

    bool AnimationIKSystem::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return false;
        }

        AnimationIKSystem parsedSystem;
        for( const auto &child : properties->getChildrenByName( constraintStr ) )
        {
            if( !child )
            {
                continue;
            }

            TwoBoneIKConstraint constraint;
            u32 blendMode = static_cast<u32>( IKBlendMode::Effector );
            child->getPropertyValue( idStr, constraint.id );
            child->getPropertyValue( effectorBoneStr, constraint.effectorBone );
            child->getPropertyValue( targetTransformStr, constraint.targetTransform );
            child->getPropertyValue( poleTargetStr, constraint.settings.poleTarget );
            child->getPropertyValue( chainRotationWeightStr, constraint.settings.chainRotationWeight );
            child->getPropertyValue( blendWeightStr, constraint.settings.blendWeight );
            child->getPropertyValue( blendModeStr, blendMode );
            child->getPropertyValue( matchTargetOrientationStr,
                                     constraint.settings.matchTargetOrientation );
            child->getPropertyValue( enabledStr, constraint.enabled );

            if( blendMode > static_cast<u32>( IKBlendMode::Pose ) )
            {
                return false;
            }
            constraint.settings.blendMode = static_cast<IKBlendMode>( blendMode );
            if( !parsedSystem.addConstraint( constraint ) )
            {
                return false;
            }
        }

        m_constraints.swap( parsedSystem.m_constraints );
        return true;
    }

    Array<AnimationIKSolveResult> AnimationIKSystem::solve( ISkeleton *skeleton ) const
    {
        auto prepared = prepareSolve( skeleton );
        solvePrepared( prepared );
        return prepared.takeResults();
    }

    const Array<AnimationIKSolveResult> &AnimationIKSystem::PreparedSolve::getResults() const
    {
        return m_results;
    }

    Array<AnimationIKSolveResult> AnimationIKSystem::PreparedSolve::takeResults() noexcept
    {
        auto results = std::move( m_results );
        m_commands = makeSourceBudgetSnapshot<Command>();
        m_hierarchy = makeSourceBudgetSnapshot<SmartPtr<IBone>>();
        return results;
    }

    AnimationIKSystem::PreparedSolve AnimationIKSystem::prepareSolve( ISkeleton *skeleton ) const
    {
        PreparedSolve prepared;
        if( !skeleton )
        {
            return prepared;
        }
        prepared.m_results.reserve( m_constraints.size() );
        prepared.m_commands.reserve( m_constraints.size() );
        size_t maximumDepth = 0;
        for( const auto &constraint : m_constraints )
        {
            if( !constraint.enabled || !constraint.isValid() )
            {
                continue;
            }
            auto effector = skeleton->getBone( constraint.effectorBone );
            // IDs can own nonpooled character buffers. Capture every ID and
            // command before the first solver is allowed to publish bone data.
            prepared.m_results.push_back( { constraint.id, {} } );
            prepared.m_commands.push_back(
                { effector, constraint.targetTransform, constraint.settings } );
            size_t depth = 0;
            if( !hierarchyDepth( effector, depth ) )
            {
                prepared.m_results.back().result.status = TwoBoneIKStatus::InvalidChain;
            }
            else
            {
                maximumDepth = std::max( maximumDepth, depth );
            }
        }
        prepared.m_hierarchy.reserve( maximumDepth );
        return prepared;
    }

    void AnimationIKSystem::solvePrepared( PreparedSolve &prepared ) const
    {
        // Native hierarchies can be edited between prepare and execution. Any
        // resulting scratch growth must still finish before the first write.
        size_t maximumDepth = 0;
        for( const auto &command : prepared.m_commands )
        {
            size_t depth = 0;
            if( hierarchyDepth( command.effector, depth ) )
            {
                maximumDepth = std::max( maximumDepth, depth );
            }
        }
        prepared.m_hierarchy.reserve( maximumDepth );
        for( size_t i = 0; i < prepared.m_commands.size(); ++i )
        {
            auto &result = prepared.m_results[i].result;
            const auto &command = prepared.m_commands[i];
            result = TwoBoneIKSolver::solve( command.effector, command.target, command.settings,
                                             prepared.m_hierarchy );
        }
    }
}  // namespace workphone::animation
