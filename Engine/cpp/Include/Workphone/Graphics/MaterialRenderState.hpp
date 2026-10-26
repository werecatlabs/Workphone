#ifndef WP_MATERIAL_RENDER_STATE_HPP
#define WP_MATERIAL_RENDER_STATE_HPP
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <memory>
#include <stdexcept>

namespace workphone
{
    /** Immutable, material-only CPU values. Texture objects retain resource lifetime;
     * native views remain device-owned and are resolved by the texture adapter.
     */
    struct WPCore_API MaterialRenderState
    {
        using Ptr = std::shared_ptr<const MaterialRenderState>;

        /** Reserve each physical snapshot before allocation. The deleter owns
         * the lease so draw records remain charged after their material retires. */
        static std::shared_ptr<MaterialRenderState> create( const MaterialRenderState *source = nullptr )
        {
            String error;
            auto budget = WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::SourceCpu, sizeof( MaterialRenderState ), error );
            if( !budget )
            {
                throw std::runtime_error( error.c_str() );
            }
            return std::shared_ptr<MaterialRenderState>(
                source ? new MaterialRenderState( *source ) : new MaterialRenderState(),
                [budget]( MaterialRenderState *value ) mutable {
                    delete value;
                    budget.reset();
                } );
        }

        static constexpr u32 TextureCount = 32;
        ColourF specularColour = ColourF::White;
        ColourF ambientColour = ColourF::White;
        ColourF diffuseColour = ColourF::White;
        ColourF emissiveColour = ColourF::Black;
        f32 alphaClip = 0.5f;
        f32 opacity = 1.0f;
        f32 refractionAmount = 0.0f;
        f32 refractionIor = 1.45f;
        f32 metalness = 0.0f;
        f32 roughness = 0.5f;
        f32 normalStrength = 1.0f;
        f32 aoStrength = 1.0f;
        f32 emissionIntensity = 1.0f;
        f32 clearCoat = 0.0f;
        f32 clearCoatRoughness = 0.1f;
        f32 uvTilingX = 1.0f;
        f32 uvTilingY = 1.0f;
        f32 uvOffsetX = 0.0f;
        f32 uvOffsetY = 0.0f;
        f32 uvRotation = 0.0f;
        f32 uvTriplanarScale = 1.0f;
        f32 uvAniso = 1.0f;
        u32 blendMode = 0;
        u32 cullMode = 2;
        u32 uvProjection = 0u;
        u32 uvWrapU = 0u;
        u32 uvWrapV = 0u;
        u32 uvFilter = 1u;
        u32 metallicSource = 0u;
        u32 roughnessSource = 0u;
        u32 aoSource = 0u;
        u32 opacitySource = 0u;
        u32 depthTest = 2u;
        u32 flags = 0;
        MaterialType materialType = MaterialType::Standard;

        FixedArray<SmartPtr<render::ITexture>, TextureCount> textures = {};
        u64 revision = 0;

        bool getFlag( u32 flag ) const
        {
            return ( flags & flag ) != 0;
        }
    };
}  // namespace workphone
#endif
