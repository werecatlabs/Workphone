#ifndef WP_CLAW_MATERIAL_RENDER_CACHE_HPP
#define WP_CLAW_MATERIAL_RENDER_CACHE_HPP

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
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

    /** Persistent graphics-thread-owned CPU record table, with a small set-associative
     * front cache. Stable pages and generation-qualified indices survive frame/view
     * publications. Weak state identities permit retirement without retaining textures.
     * No device views, lighting or camera values are stored here.
     */
    class WPGraphics_API ClawMaterialRenderCache
    {
        struct Slot;
        struct Store;
    public:
        static constexpr size_t Ways = 4;
        static constexpr size_t SetCount = 32;
        static constexpr size_t Capacity = Ways * SetCount;

        // Capacity describes the front cache, not the number of persistent records.
        struct Handle { u64 table = 0, generation = 0; u32 slot = ~u32(0); };
        class WPGraphics_API Lease
        {
        public:
            const ClawPreparedMaterialDX11 *get(Handle) const;
            u64 getStorageBytes() const;
            u64 getIdentity() const;
        private:
            friend class ClawMaterialRenderCache;
            friend class ClawMaterialDrawRecords;
            const ClawPreparedMaterialDX11 *getPinned(Handle, const MaterialRenderState *) const;
            std::shared_ptr<const Store> m_store;
        };
        ClawMaterialRenderCache();
        ~ClawMaterialRenderCache();
        ClawMaterialRenderCache(const ClawMaterialRenderCache &);
        ClawMaterialRenderCache &operator=(const ClawMaterialRenderCache &);
        ClawMaterialRenderCache(ClawMaterialRenderCache &&) noexcept;
        ClawMaterialRenderCache &operator=(ClawMaterialRenderCache &&) noexcept;
        const ClawPreparedMaterialDX11 &get(const MaterialRenderState::Ptr &state)
        {
            if(!state) return defaults();
            if(m_lastIndex < Capacity)
            {
                const auto &last = m_sets[m_lastIndex / Ways].tags[m_lastIndex % Ways];
                if(matches(last, state)) return *last.prepared;
            }
            auto hash = std::hash<const MaterialRenderState *>{}(state.get()) >> 4u;
            hash ^= hash >> 11u;
            const auto setIndex = hash % SetCount;
            const auto &set = m_sets[setIndex];
            for(size_t way = 0; way < Ways; ++way)
                if(matches(set.tags[way], state))
                { m_lastIndex = setIndex * Ways + way; return *set.tags[way].prepared; }
            return getSlow(state, setIndex);
        }
        Handle resolve(const MaterialRenderState::Ptr &);
        const ClawPreparedMaterialDX11 *lookup(Handle) const;
        Lease retain() const;
        void clear();
        void collectExpired();
        u64 getStorageBytes() const;
        u64 getStorageAllocationCount() const;
        u32 getSlotCount() const;
        u32 getResidentCount() const;
        u64 getPreparationCount() const { return m_preparations; }

        // Also used by immutable scene publications, which prepare each distinct
        // material once and retain the result for every colour/shadow consumer.
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

    private:
        struct Tag
        {
            const MaterialRenderState *address = nullptr;
            Slot *slot = nullptr;
            const ClawPreparedMaterialDX11 *prepared = nullptr;
            std::weak_ptr<const MaterialRenderState> owner;
        };
        static bool matches(const Tag &tag, const MaterialRenderState::Ptr &state)
        {
            return tag.address == state.get() && !tag.owner.owner_before(state) &&
                !state.owner_before(tag.owner);
        }
        static const ClawPreparedMaterialDX11 &defaults();
        const ClawPreparedMaterialDX11 &getSlow(const MaterialRenderState::Ptr &, size_t setIndex);
        struct Set
        {
            std::array<Tag, Ways> tags;
            size_t next = 0;
        };
        std::shared_ptr<Store> m_store;
        std::array<Set, SetCount> m_sets{};
        u64 m_preparations = 0;
        size_t m_lastIndex = Capacity;
    };
}
#endif
