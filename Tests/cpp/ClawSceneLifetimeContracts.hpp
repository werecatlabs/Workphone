#pragma once

#include "ClawFoliageSceneContracts.hpp"
#include <future>
#include <atomic>
#include <Workphone/Thread/ScopedLock.hpp>
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <WorkphoneGraphics/workphone_graphics_object.h>

namespace claw_scene_lifetime_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using claw_foliage_contracts::require;

    class CaptureGraphics : public claw_foliage_scene_contracts::FixtureGraphics
    {
    public:
        std::atomic<std::promise<void> *> lockAttempt{ nullptr };

        void lock() override
        {
            if( auto signal = lockAttempt.exchange( nullptr ) )
            {
                signal->set_value();
            }
            FixtureGraphics::lock();
        }
    };

    inline void creationDuringCapture()
    {
        claw_foliage_scene_contracts::Fixture fixture;
        auto graphics = make_ptr<CaptureGraphics>();
        graphics->setMaterials( fixture.materials );
        fixture.services.application->setGraphicsSystem( graphics );
        auto scene = make_ptr<ClawScene>();
        scene->load( nullptr );
        auto native = scene->getNativeScene();
        const auto before = wp_graphics_scene_get_object_count( native );
        std::promise<void> started;
        auto ready = started.get_future();
        std::future<SmartPtr<ISharedObject>> created;
        bool blocked, unchanged, captured;
        {
            // Model the graphics-system lock held through capture/submission.
            ScopedLock renderLock( graphics );
            graphics->lockAttempt = &started;
            created = std::async( std::launch::async, [&] {
                return scene->addGraphicsObjectByTypeId( IGraphicsMesh::typeInfo() );
            } );
            ready.wait();
            blocked = created.wait_for( std::chrono::milliseconds( 0 ) ) ==
                      std::future_status::timeout;
            unchanged = wp_graphics_scene_get_object_count( native ) == before;
            auto result = wp_graphics_scene_prepare_cull( native, nullptr, WP_GRAPHICS_CULL_PASS_MAIN );
            captured = result && wp_graphics_cull_result_get_candidate_count( result ) == before;
            wp_graphics_cull_result_destroy( result );
        }
        auto mesh = created.get();
        const bool published = mesh && wp_graphics_scene_get_object_count( native ) == before + 1;
        auto nativeMesh = dynamic_pointer_cast<ClawMesh>( mesh );
        auto root = scene->getRootSceneNode();
        bool attachmentBlocked = false;
        if( nativeMesh && root )
        {
            std::promise<void> attaching;
            auto attachmentReady = attaching.get_future();
            std::future<void> attached;
            {
                ScopedLock renderLock( graphics );
                graphics->lockAttempt = &attaching;
                attached = std::async( std::launch::async, [&] {
                    root->attachObject( nativeMesh );
                } );
                attachmentBlocked = attachmentReady.wait_for( std::chrono::seconds( 2 ) ) ==
                                        std::future_status::ready &&
                                    attached.wait_for( std::chrono::milliseconds( 0 ) ) ==
                                        std::future_status::timeout &&
                                    !wp_graphics_object_get_owner( nativeMesh->getNativeRenderObject() );
                graphics->lockAttempt = nullptr;
            }
            attached.get();
            attachmentBlocked = attachmentBlocked &&
                                wp_graphics_object_get_owner( nativeMesh->getNativeRenderObject() ) ==
                                    wp_graphics_scene_get_root_node( native );
            root->detachObject( nativeMesh );
        }
        // This headless fixture has no render loop to drain deferred loads.
        graphics->clearObjectQueues();
        scene->clear();
        nativeMesh = nullptr;
        mesh = nullptr;
        root = nullptr;
        scene->unload( nullptr );
        require( blocked && unchanged && captured,
                 "mesh creation must not mutate native membership while rendering holds the lock" );
        require( published, "mesh creation must resume and publish once rendering releases the lock" );
        require( attachmentBlocked,
                 "mesh attachment must preserve the captured owner chain until rendering releases the lock" );
    }

    inline bool run()
    {
        try
        {
            creationDuringCapture();
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
