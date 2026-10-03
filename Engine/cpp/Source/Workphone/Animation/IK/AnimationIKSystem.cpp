#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <algorithm>
#include <cmath>

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
    }  // namespace

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
        *existing = constraint;
        return true;
    }

    bool AnimationIKSystem::removeConstraint( const String &id )
    {
        const auto previousSize = m_constraints.size();
        m_constraints.erase( std::remove_if( m_constraints.begin(), m_constraints.end(),
                                             [&id]( const TwoBoneIKConstraint &constraint ) {
                                                 return constraint.id == id;
                                             } ),
                             m_constraints.end() );
        return previousSize != m_constraints.size();
    }

    void AnimationIKSystem::clear()
    {
        m_constraints.clear();
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

        m_constraints = parsedSystem.m_constraints;
        return true;
    }

    Array<AnimationIKSolveResult> AnimationIKSystem::solve( ISkeleton *skeleton ) const
    {
        Array<AnimationIKSolveResult> results;
        if( !skeleton )
        {
            return results;
        }

        results.reserve( m_constraints.size() );
        for( const auto &constraint : m_constraints )
        {
            if( !constraint.enabled || !constraint.isValid() )
            {
                continue;
            }

            auto effector = skeleton->getBone( constraint.effectorBone );
            results.push_back(
                { constraint.id, TwoBoneIKSolver::solve( effector, constraint.targetTransform,
                                                         constraint.settings ) } );
        }
        return results;
    }
}  // namespace workphone::animation
