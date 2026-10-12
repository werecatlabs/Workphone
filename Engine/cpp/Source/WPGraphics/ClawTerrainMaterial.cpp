#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawTerrainMaterial.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <thread>
#include <limits>
#include <cassert>

namespace workphone::render
{
    using Microsoft::WRL::ComPtr;

    struct ClawTerrainMaterial::Candidate
    {
        // Declaration order releases views/textures before their owners and budget.
        StreamBudget ownedGpu;
        TextureResolver retainedResolver;
        TerrainSnapshot snapshot;
        wp_terrain_material_dx11 constants{};
        std::array<SmartPtr<ITexture>, 13> textures;
        std::array<ComPtr<ID3D11ShaderResourceView>, WP_TERRAIN_MATERIAL_VIEWS> views;
        ComPtr<ID3D11Device> device;
        std::shared_ptr<const int> publisher;
        u64 ticket = 0;
    };

    struct ClawTerrainMaterial::Data
    {
        mutable std::thread::id owner;
        std::shared_ptr<const int> publisher = std::make_shared<const int>( 0 );
        mutable u64 ticket = 0;
        std::shared_ptr<const Candidate> active;

        bool check( String &error, bool claim = false ) const
        {
            // Component construction may happen on the editor/game thread. Only
            // actual DX11 staging establishes the render-thread ownership contract.
            if( claim && owner == std::thread::id{} )
            {
                owner = std::this_thread::get_id();
            }
            if( owner == std::this_thread::get_id() )
            {
                return true;
            }
            error = "Terrain material requires its owning render thread.";
            return false;
        }
    };

    namespace
    {
        wp_renderer_dx11 *nativeRenderer( ClawRendererDX11 &renderer )
        {
            return renderer.isLoaded() ? wp_renderer_get_dx11( renderer.getNativeRenderer() ) : nullptr;
        }

