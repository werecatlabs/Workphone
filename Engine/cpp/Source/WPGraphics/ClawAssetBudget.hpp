#pragma once

#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_mesh.h>
#include <workphone_graphics_material.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <atomic>
#include <new>

namespace workphone::render::asset_budget
{
    inline StreamBudget reserveCpuBytes( u64 bytes, bool working ) noexcept
    {
        try
        {
            String error;
            return WorldStreamingScheduler::shared()->reserve(
                working ? StreamMemoryClass::WorkingCpu : StreamMemoryClass::SourceCpu, bytes, error );
        }
        catch( ... ) { return {}; }
    }
    inline void *reserveCpu( void *, wp_u64 bytes, wp_s32 working ) noexcept
    {
        auto lease = reserveCpuBytes( bytes, working != 0 );
        return lease ? new( std::nothrow ) StreamBudget( std::move( lease ) ) : nullptr;
    }
    inline void releaseCpu( void *reservation ) noexcept
    {
        delete static_cast<StreamBudget *>( reservation );
    }
    inline const wp_mesh_memory_callbacks meshMemoryCallbacks{ nullptr, reserveCpu, releaseCpu };
    inline const wp_material_memory_callbacks materialMemoryCallbacks{ nullptr, reserveCpu, releaseCpu };
    inline wp_graphics_material *createMaterial()
    {
        return wp_graphics_material_create_budgeted( &materialMemoryCallbacks );
    }
    inline wp_graphics_mesh *createMesh()
    {
        return wp_graphics_mesh_create_budgeted( &meshMemoryCallbacks );
    }

    // The physical D3D resource owns this lease, including references held by
    // the immediate context, native clients and views after wrapper retirement.
    inline const GUID resourceBudgetId =
        { 0xe6b8b0f1, 0xa1cc, 0x4bce, { 0x86, 0x61, 0x69, 0xfa, 0x10, 0xc5, 0x7a, 0xbb } };

    class GpuReservation final : public IUnknown
    {
    public:
        explicit GpuReservation( StreamBudget budget ) : m_budget( std::move( budget ) ) {}
        HRESULT STDMETHODCALLTYPE QueryInterface( REFIID id, void **out ) override
        {
            if( !out ) return E_POINTER;
            *out = nullptr;
            if( id != __uuidof( IUnknown ) ) return E_NOINTERFACE;
            *out = static_cast<IUnknown *>( this );
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            const auto remaining = --m_references;
            if( !remaining ) delete this;
            return remaining;
        }

    private:
        std::atomic<ULONG> m_references{ 1 };
        StreamBudget m_budget;
    };

    inline void *reserveGpu( void *, wp_u64 bytes ) noexcept
    {
        try
        {
            String error;
            auto lease = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Gpu, bytes, error );
            return lease ? new( std::nothrow ) GpuReservation( std::move( lease ) ) : nullptr;
        }
        catch( ... ) { return nullptr; }
    }
    inline wp_s32 retainGpu( void *reservation, void *resource ) noexcept
    {
        return reservation && resource && SUCCEEDED(
            static_cast<ID3D11Resource *>( resource )->SetPrivateDataInterface(
                resourceBudgetId, static_cast<GpuReservation *>( reservation ) ) );
    }
    inline void releaseGpu( void *reservation ) noexcept
    {
        if( reservation ) static_cast<GpuReservation *>( reservation )->Release();
    }
    inline bool retainTexture( void *view, StreamBudget budget )
    {
        if( !view || !budget ) return false;
        auto owner = new( std::nothrow ) GpuReservation( std::move( budget ) );
        if( !owner ) return false;
        ID3D11Resource *resource = nullptr;
        static_cast<ID3D11ShaderResourceView *>( view )->GetResource( &resource );
        const bool retained = retainGpu( owner, resource ) != 0;
        if( resource ) resource->Release();
        owner->Release();
        return retained;
    }
    inline wp_geometry_dx11 *prepareMesh( wp_renderer_dx11 *renderer, wp_graphics_mesh *mesh,
                                         wp_u32 first, wp_u32 count )
    {
        const wp_gpu_allocation_callbacks callbacks{ nullptr, reserveGpu, retainGpu, releaseGpu };
        return wp_renderer_dx11_prepare_mesh_geometry_budgeted( renderer, mesh, first, count, &callbacks );
    }
}
