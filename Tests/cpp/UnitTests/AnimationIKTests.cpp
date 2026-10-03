#include "UnitTests.hpp"
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>
#include <Workphone/Animation/IK/TwoBoneIK.hpp>
#include <Workphone/Core/Properties.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <cmath>

using namespace workphone;
using namespace workphone::animation;

namespace
{
    Transform3<real_Num> makeTransform( real_Num x, real_Num y, real_Num z )
    {
        return Transform3<real_Num>( Vector3<real_Num>( x, y, z ), Quaternion<real_Num>::identity() );
    }

    TwoBoneIKSolver::ChainTransforms makeStraightChain()
    {
        return { makeTransform( 0.0f, 0.0f, 0.0f ), makeTransform( 1.0f, 0.0f, 0.0f ),
                 makeTransform( 2.0f, 0.0f, 0.0f ) };
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( AnimationIKTests )

BOOST_AUTO_TEST_CASE( two_bone_solver_reaches_target_and_preserves_lengths )
{
    auto chain = makeStraightChain();
    auto referenceChain = chain;
    referenceChain[1].setPosition( Vector3<real_Num>( 1.0f, 0.0f, 0.1f ) );

    TwoBoneIKSettings settings;
    settings.poleTarget = Vector3<real_Num>( 0.0f, 0.0f, 1.0f );
    settings.matchTargetOrientation = false;
    const auto result =
        TwoBoneIKSolver::solve( chain, referenceChain, makeTransform( 1.0f, 1.0f, 0.0f ), settings );

    BOOST_REQUIRE( result.success );
    BOOST_CHECK( !result.targetClamped );
    BOOST_CHECK_SMALL( result.effectorError, 0.001f );
    BOOST_CHECK_CLOSE( ( chain[1].getPosition() - chain[0].getPosition() ).length(), 1.0f, 0.01f );
    BOOST_CHECK_CLOSE( ( chain[2].getPosition() - chain[1].getPosition() ).length(), 1.0f, 0.01f );
    BOOST_CHECK_GT( chain[1].getPosition().Z(), 0.0f );
}

BOOST_AUTO_TEST_CASE( two_bone_solver_clamps_unreachable_targets )
{
    auto chain = makeStraightChain();
    const auto referenceChain = chain;
    TwoBoneIKSettings settings;
    settings.poleTarget = Vector3<real_Num>( 0.0f, 1.0f, 0.0f );

    const auto result =
        TwoBoneIKSolver::solve( chain, referenceChain, makeTransform( 4.0f, 0.0f, 0.0f ), settings );

    BOOST_REQUIRE( result.success );
    BOOST_CHECK( result.targetClamped );
    BOOST_CHECK_CLOSE( chain[2].getPosition().length(), 2.0f, 0.01f );
    BOOST_CHECK_CLOSE( result.effectorError, 2.0f, 0.01f );
}

BOOST_AUTO_TEST_CASE( two_bone_solver_supports_effector_and_pose_blending )
{
    auto effectorBlendChain = makeStraightChain();
    const auto referenceChain = effectorBlendChain;
    TwoBoneIKSettings settings;
    settings.poleTarget = Vector3<real_Num>( 0.0f, 0.0f, 1.0f );
    settings.blendWeight = 0.5f;
    settings.blendMode = IKBlendMode::Effector;
    auto result = TwoBoneIKSolver::solve( effectorBlendChain, referenceChain,
                                          makeTransform( 0.0f, 2.0f, 0.0f ), settings );
    BOOST_REQUIRE( result.success );
    BOOST_CHECK_CLOSE( effectorBlendChain[2].getPosition().X(), 1.0f, 0.01f );
    BOOST_CHECK_CLOSE( effectorBlendChain[2].getPosition().Y(), 1.0f, 0.01f );

    auto poseBlendChain = makeStraightChain();
    settings.blendMode = IKBlendMode::Pose;
    result = TwoBoneIKSolver::solve( poseBlendChain, referenceChain, makeTransform( 0.0f, 2.0f, 0.0f ),
                                     settings );
    BOOST_REQUIRE( result.success );
    BOOST_CHECK_CLOSE( poseBlendChain[2].getPosition().X(), 1.0f, 0.01f );
    BOOST_CHECK_CLOSE( poseBlendChain[2].getPosition().Y(), 1.0f, 0.01f );
}

BOOST_AUTO_TEST_CASE( ik_system_manages_named_constraints )
{
    AnimationIKSystem system;
    TwoBoneIKConstraint constraint;
    constraint.id = "LeftHand";
    constraint.effectorBone = "hand_l";
    constraint.targetTransform = makeTransform( 0.5f, 1.0f, 0.0f );
    constraint.settings.poleTarget = Vector3<real_Num>( 0.0f, 0.0f, 1.0f );

    BOOST_REQUIRE( system.addConstraint( constraint ) );
    BOOST_CHECK( !system.addConstraint( constraint ) );
    BOOST_REQUIRE( system.setWeight( "LeftHand", 0.25f ) );
    BOOST_CHECK_CLOSE( system.findConstraint( "LeftHand" )->settings.blendWeight, 0.25f, 0.01f );
    BOOST_CHECK( system.setTarget( "LeftHand", makeTransform( 1.0f, 1.0f, 0.0f ) ) );
    BOOST_CHECK( system.removeConstraint( "LeftHand" ) );
    BOOST_CHECK( system.getConstraints().empty() );
}

BOOST_AUTO_TEST_CASE( two_bone_solver_distributes_target_twist_through_chain )
{
    auto referenceChain = makeStraightChain();
    referenceChain[1].setPosition( Vector3<real_Num>( 1.0f, 0.0f, 0.2f ) );

    auto target = makeTransform( 1.5f, 0.0f, 0.0f );
    target.setOrientation( Quaternion<real_Num>::angleAxis( static_cast<real_Num>( 1.57079632679 ),
                                                            Vector3<real_Num>::unitX() ) );

    TwoBoneIKSettings settings;
    settings.poleTarget = Vector3<real_Num>( 0.0f, 0.0f, 1.0f );
    settings.chainRotationWeight = 0.0f;
    auto poleOnlyChain = referenceChain;
    BOOST_REQUIRE( TwoBoneIKSolver::solve( poleOnlyChain, referenceChain, target, settings ).success );

    settings.chainRotationWeight = 1.0f;
    auto twistedChain = referenceChain;
    const auto result = TwoBoneIKSolver::solve( twistedChain, referenceChain, target, settings );
    BOOST_REQUIRE( result.success );
    BOOST_CHECK( result.status == TwoBoneIKStatus::Solved );
    BOOST_CHECK_GT( std::abs( twistedChain[1].getPosition().Y() ), 0.1f );
    BOOST_CHECK_LT( std::abs( twistedChain[1].getPosition().Z() ),
                    std::abs( poleOnlyChain[1].getPosition().Z() ) );
    BOOST_CHECK_SMALL( result.effectorError, 0.001f );
}

BOOST_AUTO_TEST_CASE( ik_constraints_round_trip_through_properties )
{
    AnimationIKSystem source;
    TwoBoneIKConstraint constraint;
    constraint.id = "RightFoot";
    constraint.effectorBone = "foot_r";
    constraint.targetTransform = makeTransform( 1.0f, 2.0f, 3.0f );
    constraint.settings.poleTarget = Vector3<real_Num>( 0.0f, 0.5f, 1.0f );
    constraint.settings.chainRotationWeight = 0.75f;
    constraint.settings.blendWeight = 0.4f;
    constraint.settings.blendMode = IKBlendMode::Pose;
    constraint.settings.matchTargetOrientation = false;
    constraint.enabled = false;
    BOOST_REQUIRE( source.addConstraint( constraint ) );

    AnimationIKSystem restored;
    BOOST_REQUIRE( restored.setProperties( source.getProperties() ) );
    BOOST_REQUIRE_EQUAL( restored.getConstraints().size(), 1 );
    const auto &roundTripped = restored.getConstraints().front();
    BOOST_CHECK_EQUAL( roundTripped.id, constraint.id );
    BOOST_CHECK_EQUAL( roundTripped.effectorBone, constraint.effectorBone );
    BOOST_CHECK( roundTripped.targetTransform == constraint.targetTransform );
    BOOST_CHECK( roundTripped.settings.poleTarget == constraint.settings.poleTarget );
    BOOST_CHECK_CLOSE( roundTripped.settings.chainRotationWeight, 0.75f, 0.01f );
    BOOST_CHECK_CLOSE( roundTripped.settings.blendWeight, 0.4f, 0.01f );
    BOOST_CHECK( roundTripped.settings.blendMode == IKBlendMode::Pose );
    BOOST_CHECK( !roundTripped.settings.matchTargetOrientation );
    BOOST_CHECK( !roundTripped.enabled );
}

BOOST_AUTO_TEST_SUITE_END()