        ID3D11Device *nativeDevice( ClawRendererDX11 &renderer )
        {
            auto native = nativeRenderer( renderer );
            return native ? static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) )
                          : nullptr;
        }

        SmartPtr<ITexture> resolveTexture( const String &name, String &error )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto database = app ? app->getResourceDatabase() : nullptr;
            auto texture = database ? database->loadResourceByType<ITexture>( name ) : nullptr;
            if( !texture )
            {
                error = "Terrain texture is unavailable: " + name;
            }
            return texture;
        }
    }  // namespace

    ClawTerrainMaterial::ClawTerrainMaterial() : m_data( std::make_unique<Data>() )
    {
    }

    ClawTerrainMaterial::~ClawTerrainMaterial() = default;

    std::shared_ptr<const ClawTerrainMaterial::Candidate> ClawTerrainMaterial::stage(
        ClawRendererDX11 &renderer, TerrainSnapshot snapshot, String &error, TextureResolver resolver,
        SmartPtr<ITexture> legacyAlbedo ) const
    {
        error.clear();
        if( !m_data->check( error, true ) )
        {
            return {};
        }
        if( m_data->ticket == std::numeric_limits<u64>::max() )
        {
            error = "Terrain material request sequence exhausted.";
            return {};
        }
        const auto ticket = ++m_data->ticket;
        try
        {
            if( !snapshot )
            {
                error = "Terrain material snapshot is missing.";
                return {};
            }
            if( !validateTerrainData( *snapshot, error ) )
            {
                return {};
            }
            auto candidate = std::make_shared<Candidate>();
            candidate->device = nativeDevice( renderer );
            if( !candidate->device || FAILED( candidate->device->GetDeviceRemovedReason() ) )
            {
                error = "Terrain material requires an available DX11 device.";
                return {};
            }
            candidate->snapshot = snapshot;
            candidate->publisher = m_data->publisher;
            candidate->ticket = ticket;
            auto &c = candidate->constants;
            c.controls.x = 1;
            const auto &appearance = snapshot->appearance;
            c.appearance = { f32( appearance.projection ), appearance.triplanarSharpness,
                             appearance.heightBlend ? 1.0f : 0.0f, appearance.heightTransition };
            c.detail = { appearance.detailUvScale.x, appearance.detailUvScale.y,
                         appearance.detailStrength, 0 };
            c.macro = { appearance.macroScale, appearance.macroStrength,
                        f32( appearance.macroSeed & 65535u ), f32( appearance.macroSeed >> 16 ) };
            c.macro_tint = { appearance.macroTint.x, appearance.macroTint.y, appearance.macroTint.z,
                             appearance.macroTint.w };
            c.weather = { appearance.wetness, appearance.wetDarkening, appearance.wetRoughness,
                          appearance.snow };
            c.snow = { appearance.snowRoughness, 0, 0, 0 };
            c.snow_tint = { appearance.snowTint.x, appearance.snowTint.y, appearance.snowTint.z,
                            appearance.snowTint.w };
            const auto width = snapshot->dimensions.X(), depth = snapshot->dimensions.Y();
            const u64 ownedGpuBytes = snapshot->layerWeights.size() * sizeof( wp_vec4f ) +
                                      snapshot->surfaceMasks.size() * sizeof( wp_vec2f );
            if( ownedGpuBytes )
            {
                candidate->ownedGpu = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Gpu,
                                                                                  ownedGpuBytes, error );
                if( !candidate->ownedGpu )
                {
                    return {};
                }
            }
            // Samples align at texel centres; bilinear controls agree at shared tile borders.
            c.control_transform.z = 1.0f / ( width * snapshot->spacing.X() );
            c.control_transform.w = 1.0f / ( depth * snapshot->spacing.Y() );
            c.control_transform.x = 0.5f / width - snapshot->origin.X() * c.control_transform.z;
            c.control_transform.y = 0.5f / depth - snapshot->origin.Y() * c.control_transform.w;
            if( !resolver )
            {
                resolver = resolveTexture;
            }
            candidate->retainedResolver = resolver;
            for( size_t layer = 0; layer < 4; ++layer )
            {
                const auto &source = snapshot->layers[layer];
                c.tint[layer] = { source.tint.X(), source.tint.Y(), source.tint.Z(), source.tint.W() };
                c.surface[layer] = { source.uvScale.X(), source.uvScale.Y(), source.roughness,
                                     source.metallic };
                c.maps[layer].w = source.normalStrength;
                c.height[layer] = { source.heightScale, source.heightBias, 0, 0 };
                const String *names[] = { &source.albedo, &source.normal, &source.orm };
                float *flags[] = { &c.maps[layer].x, &c.maps[layer].y, &c.maps[layer].z };
                for( size_t map = 0; map < 3; ++map )
                {
                    const bool legacy = layer == 0 && map == 0 && names[map]->empty() && legacyAlbedo;
                    if( names[map]->empty() && !legacy )
                    {
                        continue;
                    }
                    auto texture = legacy ? legacyAlbedo : resolver( *names[map], error );
                    if( !texture )
                    {
                        if( error.empty() )
                        {
                            error = "Terrain texture resolver returned no resource: " + *names[map];
                        }
                        return {};
                    }
                    void *view = nullptr;
                    texture->getTextureFinal( &view );
                    if( !view )
                    {
                        error = "Terrain texture has no uploaded GPU view: " + *names[map];
                        return {};
                    }
                    const auto slot = layer * 3 + map;
                    candidate->textures[slot] = texture;
                    candidate->views[slot] = static_cast<ID3D11ShaderResourceView *>( view );
                    ComPtr<ID3D11Device> textureDevice;
                    candidate->views[slot]->GetDevice( &textureDevice );
                    D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                    candidate->views[slot]->GetDesc( &desc );
                    if( textureDevice.Get() != candidate->device.Get() ||
                        desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D )
                    {
                        error = "Terrain layer requires a 2D texture from the active device.";
                        return {};
                    }
                    *flags[map] = 1;
                }
            }
            if( !appearance.detailNormal.empty() )
            {
                auto texture = resolver( appearance.detailNormal, error );
                void *view = nullptr;
                if( texture )
                {
                    texture->getTextureFinal( &view );
                }
                if( !view )
                {
                    if( error.empty() )
                    {
                        error =
                            "Terrain detail normal has no uploaded GPU view: " + appearance.detailNormal;
                    }
                    return {};
                }
                candidate->textures[12] = texture;
                candidate->views[13] = static_cast<ID3D11ShaderResourceView *>( view );
                c.detail.w = 1;
            }
            if( !snapshot->layerWeights.empty() )
            {
                Array<wp_vec4f> weights;
                weights.reserve( snapshot->layerWeights.size() );
                for( const auto &w : snapshot->layerWeights )
                {
                    weights.push_back( { w.X(), w.Y(), w.Z(), w.W() } );
                }
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = width;
                desc.Height = depth;
                desc.MipLevels = desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA data{};
                data.pSysMem = weights.data();
                data.SysMemPitch = width * sizeof( wp_vec4f );
                ComPtr<ID3D11Texture2D> control;
                if( FAILED( candidate->device->CreateTexture2D( &desc, &data, &control ) ) ||
                    FAILED( candidate->device->CreateShaderResourceView( control.Get(), nullptr,
                                                                         &candidate->views[12] ) ) )
                {
                    error = "Terrain control-map upload failed; previous material retained.";
                    return {};
                }
                c.controls.y = 1;
            }
            if( !snapshot->surfaceMasks.empty() )
            {
                Array<wp_vec2f> masks;
                masks.reserve( snapshot->surfaceMasks.size() );
                for( const auto &mask : snapshot->surfaceMasks )
                {
                    masks.push_back( { mask.x, mask.y } );
                }
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = width;
                desc.Height = depth;
                desc.MipLevels = desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R32G32_FLOAT;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA data{};
                data.pSysMem = masks.data();
                data.SysMemPitch = width * sizeof( wp_vec2f );
                ComPtr<ID3D11Texture2D> masksTexture;
                if( FAILED( candidate->device->CreateTexture2D( &desc, &data, &masksTexture ) ) ||
                    FAILED( candidate->device->CreateShaderResourceView( masksTexture.Get(), nullptr,
                                                                         &candidate->views[14] ) ) )
                {
                    error = "Terrain wetness/snow mask upload failed; previous material retained.";
                    return {};
                }
                c.snow.y = 1;
            }
            if( candidate->device.Get() != nativeDevice( renderer ) )
            {
                error = "DX11 device changed while staging terrain material.";
                return {};
            }
            void *views[WP_TERRAIN_MATERIAL_VIEWS]{};
            for( size_t i = 0; i < WP_TERRAIN_MATERIAL_VIEWS; ++i )
            {
                views[i] = candidate->views[i].Get();
            }
            if( !wp_renderer_dx11_validate_terrain_material( nativeRenderer( renderer ), &c, views ) )
            {
                error = "Terrain constants or views are outside the supported DX11 range.";
                return {};
            }
            return candidate;
        }
        catch( const std::exception &e )
        {
            error = String( "Terrain material staging failed: " ) + e.what();
            return {};
        }
    }

    bool ClawTerrainMaterial::publish( ClawRendererDX11 &renderer,
                                       std::shared_ptr<const Candidate> candidate, String &error )
    {
        if( !validateCandidate( renderer, candidate, error ) )
        {
            return false;
        }
        commitPrevalidated( std::move( candidate ) );
        return true;
    }

    bool ClawTerrainMaterial::validateCandidate( ClawRendererDX11 &renderer,
                                                 const std::shared_ptr<const Candidate> &candidate,
                                                 String &error ) const
    {
        error.clear();
        if( !m_data->check( error ) )
        {
            return false;
        }
        if( !candidate || candidate->publisher != m_data->publisher ||
            candidate->ticket != m_data->ticket || candidate->device.Get() != nativeDevice( renderer ) ||
            FAILED( candidate->device->GetDeviceRemovedReason() ) )
        {
            error = "Terrain material candidate is obsolete or belongs to another owner/device.";
            return false;
        }
        return true;
    }

    void ClawTerrainMaterial::commitPrevalidated( std::shared_ptr<const Candidate> candidate ) noexcept
    {
        swapPrevalidated( candidate );
    }

    void ClawTerrainMaterial::swapPrevalidated( std::shared_ptr<const Candidate> &candidate ) noexcept
    {
        assert( m_data->owner == std::this_thread::get_id() && candidate &&
                candidate->publisher == m_data->publisher && candidate->ticket == m_data->ticket );
        m_data->active.swap( candidate );
    }

    bool ClawTerrainMaterial::bind( ClawRendererDX11 &renderer, String &error ) const
    {
        error.clear();
        if( !m_data->check( error ) )
        {
            return false;
        }
        const auto active = m_data->active;
        if( !active || active->device.Get() != nativeDevice( renderer ) ||
            FAILED( active->device->GetDeviceRemovedReason() ) )
        {
            error = "Terrain material has no active publication for this device.";
            return false;
        }
        void *views[WP_TERRAIN_MATERIAL_VIEWS]{};
        for( size_t i = 0; i < WP_TERRAIN_MATERIAL_VIEWS; ++i )
        {
            views[i] = active->views[i].Get();
        }
        if( !wp_renderer_dx11_set_terrain_material( nativeRenderer( renderer ), &active->constants,
                                                    views ) )
        {
            error = "Terrain material binding rejected.";
            return false;
        }
        return true;
    }

    void ClawTerrainMaterial::unbind( ClawRendererDX11 &renderer )
    {
        wp_renderer_dx11_set_terrain_material( nativeRenderer( renderer ), nullptr, nullptr );
    }

    TerrainSnapshot ClawTerrainMaterial::snapshot() const
    {
        return m_data->active ? m_data->active->snapshot : TerrainSnapshot{};
    }

    u64 ClawTerrainMaterial::ownedGpuBytes() const
    {
        return m_data->active && m_data->active->ownedGpu ? m_data->active->ownedGpu->bytes() : 0;
    }

    void ClawTerrainMaterial::clear()
    {
        // Called under the terrain lifecycle/render exclusion gate. Releasing
        // retained D3D11 resource references does not use the immediate context.
        if( m_data->ticket != std::numeric_limits<u64>::max() )
        {
            ++m_data->ticket;
        }
        m_data->active.reset();
        m_data->owner = std::thread::id{};
    }
}  // namespace workphone::render
