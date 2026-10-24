#pragma once

#include <Workphone/Animation/AnimationGraph.hpp>
#include <workphone_graphics_animation.h>
#include <workphone_graphics_skeleton.h>
#include "../../Engine/cpp/Source/WPGraphics/ClawAssetBudget.hpp"
#include <memory>
#include <stdexcept>

namespace native_animation_budget_contracts
{
    using namespace workphone;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    inline void run()
    {
        auto scheduler = WorldStreamingScheduler::shared();
        constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto original = scheduler->stats().limits[source];
        const auto baseline = scheduler->stats().used[source];
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, original };
        const auto used = [&] { return scheduler->stats().used[source]; };
        const auto set = [&]( u64 bytes )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ), "native shared limit applies" );
        };
        const auto &policy = render::asset_budget::meshMemoryCallbacks;
        set( baseline );
        require( !wp_animation_create_budgeted( "denied", 1, &policy ) && used() == baseline,
                 "native animation body obeys the engine SourceCpu cap" );
        set( original );
        std::unique_ptr<wp_animation, decltype(&wp_animation_destroy)> animation(
            wp_animation_create_budgeted( "city", 1, &policy ), wp_animation_destroy );
        require( bool(animation) && used() > baseline + sizeof(wp_animation),
                 "native inline track/keyframe spare body and header use the shared engine ledger" );
        const auto animationBytes = used() - baseline;
        wp_skeleton skeleton;
        require( wp_skeleton_init_budgeted( &skeleton, &policy ) != 0, "native skeleton accepts engine policy" );
        struct Cleanup
        {
            wp_skeleton *skeleton;
            ~Cleanup() { wp_skeleton_destroy( skeleton ); }
        } cleanup{ &skeleton };
        require( wp_skeleton_create_bone_auto( &skeleton ) != nullptr, "native bone uses the same policy" );
        const auto boneBytes = used() - baseline - animationBytes;
        require( boneBytes > sizeof(wp_bone), "native bone header is part of shared Source ownership" );
        set( used() );
        require( !wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 1,
                 "full shared ledger refuses a bone without consuming its handle" );
        bool denied = false;
        try { auto graph = std::make_unique<animation::AnimationGraphDefinition>(); }
        catch( const std::bad_alloc & ) { denied = true; }
        require( denied, "native bodies consume the same cap as C++ graph bodies" );
        animation.reset();
        require( used() == baseline + boneBytes, "native animation physically retires its shared reservation" );
        {
            auto graph = std::make_unique<animation::AnimationGraphDefinition>();
            require( used() > baseline + boneBytes, "retired native capacity admits a real C++ graph body" );
        }
        require( wp_skeleton_create_bone_auto( &skeleton ) && skeleton.next_auto_handle == 2,
                 "native bone retries after shared capacity is released" );
        set( used() );
        wp_skeleton_destroy( &skeleton );
        require( used() == baseline, "full-cap native retirement releases all engine Source charges" );
    }
}
