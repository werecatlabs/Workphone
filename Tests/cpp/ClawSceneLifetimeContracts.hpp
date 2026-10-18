#pragma once

#include "ClawFoliageSceneContracts.hpp"

namespace claw_scene_lifetime_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using claw_foliage_contracts::require;

    inline bool run()
    {
        try
        {
            claw_foliage_scene_contracts::Fixture fixture;
            auto manager = fixture.services.states;
            const auto baseline = manager->getStateContexts().size();
            for( int mode = 0; mode < 3; ++mode )
            {
                auto native = mode == 1 ? wp_graphics_scene_create() : nullptr;
                auto scene = mode == 1 ? make_ptr<ClawScene>( native, false ) : make_ptr<ClawScene>();
                require( manager->getStateContexts().size() == baseline + 5,
                         "each constructor must register exactly one set of five scene contexts" );
                Array<SmartPtr<IStateContext>> retained;
                for( auto &context : manager->getStateContexts() )
                    if( context->getOwnerPtr() == scene.get() )
                        retained.push_back( context );
                require( retained.size() == 5, "all scene contexts must name the scene owner" );
                auto listener = scene->getStateListener();
                auto typed = static_cast<GraphicsScene::StateListener *>( listener.get() );
                require( typed && typed->getOwnerPtr() == scene.get(), "scene listener must have its owner" );
                if( mode == 2 )
                {
                    scene->unload( nullptr );
                    scene->unload( nullptr );
                }
                scene = nullptr;
                require( manager->getStateContexts().size() == baseline,
                         "destruction and repeated unload must retire every scene context" );
                require( typed->getOwnerPtr() == nullptr,
                         "a retained scene listener must detach before its owner is freed" );
                for( auto &context : retained )
                    require( !context->isLoaded() && !context->getOwnerPtr() &&
                                 context->getStates().empty() && context->getStateListeners().empty(),
                             "retained contexts must be unloaded without dangling owners or listeners" );
                if( native )
                {
                    require( wp_graphics_scene_get_root_node( native ) != nullptr,
                             "retiring a wrapper must preserve its borrowed native scene" );
                    wp_graphics_scene_destroy( native );
                }
            }

            // A foreign-owned shared primary context keeps its other owner's state.
            auto scene = make_ptr<ClawScene>();
            auto shared = scene->getStateContext();
            auto foreign = make_ptr<State>();
            auto sentinel = make_ptr<State>();
            sentinel->setId( 0x7fe51234u );
            sentinel->setOwner( foreign );
            shared->setOwner( foreign );
            shared->addState( sentinel );
            auto listener = scene->getStateListener();
            scene = nullptr;
            require( manager->getStateContexts().size() == baseline + 1 && shared->isLoaded() &&
                         shared->getOwnerPtr() == foreign.get() && shared->getStates().size() == 1 &&
                         shared->getStates()[0] == sentinel && shared->getStateListeners().empty(),
                     "scene retirement must preserve a shared context and its foreign state" );
            require( static_cast<GraphicsScene::StateListener *>( listener.get() )->getOwnerPtr() == nullptr,
                     "shared-context retirement must also detach the scene listener" );
            manager->removeStateContext( shared );
            require( manager->getStateContexts().size() == baseline, "scene lifetime fixture must drain" );
            std::puts( "Scene context and retained-listener lifetime contracts: PASS" );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "FAIL scene lifetime: %s\n", error.what() );
            return false;
        }
    }
}
