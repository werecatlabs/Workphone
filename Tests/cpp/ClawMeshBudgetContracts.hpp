#pragma once

#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <workphone_graphics_mesh_writer.h>
#include <workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace claw_mesh_budget_contracts
{
    using namespace workphone;
    using namespace workphone::render;

    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    class MeshFiles final : public FileSystem
    {
    public:
        Array<u8> encoded;
        MeshFiles()
        {
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0 } },
                { { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
                { { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 } } };
            const wp_u32 indices[] = { 0, 1, 2 };
            std::unique_ptr<wp_graphics_mesh, decltype( &wp_graphics_mesh_destroy )> source(
                wp_graphics_mesh_create(), &wp_graphics_mesh_destroy );
            void *bytes = nullptr;
            wp_u32 size = 0;
            require( source && wp_graphics_mesh_set_vertices( source.get(), WORKPHONE_VERTEX_FORMAT_PNT, vertices, 3 ) &&
                wp_graphics_mesh_set_indices_u32( source.get(), indices, 3 ) &&
                wp_graphics_mesh_add_submesh( source.get(), 0, 3, 1 ) >= 0 &&
                wp_graphics_mesh_write_to_buffer( source.get(), &bytes, &size ), "CPU mesh fixture must serialize" );
            std::unique_ptr<void, decltype( &std::free )> owner( bytes, &std::free );
            encoded.resize( size );
            std::memcpy( encoded.data(), bytes, size );
        }
        SmartPtr<workphone::IStream> open( const String &name, bool, bool, bool, bool, bool ) override
        {
            return make_ptr<MemoryFile>( encoded.data(), long( encoded.size() ), name, false );
        }
    };

    inline bool run()
    {
        const auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto working = size_t( StreamMemoryClass::WorkingCpu );
        auto scheduler = WorldStreamingScheduler::shared();
        const auto baseline = scheduler->stats();
        auto app = core::IApplicationManager::instance();
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            WorldStreamStats stats;
            SmartPtr<core::IApplicationManager> application;
            SmartPtr<IFileSystem> files;
            ~Restore()
            {
                String error;
                scheduler->setBudget( StreamMemoryClass::SourceCpu,
                    stats.limits[size_t( StreamMemoryClass::SourceCpu )], error );
                scheduler->setBudget( StreamMemoryClass::WorkingCpu,
                    stats.limits[size_t( StreamMemoryClass::WorkingCpu )], error );
                application->setFileSystem( files );
            }
        } restore{ scheduler, baseline, app, app->getFileSystem() };
        try
        {
            app->setFileSystem( make_ptr<MeshFiles>() );
            {
                ClawMesh mesh;
                // Empty initialization isolates the persistent mesh object from
                // the file/decode allocation whose rejection is under test.
                mesh.load( nullptr );
                mesh.setMeshName( "budgeted.meshbin" );
                const auto placeholder = scheduler->stats();
                require( placeholder.used[source] > baseline.used[source],
                    "ordinary native mesh objects must reserve shared source bytes" );
                String error;
                require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, placeholder.used[working], error ),
                    "CPU mesh test must constrain file working capacity" );
                mesh.load( nullptr );
                require( !mesh.isLoaded() && scheduler->stats().used[source] == placeholder.used[source] &&
                    scheduler->stats().used[working] == placeholder.used[working],
                    "denied file working bytes must leave no partial resident mesh" );
                require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, baseline.limits[working], error ) &&
                    scheduler->setBudget( StreamMemoryClass::SourceCpu, placeholder.used[source], error ),
                    "CPU mesh test must constrain retained native storage" );
                mesh.load( nullptr );
                require( !mesh.isLoaded() && scheduler->stats().used[source] == placeholder.used[source],
                    "denied retained geometry must release all decoder working bytes" );
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                    "CPU mesh source capacity must restore" );
                mesh.load( nullptr );
                require( mesh.isLoaded() && wp_graphics_mesh_get_vertex_count( mesh.getNativeMesh() ) == 3,
                    "CPU mesh load must retry without changing the resource path" );
                require( scheduler->stats().used[working] == baseline.used[working],
                    "successful mesh decoding must release transient file storage" );
                const auto raw = scheduler->stats();
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, raw.used[source], error ),
                    "CPU mesh test must constrain normalization capacity" );
                require( !wp_graphics_mesh_prepare_lit_vertices( mesh.getNativeMesh() ) &&
                    scheduler->stats().used[source] == raw.used[source],
                    "native lit conversion must reject source capacity without leaking" );
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                    "CPU lit conversion capacity must restore" );
                require( wp_graphics_mesh_prepare_lit_vertices( mesh.getNativeMesh() ),
                    "native lit conversion must retry after budget recovery" );
                const auto resident = scheduler->stats();
                require( resident.used[source] > raw.used[source],
                    "cached native normalized vertices must retain their own source lease" );
                auto previous = mesh.getNativeMesh();
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, resident.used[source], error ),
                    "CPU mesh test must constrain replacement overlap" );
                mesh.reload( nullptr );
                require( mesh.isLoaded() && mesh.getNativeMesh() == previous &&
                    scheduler->stats().used[source] == resident.used[source],
                    "denied native reload must retain last-good geometry and reservations" );
                require( !mesh.clone(), "mesh clone must independently admit its native CPU storage" );
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                    "CPU mesh clone capacity must restore" );
                Array<wp_skin_vertex> bind( 3 );
                bind.reserve( 32 );
                for( auto &vertex : bind )
                {
                    std::memset( &vertex, 0, sizeof( vertex ) );
                    vertex.weights[0] = 1;
                    vertex.normal.z = 1;
                }
                require( mesh.setSkinningData( bind ), "native skin bind/output must admit shared source bytes" );
                const auto skinned = scheduler->stats();
                const auto skinBytes = 3 * ( sizeof( wp_skin_vertex ) + sizeof( wp_skin_result ) );
                require( skinned.used[source] == resident.used[source] + skinBytes,
                    "skin bind and output must reserve their actual independent arrays" );
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, skinned.used[source], error ) &&
                    !mesh.setSkinningData( bind ) && mesh.hasSkinningData() &&
                    scheduler->stats().used[source] == skinned.used[source],
                    "denied skin replacement must retain the previous bind/output leases" );
                require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                    "CPU skin replacement capacity must restore" );
                auto clone = dynamic_pointer_cast<ClawMesh>( mesh.clone() );
                require( clone && clone->hasSkinningData() && clone->getNativeMesh() != mesh.getNativeMesh(),
                    "mesh clone must own independent native storage and share immutable skin input" );
                const auto cloned = scheduler->stats();
                const auto nativeRawBytes = raw.used[source] - baseline.used[source];
                require( cloned.used[source] == skinned.used[source] + nativeRawBytes + 3 * sizeof( wp_skin_result ),
                    "clone must charge geometry/output once while sharing the immutable bind reservation" );
                mesh.unload( nullptr );
                require( scheduler->stats().used[source] == baseline.used[source] + nativeRawBytes + skinBytes,
                    "clone must retain immutable bind bytes after original native mesh destruction" );
                clone = nullptr;
                require( scheduler->stats().used[source] == baseline.used[source],
                    "final clone destruction must release all native CPU and shared skin bytes" );
            }
            require( scheduler->stats().used[working] == baseline.used[working] &&
                scheduler->stats().used[source] == baseline.used[source],
                "CPU mesh contracts must restore their physical allocation baselines" );
            std::printf( "Claw native CPU mesh budget contracts passed\n" );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "FAIL Claw native CPU mesh budgets: %s\n", error.what() );
            return false;
        }
    }

    inline bool runRendered( ClawRendererDX11 &renderer )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        const auto baseline = scheduler->stats();
        const auto working = size_t( StreamMemoryClass::WorkingCpu );
        const auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto gpu = size_t( StreamMemoryClass::Gpu );
        auto app = core::IApplicationManager::instance();
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 sourceLimit;
            u64 workingLimit;
            u64 gpuLimit;
            SmartPtr<core::IApplicationManager> application;
            SmartPtr<IFileSystem> files;
            ClawRendererDX11 &renderer;
            ~Restore()
            {
                String error;
                scheduler->setBudget( StreamMemoryClass::SourceCpu, sourceLimit, error );
                scheduler->setBudget( StreamMemoryClass::WorkingCpu, workingLimit, error );
                scheduler->setBudget( StreamMemoryClass::Gpu, gpuLimit, error );
                application->setFileSystem( files );
                auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
                auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
                context->ClearState();
                wp_renderer_dx11_invalidate_state( native );
                context->Flush();
            }
        } restore{ scheduler, baseline.limits[source], baseline.limits[working], baseline.limits[gpu], app, app->getFileSystem(), renderer };
        try
        {
            app->setFileSystem( make_ptr<MeshFiles>() );
            ClawMesh mesh;
            mesh.setMeshName( "render-working.meshbin" );
            mesh.load( nullptr );
            require( mesh.isLoaded(), "native conversion scratch test must load CPU geometry" );
            String error;
            const auto beforeMetadata = scheduler->stats();
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, beforeMetadata.used[source], error ),
                "direct draw metadata test must exhaust retained CPU capacity" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            require( scheduler->stats().used[gpu] == baseline.used[gpu] &&
                scheduler->stats().used[source] == beforeMetadata.used[source] &&
                scheduler->stats().rejectedReservations[source] > beforeMetadata.rejectedReservations[source],
                "denied cold material metadata must defer a direct draw without false GPU admission or leaks" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                "direct draw material metadata capacity must restore" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, baseline.used[working], error ),
                "native index conversion test must constrain working capacity" );
            const auto rejectedBefore = scheduler->stats().rejectedReservations[working];
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            require( scheduler->stats().used[gpu] == baseline.used[gpu] &&
                scheduler->stats().used[working] == baseline.used[working] &&
                scheduler->stats().rejectedReservations[working] == rejectedBefore + 1,
                "denied native index conversion must upload no GPU geometry and release all scratch" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, baseline.limits[working], error ),
                "native index conversion working capacity must restore" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            const auto resident = scheduler->stats();
            require( resident.used[gpu] > baseline.used[gpu] && resident.used[working] == baseline.used[working],
                "native index conversion must retry and release its working lease after upload" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, baseline.used[working], error ),
                "native cache-hit test must constrain working capacity" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            require( scheduler->stats().used[gpu] == resident.used[gpu] &&
                scheduler->stats().rejectedReservations[working] == resident.rejectedReservations[working],
                "resident native mesh cache hits must not request transient conversion storage" );
            const wp_u32 changedIndices[] = { 0, 2, 1 };
            require( wp_graphics_mesh_set_indices_u32( mesh.getNativeMesh(), changedIndices, 3 ),
                "native scratch test must change the mesh generation" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            require( scheduler->stats().used[gpu] == resident.used[gpu] &&
                scheduler->stats().used[working] == baseline.used[working],
                "denied native conversion replacement must retain last-good physical GPU storage" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, baseline.limits[working], error ),
                "native skin replacement test must restore conversion capacity" );
            // Recover the changed index generation, then exercise the CPU skin
            // setter without allowing it to retire the previous GPU generation.
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
            Microsoft::WRL::ComPtr<ID3D11Buffer> previousVertices;
            UINT stride = 0, offset = 0;
            context->IAGetVertexBuffers( 0, 1, previousVertices.GetAddressOf(), &stride, &offset );
            require( previousVertices != nullptr, "skin replacement test must bind previous GPU geometry" );
            Array<wp_skin_vertex> bind( 3 );
            const auto original = static_cast<const wp_graphics_mesh_vertex_pnt *>(
                wp_graphics_mesh_get_vertices( mesh.getNativeMesh() ) );
            for( size_t i = 0; i < bind.size(); ++i )
            {
                std::memset( &bind[i], 0, sizeof( bind[i] ) );
                bind[i].position = original[i].position;
                bind[i].normal = original[i].normal;
                bind[i].weights[0] = 1;
            }
            require( mesh.setSkinningData( bind ), "skin replacement test must admit bind/output arrays" );
            Array<wp_mat4f> palette( 1 );
            std::memset( palette.data(), 0, sizeof( palette[0] ) );
            for( unsigned i = 0; i < 4; ++i ) palette[0].m[i][i] = 1;
            palette[0].m[0][3] = 0.1f;
            const auto beforeSkin = scheduler->stats();
            require( scheduler->setBudget( StreamMemoryClass::Gpu, beforeSkin.used[gpu], error ) &&
                mesh.applySkinningPalette( palette ), "CPU skin update must succeed while GPU replacement is denied" );
            require( scheduler->stats().used[gpu] == beforeSkin.used[gpu],
                "CPU skin mutation must retain the previous GPU generation until replacement admission" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            require( scheduler->stats().used[gpu] == beforeSkin.used[gpu] &&
                scheduler->stats().rejectedReservations[gpu] > beforeSkin.rejectedReservations[gpu],
                "denied skin GPU replacement must retain last-good physical geometry" );
            require( scheduler->setBudget( StreamMemoryClass::Gpu, baseline.limits[gpu], error ),
                "skin GPU replacement capacity must restore" );
            renderer.renderMesh( &mesh, Matrix4F::identity() );
            Microsoft::WRL::ComPtr<ID3D11Buffer> replacedVertices;
            context->IAGetVertexBuffers( 0, 1, replacedVertices.GetAddressOf(), &stride, &offset );
            require( replacedVertices && replacedVertices.Get() != previousVertices.Get(),
                "skin GPU replacement must retry and bind the newly admitted generation" );
            std::printf( "Claw native CPU index conversion budget contracts passed\n" );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "FAIL Claw native CPU index conversion budgets: %s\n", error.what() );
            return false;
        }
    }
}
