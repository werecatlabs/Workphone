// Frozen 19 October 2026 preparation cache before compact fields and lookup changes.
// Benchmark reference only. Keep the original ownership, replacement and packing behavior.
// Both implementations restore the pre-cache metallic-alpha roughness routing below.
#ifndef WP_MATERIAL_RENDER_CACHE_REFERENCE_HPP
#define WP_MATERIAL_RENDER_CACHE_REFERENCE_HPP

#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <workphone_graphics_renderer_dx11.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <functional>

namespace workphone::render::material_cache_reference
{
    /** Material-only preparation. Lighting, fog, map readiness and native views remain draw-owned. */
    struct PreparedMaterialReference
    {
        wp_material_dx11 constants{};
        wp_material_dx11 compose() const { return constants; }
        std::array<u32, 6> textureSlots{ 1u, 2u, 3u, 13u, 22u, 24u };
        u32 wrapU = 0u, wrapV = 0u, filter = 1u, anisotropy = 1u;
    };

    /** Bounded, graphics-thread-owned cache. Weak owners neither retain textures nor permit
     * address reuse to turn an expired snapshot into a hit. Four adjacent entries form a set;
     * misses replace one entry without allocating. No device pointers are cached here.
     */
    class MaterialRenderCacheReference
    {
    public:
        static constexpr size_t Ways = 4;
        static constexpr size_t SetCount = 32;
        static constexpr size_t Capacity = Ways * SetCount;

        const PreparedMaterialReference &get( const MaterialRenderState::Ptr &state )
        {
            if( !state )
            {
                static const auto defaults = prepare( MaterialRenderState{} );
                return defaults;
            }
            auto hash = std::hash<const MaterialRenderState *>{}( state.get() ) >> 4u;
            hash ^= hash >> 11u;
            auto &set = m_sets[hash % SetCount];
            Entry *vacant = nullptr;
            for( auto &entry : set.entries )
            {
                if( !entry.owner.owner_before( state ) && !state.owner_before( entry.owner ) &&
                    entry.address == state.get() )
                    return entry.prepared;
                if( !vacant && entry.owner.expired() ) vacant = &entry;
            }
            auto &entry = vacant ? *vacant : set.entries[set.next++ % Ways];
            entry.prepared = prepare( *state );
            entry.address = state.get();
            entry.owner = state;
            ++m_preparations;
            return entry.prepared;
        }

        void clear()
        {
            for( auto &set : m_sets )
            {
                for( auto &entry : set.entries )
                {
                    entry.owner.reset();
                    entry.address = nullptr;
                }
                set.next = 0;
            }
            m_preparations = 0;
        }

        u64 getPreparationCount() const { return m_preparations; }

    private:
        static PreparedMaterialReference prepare( const MaterialRenderState &state )
        {
            PreparedMaterialReference result;
            auto &value = result.constants;
            const auto &diffuse = state.diffuseColour;
            const auto &specular = state.specularColour;
            const auto &emissive = state.emissiveColour;
            const auto intensity = state.getFlag( emissionEnabledFlag ) ? state.emissionIntensity : 0.0f;
            value.base_color = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
            value.specular_color = { specular.r, specular.g, specular.b, specular.a };
            value.emissive_color = { emissive.r * intensity, emissive.g * intensity,
                                     emissive.b * intensity, emissive.a };
            value.surface = { state.metalness, state.roughness, state.uvTilingX, state.uvTilingY };
            value.uv_transform = { state.uvOffsetX, state.uvOffsetY,
                                   state.materialType == MaterialType::UI ? 0.0f : 1.0f, state.uvRotation };
            value.controls = { state.normalStrength, state.aoStrength,
                               state.getFlag( cutoutFlag ) ? state.alphaClip : -1.0f,
                               state.blendMode == 2u ? 1.0f : 0.0f };
            value.texture_sources = { static_cast<f32>( state.metallicSource ),
                                      static_cast<f32>( state.roughnessSource ),
                                      static_cast<f32>( state.aoSource ),
                                      static_cast<f32>( state.opacitySource ) };
            value.projection = { static_cast<f32>( state.uvProjection <= 6u ? state.uvProjection : 0u ),
                                  std::isfinite( state.uvTriplanarScale ) ?
                                      std::max( state.uvTriplanarScale, 0.0001f ) : 1.0f, 0.0f, 0.0f };
            if( state.metallicSource == 3u ) result.textureSlots[1] = 25u;
            if( state.roughnessSource == 2u || state.roughnessSource == 4u ) result.textureSlots[2] = 2u;
            if( state.roughnessSource == 3u ) result.textureSlots[2] = 25u;
            if( state.aoSource == 1u ) result.textureSlots[4] = 25u;
            if( state.opacitySource == 2u ) result.textureSlots[5] = 25u;
            result.wrapU = state.uvWrapU;
            result.wrapV = state.uvWrapV;
            result.filter = state.uvFilter;
            result.anisotropy = std::isfinite( state.uvAniso ) ?
                                   static_cast<u32>( std::clamp( state.uvAniso, 1.0f, 16.0f ) ) : 1u;
            return result;
        }

        struct Entry
        {
            std::weak_ptr<const MaterialRenderState> owner;
            const MaterialRenderState *address = nullptr;
            PreparedMaterialReference prepared;
        };
        struct Set
        {
            std::array<Entry, Ways> entries;
            size_t next = 0;
        };
        std::array<Set, SetCount> m_sets;
        u64 m_preparations = 0;
    };
}
#endif
