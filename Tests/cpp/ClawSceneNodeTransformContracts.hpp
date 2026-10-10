#pragma once

#include <WPGraphics/ClawSceneNode.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/TransformStateData.hpp>
#include <Workphone/System/StateContext.hpp>
#include <workphone_graphics_scenenode.h>
#include <cmath>
#include <cstdio>

namespace claw_scene_node_transform_contracts
{
    using namespace workphone;

    class Node : public render::ClawSceneNode
    {
    public:
        // Deterministically exercise worker then owner dispatch without starting a pool.
        bool ownerThread = false;
        bool isThreadSafe() const override
        {
            return ownerThread;
        }
    };

    class TransformState : public State
    {
    public:
        // The contract depends on pending state, not the application's clock or job queue.
        void setDirty( bool value ) override
        {
            dirty = value;
        }
        bool isDirty() const override
        {
            return dirty;
        }
        bool dirty = false;
    };

    inline bool check( bool condition, const char *message )
    {
        if( !condition )
            std::fprintf( stderr, "FAIL: %s\n", message );
        return condition;
    }

    inline bool matches( wp_scenenode *native, const Transform3<real_Num> &transform )
    {
        const auto position = wp_scenenode_get_position( native );
        const auto orientation = wp_scenenode_get_orientation( native );
        const auto scale = wp_scenenode_get_scale( native );
        const auto p = transform.getPosition();
        const auto q = transform.getOrientation();
        const auto s = transform.getScale();
        return std::abs( position.x - p.x ) < 1e-5f && std::abs( position.y - p.y ) < 1e-5f &&
               std::abs( position.z - p.z ) < 1e-5f && std::abs( orientation.w - q.w ) < 1e-5f &&
               std::abs( orientation.x - q.x ) < 1e-5f && std::abs( orientation.y - q.y ) < 1e-5f &&
               std::abs( orientation.z - q.z ) < 1e-5f && std::abs( scale.x - s.x ) < 1e-5f &&
               std::abs( scale.y - s.y ) < 1e-5f && std::abs( scale.z - s.z ) < 1e-5f;
    }

    inline bool run()
    {
        auto context = make_ptr<StateContext>();
        context->setLoadingState( LoadingState::Loaded );
        auto node = make_ptr<Node>();
        auto state = make_ptr<TransformState>();
        state->setId( node->getId() );
        state->setOwner( node );
        state->setData( make_ptr<TransformStateData>() );
        context->addState( state );
        node->setStateContext( context );
        node->load( nullptr );
        auto *native = node->getNativeNode();
        bool ok = check( native != nullptr, "transform fixture must create a native scene node" );
        if( native )
        {
            const auto initial = node->getTransform();
            auto desired = initial;
            desired.setPosition( { 7, -3, 2 } );
            desired.setOrientation( Quaternion<real_Num>::eulerDegrees( 15, 30, -20 ) );
            desired.setScale( { 2, 3, 4 } );

            node->setTransform( desired );
            ok &=
                check( node->getTransform() == desired && state->isDirty() && matches( native, initial ),
                       "worker update must change authored state while native dispatch is pending" );

            node->ownerThread = true;
            node->setTransform( desired );
            ok &= check( matches( native, desired ),
                         "owner must apply an equal queued transform to the native node" );
            node->setTransform( desired );
            ok &= check( matches( native, desired ),
                         "repeated owner transform must retain native position, orientation and scale" );

            desired.setPosition( { -2, 5, 11 } );
            node->setTransform( desired );
            ok &= check( node->getTransform() == desired && matches( native, desired ),
                         "changed owner transform must update both authored and native state" );
        }
        node->unload( nullptr );
        node->setStateContext( nullptr );
        context->removeState( state );
        state->setOwner( nullptr );
        context->unload( nullptr );
        if( ok )
            std::puts( "Queued scene-node transform and render-owner publication contracts passed." );
        return ok;
    }
}  // namespace claw_scene_node_transform_contracts
