#ifndef WP_CLAW_MATERIAL_RENDER_CACHE_HPP
#define WP_CLAW_MATERIAL_RENDER_CACHE_HPP

#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <workphone_graphics_renderer_dx11.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cstring>

namespace workphone::render
{
    /** Material-only preparation. Lighting, fog, map readiness and native views remain draw-owned. */
    struct ClawPreparedMaterialDX11
    {
        // Keep view/draw-owned fields out of every resident cache entry.
        struct Constants
        {
            wp_vec4f base_color{}, emissive_color{}, specular_color{};
            wp_vec4f surface{}, uv_transform{}, controls{}, texture_sources{}, projection{};
        } constants;
        std::array<u32, 6> textureSlots{ 1u, 2u, 3u, 13u, 22u, 24u };
        u32 wrapU = 0u, wrapV = 0u, filter = 1u, anisotropy = 1u;

        wp_material_dx11 compose() const
        {
            static_assert( sizeof( Constants ) == sizeof( wp_vec4f ) * 8,
                           "material constants must contain eight packed vectors" );
            static_assert( sizeof( wp_material_dx11 ) == sizeof( wp_vec4f ) * 17,
                           "update draw composition when the native shader block changes" );
            wp_material_dx11 draw;
            // Copy contiguous groups and initialize only draw-owned fields. A
            // whole-block clear followed by individual assignments adds stores
            // to the warm path and prevents efficient group copies on MSVC.
            std::memcpy( &draw.base_color, &constants.base_color, sizeof( wp_vec4f ) * 3 );
            std::memcpy( &draw.surface, &constants.surface, sizeof( wp_vec4f ) * 3 );
            std::memcpy( &draw.texture_sources, &constants.texture_sources, sizeof( wp_vec4f ) * 2 );
            draw.light_color = draw.light_direction = draw.camera_position = {};
            draw.map_flags = draw.extra_map_flags = {};
            draw.ambient_color = draw.environment = draw.fog_color = draw.fog_params = {};
            return draw;
        }
    };

    /** Bounded, graphics-thread-owned cache. Weak owners neither retain textures nor permit
     * address reuse to turn an expired snapshot into a hit. Four adjacent lookup tags form a set;
     * misses replace one entry without allocating. No device pointers are cached here.
     */
    class ClawMaterialRenderCache
    {
    public:
        static constexpr size_t Ways = 4;
        static constexpr size_t SetCount = 32;
        static constexpr size_t Capacity = Ways * SetCount;

        const ClawPreparedMaterialDX11 &get( const MaterialRenderState::Ptr &state )
        {
            if( !state )
            {
                static const auto defaults = prepare( MaterialRenderState{} );
                return defaults;
            }
            if( m_lastIndex < Capacity )
            {
                auto &last = m_sets[m_lastIndex / Ways];
                const auto way = m_lastIndex % Ways;
                if( matches( last.tags[way], state ) ) return last.prepared[way];
            }
            auto hash = std::hash<const MaterialRenderState *>{}( state.get() ) >> 4u;
            hash ^= hash >> 11u;
            const auto setIndex = hash % SetCount;
            auto &set = m_sets[setIndex];
            for( size_t way = 0; way < Ways; ++way )
            {
                if( matches( set.tags[way], state ) )
                {
                    m_lastIndex = setIndex * Ways + way;
                    return set.prepared[way];
                }
            }
            // Strong-count inspection belongs to misses. A warm lookup compares
            // addresses/control-block identities without touching reference counts.
            size_t vacant = Ways;
            for( size_t way = 0; way < Ways; ++way )
                if( set.tags[way].owner.expired() ) { vacant = way; break; }
            const auto way = vacant < Ways ? vacant : set.next++ % Ways;
            auto &entry = set.tags[way];
            set.prepared[way] = prepare( *state );
            entry.address = state.get();
            entry.owner = state;
            m_lastIndex = setIndex * Ways + way;
            ++m_preparations;
            return set.prepared[way];
        }

        void clear()
        {
            for( auto &set : m_sets )
            {
                for( auto &entry : set.tags )
                {
                    entry.owner.reset();
                    entry.address = nullptr;
                }
                set.next = 0;
            }
            m_preparations = 0;
            m_lastIndex = Capacity;
        }

        u64 getPreparationCount() const { return m_preparations; }

    private:
        static ClawPreparedMaterialDX11 prepare( const MaterialRenderState &state )
        {
            ClawPreparedMaterialDX11 result;
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

        struct Tag
        {
            std::weak_ptr<const MaterialRenderState> owner;
            const MaterialRenderState *address = nullptr;
        };
        static bool matches( const Tag &entry, const MaterialRenderState::Ptr &state )
        {
            return entry.address == state.get() && !entry.owner.owner_before( state ) &&
                   !state.owner_before( entry.owner );
        }
        struct Set
        {
            std::array<Tag, Ways> tags;
            std::array<ClawPreparedMaterialDX11, Ways> prepared;
            size_t next = 0;
        };
        std::array<Set, SetCount> m_sets;
        u64 m_preparations = 0;
        size_t m_lastIndex = Capacity;
    };
}
#endif
