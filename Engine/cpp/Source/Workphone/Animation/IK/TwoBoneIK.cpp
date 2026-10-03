#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/IK/TwoBoneIK.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::animation
{
    namespace
    {
        using Vector = Vector3<real_Num>;
        using Rotation = Quaternion<real_Num>;
        using Transform = Transform3<real_Num>;

        constexpr auto SolverEpsilon = static_cast<real_Num>( 1.0e-5 );

        Vector lerp( const Vector &a, const Vector &b, real_Num weight )
        {
            return a + ( b - a ) * weight;
        }

        Transform blendTransform( const Transform &a, const Transform &b, real_Num weight )
        {
            Transform result;
            result.setPosition( lerp( a.getPosition(), b.getPosition(), weight ) );
            result.setScale( lerp( a.getScale(), b.getScale(), weight ) );
            result.setOrientation(
                Rotation::slerp( weight, a.getOrientation(), b.getOrientation(), true ) );
            return result;
        }

        Vector projectedDirection( const Vector &point, const Vector &origin, const Vector &axis )
        {
            auto direction = point - origin;
            direction -= axis * direction.dotProduct( axis );
            if( direction.lengthSquared() > SolverEpsilon * SolverEpsilon )
            {
                direction.normalise();
            }
            return direction;
        }

        Rotation normalized( const Rotation &rotation )
        {
            auto result = rotation;
            result.normalise();
            return result;
        }

        Transform localTransform( const SmartPtr<IBone> &bone )
        {
            auto mutableBone = bone;
            return Transform( mutableBone->getPosition(), mutableBone->getOrientation() );
        }
    }  // namespace

    TwoBoneIKResult TwoBoneIKSolver::solve( ChainTransforms &modelSpaceTransforms,
                                            const ChainTransforms &modelSpaceReferenceTransforms,
                                            const Transform3<real_Num> &requestedTargetTransform,
                                            const TwoBoneIKSettings &inputSettings )
    {
        TwoBoneIKResult result;
        auto settings = inputSettings;
        settings.blendWeight = std::clamp( settings.blendWeight, 0.0f, 1.0f );
        settings.chainRotationWeight = std::clamp( settings.chainRotationWeight, 0.0f, 1.0f );
        if( settings.blendWeight <= 0.0f )
        {
            result.success = true;
            result.status = TwoBoneIKStatus::Solved;
            return result;
        }

        const auto originalTransforms = modelSpaceTransforms;
        auto targetTransform = requestedTargetTransform;
        if( settings.blendMode == IKBlendMode::Effector && settings.blendWeight < 1.0f )
        {
            targetTransform =
                blendTransform( originalTransforms[2], requestedTargetTransform, settings.blendWeight );
        }

        const auto rootPosition = originalTransforms[0].getPosition();
        const auto originalMidPosition = originalTransforms[1].getPosition();
        const auto originalEffectorPosition = originalTransforms[2].getPosition();
        const auto firstSegment = originalMidPosition - rootPosition;
        const auto secondSegment = originalEffectorPosition - originalMidPosition;
        const auto firstLength = firstSegment.length();
        const auto secondLength = secondSegment.length();
        if( firstLength <= SolverEpsilon || secondLength <= SolverEpsilon )
        {
            result.status = TwoBoneIKStatus::DegenerateChain;
            return result;
        }

        auto targetVector = targetTransform.getPosition() - rootPosition;
        auto requestedDistance = targetVector.length();
        Vector targetDirection;
        if( requestedDistance > SolverEpsilon )
        {
            targetDirection = targetVector / requestedDistance;
        }
        else
        {
            targetDirection = originalEffectorPosition - rootPosition;
            if( targetDirection.lengthSquared() <= SolverEpsilon * SolverEpsilon )
            {
                targetDirection = firstSegment;
            }
            targetDirection.normalise();
        }

        const auto minimumReach = std::abs( firstLength - secondLength );
        const auto maximumReach = firstLength + secondLength;
        const auto solveDistance =
            std::clamp( requestedDistance, minimumReach + SolverEpsilon,
                        std::max( minimumReach + SolverEpsilon, maximumReach - SolverEpsilon ) );
        result.targetClamped = std::abs( solveDistance - requestedDistance ) > SolverEpsilon;

        auto poleDirection = projectedDirection( settings.poleTarget, rootPosition, targetDirection );
        auto currentBendDirection =
            projectedDirection( originalMidPosition, rootPosition, targetDirection );
        auto referenceBendDirection =
            projectedDirection( modelSpaceReferenceTransforms[1].getPosition(),
                                modelSpaceReferenceTransforms[0].getPosition(), targetDirection );

        if( poleDirection.lengthSquared() <= SolverEpsilon * SolverEpsilon )
        {
            poleDirection = currentBendDirection;
        }
        if( poleDirection.lengthSquared() <= SolverEpsilon * SolverEpsilon )
        {
            poleDirection = referenceBendDirection;
        }
        if( poleDirection.lengthSquared() <= SolverEpsilon * SolverEpsilon )
        {
            poleDirection = targetDirection.perpendicular();
        }

        auto bendDirection = poleDirection;
        if( settings.chainRotationWeight > 0.0f )
        {
            // Esoterica distributes target orientation through the chain by twisting the
            // bend plane about the root-to-effector axis. Express the reference hinge in
            // effector-local space, transform it by the requested target orientation and
            // rotate the pole-selected plane toward it. This preserves reach and segment
            // lengths while avoiding the visible wrist snap caused by applying all twist
            // to the effector.
            const auto referenceFirst = modelSpaceReferenceTransforms[1].getPosition() -
                                        modelSpaceReferenceTransforms[0].getPosition();
            const auto referenceSecond = modelSpaceReferenceTransforms[2].getPosition() -
                                         modelSpaceReferenceTransforms[1].getPosition();
            auto referenceHinge = referenceSecond.crossProduct( referenceFirst );
            if( referenceHinge.lengthSquared() > SolverEpsilon * SolverEpsilon )
            {
                referenceHinge.normalise();
                const auto referenceHingeLocal =
                    modelSpaceReferenceTransforms[2].getOrientation().inverse() * referenceHinge;
                auto desiredHinge = targetTransform.getOrientation() * referenceHingeLocal;
                desiredHinge -= targetDirection * desiredHinge.dotProduct( targetDirection );

                if( desiredHinge.lengthSquared() > SolverEpsilon * SolverEpsilon )
                {
                    desiredHinge.normalise();
                    auto desiredBendDirection = desiredHinge.crossProduct( targetDirection );
                    desiredBendDirection.normalise();

                    const auto cosine =
                        std::clamp( bendDirection.dotProduct( desiredBendDirection ),
                                    static_cast<real_Num>( -1.0 ), static_cast<real_Num>( 1.0 ) );
                    const auto sine =
                        targetDirection.dotProduct( bendDirection.crossProduct( desiredBendDirection ) );
                    const auto twistAngle = static_cast<real_Num>( std::atan2( sine, cosine ) ) *
                                            static_cast<real_Num>( settings.chainRotationWeight );
                    bendDirection = Rotation::angleAxis( twistAngle, targetDirection ) * bendDirection;
                    bendDirection.normalise();
                }
            }
            else if( referenceBendDirection.lengthSquared() > SolverEpsilon * SolverEpsilon )
            {
                bendDirection = lerp( poleDirection, referenceBendDirection,
                                      static_cast<real_Num>( settings.chainRotationWeight ) );
                if( bendDirection.lengthSquared() <= SolverEpsilon * SolverEpsilon )
                {
                    bendDirection = poleDirection;
                }
                bendDirection.normalise();
            }
        }

        const auto distanceAlongTarget =
            ( firstLength * firstLength - secondLength * secondLength + solveDistance * solveDistance ) /
            ( static_cast<real_Num>( 2.0 ) * solveDistance );
        const auto bendHeightSquared =
            std::max( static_cast<real_Num>( 0.0 ),
                      firstLength * firstLength - distanceAlongTarget * distanceAlongTarget );
        const auto bendHeight = static_cast<real_Num>( std::sqrt( bendHeightSquared ) );
        const auto solvedMidPosition =
            rootPosition + targetDirection * distanceAlongTarget + bendDirection * bendHeight;
        const auto solvedEffectorPosition = rootPosition + targetDirection * solveDistance;

        const auto solvedFirstSegment = solvedMidPosition - rootPosition;
        const auto solvedSecondSegment = solvedEffectorPosition - solvedMidPosition;
        const auto rootDelta = Rotation::getRotationTo( firstSegment, solvedFirstSegment );
        const auto propagatedSecondSegment = rootDelta * secondSegment;
        const auto midDelta = Rotation::getRotationTo( propagatedSecondSegment, solvedSecondSegment );

        modelSpaceTransforms[0].setPosition( rootPosition );
        modelSpaceTransforms[0].setOrientation(
            normalized( rootDelta * originalTransforms[0].getOrientation() ) );
        modelSpaceTransforms[1].setPosition( solvedMidPosition );
        modelSpaceTransforms[1].setOrientation(
            normalized( midDelta * rootDelta * originalTransforms[1].getOrientation() ) );
        modelSpaceTransforms[2].setPosition( solvedEffectorPosition );
        if( settings.matchTargetOrientation )
        {
            modelSpaceTransforms[2].setOrientation( targetTransform.getOrientation() );
        }
        else
        {
            modelSpaceTransforms[2].setOrientation(
                normalized( midDelta * rootDelta * originalTransforms[2].getOrientation() ) );
        }

        if( settings.blendMode == IKBlendMode::Pose && settings.blendWeight < 1.0f )
        {
            for( size_t i = 0; i < modelSpaceTransforms.size(); ++i )
            {
                modelSpaceTransforms[i] =
                    blendTransform( originalTransforms[i], modelSpaceTransforms[i],
                                    static_cast<real_Num>( settings.blendWeight ) );
            }
        }

        result.success = true;
        result.status = TwoBoneIKStatus::Solved;
        result.effectorError = static_cast<f32>(
            ( modelSpaceTransforms[2].getPosition() - requestedTargetTransform.getPosition() )
                .length() );
        return result;
    }

    TwoBoneIKResult TwoBoneIKSolver::solve( SmartPtr<IBone> effectorBone,
                                            const Transform3<real_Num> &targetTransform,
                                            const TwoBoneIKSettings &settings )
    {
        if( !effectorBone )
        {
            TwoBoneIKResult result;
            result.status = TwoBoneIKStatus::InvalidEffector;
            return result;
        }

        auto effector = effectorBone;
        auto mid = effector->getParent();
        auto root = mid ? mid->getParent() : SmartPtr<IBone>();
        if( !root || !mid )
        {
            TwoBoneIKResult result;
            result.status = TwoBoneIKStatus::InvalidChain;
            return result;
        }

        ChainTransforms transforms;
        if( !getModelSpaceTransform( root, transforms[0] ) ||
            !getModelSpaceTransform( mid, transforms[1] ) ||
            !getModelSpaceTransform( effector, transforms[2] ) )
        {
            TwoBoneIKResult result;
            result.status = TwoBoneIKStatus::InvalidChain;
            return result;
        }

        const auto referenceTransforms = transforms;
        const auto result = solve( transforms, referenceTransforms, targetTransform, settings );
        if( !result.success )
        {
            return result;
        }

        Transform parentModelTransform;
        auto rootParent = root->getParent();
        if( rootParent && !getModelSpaceTransform( rootParent, parentModelTransform ) )
        {
            TwoBoneIKResult invalidResult;
            invalidResult.status = TwoBoneIKStatus::InvalidChain;
            return invalidResult;
        }

        const auto rootLocalOrientation =
            rootParent ? parentModelTransform.getOrientation().inverse() * transforms[0].getOrientation()
                       : transforms[0].getOrientation();
        const auto midLocalOrientation =
            transforms[0].getOrientation().inverse() * transforms[1].getOrientation();
        const auto effectorLocalOrientation =
            transforms[1].getOrientation().inverse() * transforms[2].getOrientation();

        root->setOrientation( normalized( rootLocalOrientation ) );
        mid->setOrientation( normalized( midLocalOrientation ) );
        effector->setOrientation( normalized( effectorLocalOrientation ) );
        return result;
    }

    bool TwoBoneIKSolver::getModelSpaceTransform( SmartPtr<IBone> bone, Transform3<real_Num> &transform )
    {
        if( !bone )
        {
            return false;
        }

        Array<SmartPtr<IBone>> hierarchy;
        for( auto current = bone; current; )
        {
            hierarchy.push_back( current );
            auto mutableCurrent = current;
            current = mutableCurrent->getParent();
        }

        Transform modelTransform;
        for( auto iterator = hierarchy.rbegin(); iterator != hierarchy.rend(); ++iterator )
        {
            const auto local = localTransform( *iterator );
            Transform combined;
            combined.transformFromParent( modelTransform, local );
            modelTransform = combined;
        }
        transform = modelTransform;
        return true;
    }
}  // namespace workphone::animation
