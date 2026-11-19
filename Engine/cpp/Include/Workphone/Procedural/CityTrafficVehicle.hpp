#pragma once

#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Mesh/Geometry.hpp>
#include <Workphone/Procedural/CityTrafficAppearance.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/FourWheelVehicle.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene::race::cityTrafficVehicle
{
    inline constexpr f32 wheelRadius = .34f;
    inline constexpr f32 halfWheelbase = 1.27f;
    inline constexpr f32 maxSteeringDegrees = 34.f;

    inline QuaternionF visualOrientation()
    {
        // The shared shell faces +Z and the native controller travels along -Z.
        return QuaternionF::eulerDegrees( 0, 180, 0 );
    }

    inline QuaternionF orientation( const Vector3F &forward )
    {
        return cityTrafficAppearance::roadOrientation( forward ) * visualOrientation();
    }

    inline Vector3F wheelPosition( u32 index )
    {
        return { index % 2 == 0 ? -.88f : .88f, -.43f,
                 index < 2 ? -halfWheelbase : halfWheelbase };
    }

    inline void wheelMeshes( Geometry &tire, Geometry &hub )
    {
        constexpr int facets = 12;
        constexpr f32 tau = 6.283185307179586f;
        for( int i = 0; i < facets; ++i )
        {
            const f32 a = tau * i / facets;
            const f32 b = tau * ( i + 1 ) / facets;
            const auto ring = []( f32 x, f32 angle, f32 radius ) {
                return Vector3F( x, std::sin( angle ) * radius, std::cos( angle ) * radius );
            };
            const auto middle = ( a + b ) * .5f;
            tire.quad( ring( -.11f, a, wheelRadius ), ring( -.11f, b, wheelRadius ),
                       ring( .11f, b, wheelRadius ), ring( .11f, a, wheelRadius ),
                       { 0, std::sin( middle ), std::cos( middle ) } );
            for( f32 side : { -1.f, 1.f } )
            {
                const f32 outer = side * .11f;
                const f32 metal = side * .116f;
                const Vector3F normal( side, 0, 0 );
                tire.quad( { outer, 0, 0 }, ring( outer, a, wheelRadius ),
                           ring( outer, b, wheelRadius ), { outer, 0, 0 }, normal );
                hub.quad( { metal, 0, 0 }, ring( metal, a, .19f ), ring( metal, b, .19f ),
                          { metal, 0, 0 }, normal );
            }
        }
    }

    inline void configure( SmartPtr<IGameActor> actor, u32 style )
    {
        auto collider = actor->addComponent<CollisionBox>();
        auto chassis = actor->addComponent<Rigidbody>();
        if( !collider || !chassis )
        {
            throw std::runtime_error( "Cannot create city traffic chassis." );
        }
        const f32 length = cityTrafficAppearance::lengths[style] - .16f;
        const f32 height = style == 2 ? 1.50f : 1.30f;
        // Keep the wheel rays outside the body shell so they hit the road.
        constexpr f32 width = 1.70f;
        collider->setExtents( { width, height, length } );
        collider->setPosition( { 0, height * .5f - .41f, 0 } );
        chassis->setUseGravity( true );
        auto app = core::IApplicationManager::instancePtr();
        chassis->setKinematic( !app || !app->isPlaying() );

        auto controller = make_ptr<FourWheelVehicle>();
        actor->addComponentInstance( controller );
        auto settings = controller->getProperties();
        auto &preset = settings->getPropertyObject( FourWheelVehicle::handlingPresetStr );
        preset.setValue( "1" );  // FWD Hot Hatch road handling
        preset.setAttribute( "applyHandlingPreset", "true" );
        settings->setProperty( FourWheelVehicle::authoredGeometryStr, true );
        settings->setProperty( FourWheelVehicle::radiusStr, wheelRadius );
        settings->setProperty( FourWheelVehicle::suspensionTravelStr, .18f );
        constexpr f32 massPerAxis = 1200.f / 12.f;
        settings->setProperty( VehicleController::moiStr,
            Vector3F( massPerAxis * ( height * height + length * length ),
                      massPerAxis * ( width * width + length * length ),
                      massPerAxis * ( width * width + height * height ) ) );
        controller->setProperties( settings );
        controller->setControls( 0, 1, 0 );
        controller->load( nullptr );
        auto native = controller->getVehicleController();
        if( !controller->isLoaded() || !native || !native->isLoaded() || !native->getDriveTrain() )
        {
            throw std::runtime_error( "Cannot load the city traffic FourWheelVehicle controller." );
        }
        native->setDisplayDebugData( false );
        controller->refreshWheels();
    }

    inline bool sampleMotion( const SmartPtr<IGameActor> &actor, Vector3F &position, f32 &speed )
    {
        auto chassis = actor ? actor->getComponent<Rigidbody>() : nullptr;
        auto body = chassis ? chassis->getRigidDynamic() : nullptr;
        if( !body || !body->getScene() )
        {
            return false;
        }
        position = body->getTransform().getPosition();
        auto velocity = body->getLinearVelocity();
        speed = std::hypot( velocity.x, velocity.z );
        return position.isFinite() && std::isfinite( speed );
    }

    inline void stopTrafficVehicle( const SmartPtr<IGameActor> &actor )
    {
        if( auto controller = actor ? actor->getComponent<FourWheelVehicle>() : nullptr )
        {
            controller->setControls( 0, 1, 0 );
            auto chassis = controller->getChassis();
            auto body = chassis ? chassis->getRigidDynamic() : nullptr;
            if( body && !chassis->isKinematic() )
            {
                chassis->setKinematic( true );
                body->setLinearVelocity( Vector3F::zero() );
                body->setAngularVelocity( Vector3F::zero() );
                body->clearForce();
                body->clearTorque();
            }
        }
    }

    inline void driveToward( const SmartPtr<IGameActor> &actor, const Vector3F &worldTarget,
                             f32 targetSpeed )
    {
        auto controller = actor ? actor->getComponent<FourWheelVehicle>() : nullptr;
        auto chassis = controller ? controller->getChassis() : nullptr;
        auto body = chassis ? chassis->getRigidDynamic() : nullptr;
        if( !body || !body->getScene() )
        {
            return;
        }
        if( chassis->isKinematic() )
        {
            chassis->setKinematic( false );
            body->wakeUp();
        }

        const auto transform = body->getTransform();
        auto target = transform.inverseTransformPoint( worldTarget );
        const auto localVelocity = transform.inverseTransformVector( body->getLinearVelocity() );
        if( !target.isFinite() || !localVelocity.isFinite() || !std::isfinite( targetSpeed ) )
        {
            controller->setControls( 0, 1, 0 );
            return;
        }

        // A collision can leave the car facing away from the lane. Brake and
        // let the route's stall recovery return it to a clear spawn.
        if( target.z >= -.25f )
        {
            controller->setControls( 0, 1, 0 );
            return;
        }

        const f32 distanceSquared = target.x * target.x + target.z * target.z;
        const f32 curvature = 2.f * target.x / std::max( distanceSquared, .20f );
        const f32 maxSteer = MathF::DegToRad( maxSteeringDegrees );
        const f32 steering = std::clamp( std::atan( 2.f * halfWheelbase * curvature ) / maxSteer,
                                         -1.f, 1.f );
        const f32 cornerSpeed = std::sqrt( 3.5f / std::max( std::abs( curvature ), .01f ) );
        const f32 speed = std::min( std::max( targetSpeed, 0.f ), cornerSpeed );
        const f32 error = speed + localVelocity.z;
        const f32 throttle = speed > .05f ? std::clamp( error * .40f, 0.f, 1.f ) : 0.f;
        const f32 brake = speed > .05f ? std::clamp( -error * .42f, 0.f, 1.f ) : 1.f;
        controller->setControls( throttle, brake, steering );
    }

    inline void resetTrafficVehicle( SmartPtr<IGameActor> actor, const Vector3F &roadPosition,
                                     const Vector3F &forward )
    {
        auto controller = actor ? actor->getComponent<FourWheelVehicle>() : nullptr;
        auto chassis = controller ? controller->getChassis() : nullptr;
        auto body = chassis ? chassis->getRigidDynamic() : nullptr;
        if( !body || !roadPosition.isFinite() || !forward.isFinite() )
        {
            return;
        }
        controller->setControls( 0, 1, 0 );
        if( auto native = controller->getVehicleController() )
        {
            if( auto drivetrain = native->getDriveTrain() )
            {
                drivetrain->setGear( 2 );
            }
        }
        const auto position = roadPosition + Vector3F( 0, cityTrafficAppearance::chassisLift, 0 );
        const auto rotation = orientation( forward );
        actor->setPosition( position );
        actor->setOrientation( rotation );
        actor->updateTransform();
        body->setTransform( Transform3F( position, rotation ) );
        body->setLinearVelocity( Vector3F::zero() );
        body->setAngularVelocity( Vector3F::zero() );
        body->clearForce();
        body->clearTorque();
    }
}
