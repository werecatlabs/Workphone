#pragma once

#include "ClawFoliageContracts.hpp"
#include "ClawTextureMipContracts.hpp"
#include "DX11TestEvidence.hpp"
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/System/SceneResourceCompiler.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <WorkphoneGraphics/workphone_graphics_object.h>
#include <WorkphoneGraphics/workphone_graphics_scene.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace claw_cooked_skin_scene_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using claw_foliage_contracts::require;

    class PlaybackClock final : public TimerMT
    {
    public:
        f64 getDeltaTime() const override { return 1.0; }
    };

    inline wp_vec3f position( const wp_graphics_mesh *mesh, size_t vertex )
    {
        const auto *data = wp_graphics_mesh_get_vertices( mesh );
        switch( wp_graphics_mesh_get_vertex_format( mesh ) )
        {
        case WORKPHONE_VERTEX_FORMAT_PNT:
            return static_cast<const wp_graphics_mesh_vertex_pnt *>( data )[vertex].position;
        case WORKPHONE_VERTEX_FORMAT_PNTC:
            return static_cast<const wp_graphics_mesh_vertex_pntc *>( data )[vertex].position;
        default:
            throw std::runtime_error( "Imported skin has an unsupported native vertex layout" );
        }
    }

    inline void consume( claw_texture_mip_contracts::Fixture &host,
                         std::shared_ptr<const resource::RuntimeResource> sceneResource )
    {
        String error;
        const auto graph = resource::decodeActorGraphResource( std::move( sceneResource ), error );
        require( graph != nullptr, error.c_str() );
        const auto context = resource::createActorGraphRuntimeContext( graph, "pc", true, error );
        require( context != nullptr, error.c_str() );

        auto game = make_ptr<scene::GameManager>();
        auto targetScene = make_ptr<scene::GameScene>();
        const auto previousTask = Thread::getCurrentTask();
        Thread::setCurrentTask( TaskId::Primary );
        struct SceneScope
        {
            claw_texture_mip_contracts::Fixture &host;
            SmartPtr<scene::GameManager> game;
            SmartPtr<ClawScene> graphicsScene;
            SmartPtr<ITimer> originalTimer;
            TaskId previousTask;

            ~SceneScope()
            {
                Thread::setCurrentTask( TaskId::Primary );
                host.application->setTimer( originalTimer );
                if( graphicsScene )
                    wp_graphics_scene_set_active_camera( graphicsScene->getNativeScene(), nullptr );
                host.renderer->setCamera( nullptr );
                host.renderer->setRenderTarget( nullptr );
                game->unload( nullptr );
                host.application->setGameManager( nullptr );
                host.graphics->clearObjectQueues();
                if( graphicsScene )
                {
                    graphicsScene->unload( nullptr );
                    host.graphics->setGraphicsScene( nullptr );
                }
                host.graphics->clearObjectQueues();
                Thread::setCurrentTask( previousTask );
            }
        } scope{ host, game, nullptr, host.application->getTimer(), previousTask };

        FactoryUtil::addFactory<scene::Transform>();
        FactoryUtil::addFactory<scene::Mesh>();
        FactoryUtil::addFactory<scene::MeshRenderer>();
        FactoryUtil::addFactory<scene::Animator>();
        host.application->setGameManager( game );
        game->load( nullptr );
        require( game->isLoaded(), "Cooked scene fixture requires the loaded GameManager" );
        game->setCurrentScene( targetScene );
        targetScene->load( nullptr );
        require( targetScene->isLoaded(), "Cooked scene fixture requires the loaded GameScene" );

        auto graphicsScene = make_ptr<ClawScene>();
        scope.graphicsScene = graphicsScene;
        host.graphics->setGraphicsScene( graphicsScene );
        graphicsScene->load( nullptr );
        graphicsScene->setAmbientLight( ColourF::White );
        graphicsScene->setEnableShadows( false );
        require( graphicsScene->getRootSceneNode() != nullptr, "Claw scene must own a native root" );

        const auto installed = resource::installCookedSceneGraph( graph, targetScene, context, error );
        require( installed, error.c_str() );
        // The test owns this renderer thread. Drain the same object queue used by the next frame.
        host.graphics->GraphicsSystem::update();

        const auto actors = targetScene->getActors();
        require( actors.size() == 1 && actors.front() && actors.front()->getName() == "ImportedCharacter",
                 "Cooked scene must publish the imported character actor" );
        const auto actor = actors.front();
        auto animator = actor->getComponent<scene::Animator>();
        const auto meshRenderer = actor->getComponent<scene::MeshRenderer>();
        require( animator && animator->isLoaded() && animator->getSelectedAnimationName() == "Lift" &&
                     std::abs( animator->getSelectedAnimationLength() - 2.0f ) < 1e-4f &&
                     meshRenderer && meshRenderer->isLoaded(),
                 "Scene Animator and MeshRenderer must load the captured imported clip and skin" );
        require( animator->getAutoPlay() && animator->isPlaying() && animator->getAnimationTime() == 0.0f,
                 "Cooked scene activation must start the imported clip at time zero" );
        const auto mesh = meshRenderer->getGraphicsObjectByType<ClawMesh>();
        require( mesh && mesh->isLoaded() && mesh->isVisible() && mesh->hasSkinningData() &&
                     mesh->getNativeRenderObject() && wp_graphics_object_get_owner( mesh->getNativeRenderObject() ),
                 "Cooked character must be visible on its MeshRenderer-owned native scene node" );
        const auto nativeMesh = mesh->getNativeMesh();
        require( nativeMesh && wp_graphics_mesh_get_vertex_count( nativeMesh ) == 4,
                 "Cooked character must retain the fixture's four native skin vertices" );

        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 128, 128 } );
        auto camera = make_ptr<ClawCamera>();
        auto *nativeCamera = camera->getNativeCamera();
        require( nativeCamera != nullptr, "Cooked character view requires a native camera" );
        wp_camera_set_position( nativeCamera, { 1.5f, 1.15f, 5.0f } );
        wp_camera_look_at( nativeCamera, { 1.5f, 1.15f, 0.0f }, { 0, 1, 0 } );
        camera->setAspectRatio( 1.0f );
        camera->setFOVy( 50.0f );
        camera->setNearClipDistance( 0.1f );
        camera->setFarClipDistance( 20.0f );
        host.renderer->setRenderTarget( target );
        host.renderer->setViewport( nullptr );
        host.renderer->setCamera( camera );
        auto *nativeRenderer = wp_renderer_get_dx11( host.renderer->getNativeRenderer() );
        require( recordDX11TestDevice( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( nativeRenderer ) ) ),
                 "Cooked skin scene must record its DX11 adapter" );
        claw_foliage_contracts::setup( nativeRenderer );

        const auto silhouette = [&] {
            host.renderer->clear( ColourF::Black );
            graphicsScene->render( static_cast<IRenderer *>( host.renderer.get() ) );
            const auto pixels = claw_foliage_contracts::pixels( nativeRenderer );
            std::vector<u8> result( pixels.size() / 4 );
            for( size_t p = 0; p < result.size(); ++p )
            {
                result[p] = unsigned( pixels[p * 4] ) + pixels[p * 4 + 1] + pixels[p * 4 + 2] > 30;
            }
            return result;
        };

        size_t rootVertex = 4;
        for( size_t i = 0; i < 4; ++i )
        {
            const auto rest = position( nativeMesh, i );
            if( std::abs( rest.x ) + std::abs( rest.y ) + std::abs( rest.z ) < 1e-4f ) rootVertex = i;
        }
        require( rootVertex < 4, "Bind pose must contain the original root-weighted corner" );
        const auto rest = silhouette();

        host.application->setTimer( make_ptr<PlaybackClock>() );
        Thread::setCurrentTask( TaskId::Application );
        game->update();
        Thread::setCurrentTask( TaskId::Primary );
        require( animator->isPlaying() && std::abs( animator->getAnimationTime() - 1.0f ) < 1e-4f,
                 "One application task tick must advance the active imported clip" );
        require( mesh->getNativeMesh() == nativeMesh &&
                     std::abs( position( nativeMesh, rootVertex ).x - 1.0f ) < 1e-4f,
                 "Scene Animator playback must move the existing native skin's root-weighted vertex by one unit" );
        const auto posed = silhouette();
        require( rest.size() == posed.size(), "Skin pose capture must retain the same render target size" );
        const auto restPixels = std::count( rest.begin(), rest.end(), u8( 1 ) );
        const auto posedPixels = std::count( posed.begin(), posed.end(), u8( 1 ) );
        const auto changed = std::inner_product( rest.begin(), rest.end(), posed.begin(), size_t( 0 ),
                                                std::plus<size_t>(), std::not_equal_to<u8>() );
        require( restPixels > 100 && posedPixels > 100 && changed > 100,
                 "Real Claw scene draws must show distinct nonempty bind and animated silhouettes" );
        animator->pause();
        animator->setAnimationTime( 0.0f );
        require( silhouette() == rest, "Seeking back must restore the original rendered silhouette" );
        std::printf( "PASS: imported cooked scene Animator reaches DX11 pixels (rest=%lld posed=%lld changed=%zu)\n",
                     static_cast<long long>( restPixels ), static_cast<long long>( posedPixels ), changed );
    }
}
