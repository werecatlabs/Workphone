#ifndef MaterialPassState_h__
#define MaterialPassState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>

namespace workphone
{
    class WPCore_API MaterialPassStateData : public StateData
    {
    public:
        MaterialPassStateData();
        ~MaterialPassStateData() override;

        bool setEditorFloat( const String &name, f32 value );
        bool getEditorFloat( const String &name, f32 &value ) const;

        bool setEditorUInt( const String &name, u32 value );
        bool getEditorUInt( const String &name, u32 &value ) const;

        bool setEditorBool( const String &name, bool value );
        bool getEditorBool( const String &name, bool &value ) const;

        bool setEditorString( const String &name, const String &value );
        bool getEditorString( const String &name, String &value ) const;

        void setFlag( u32 flag, bool value );
        bool getFlag( u32 flag ) const;

        void writeEditorSettings( Properties &floats, Properties &uints, Properties &bools,
                                  Properties &strings ) const;

        WP_CLASS_REGISTER_DECL;

        ColourF tintColour = ColourF::White;
        ColourF specularColour = ColourF::White;
        ColourF ambientColour = ColourF::White;
        ColourF diffuseColour = ColourF::White;
        ColourF emissiveColour = ColourF::Black;

        FixedArray<SmartPtr<render::ITexture>, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES>
            textures = {};
        FixedArray<bool, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES> textureDirty = {};

        f32 alphaClip = 0.5f;
        f32 opacity = 1.0f;
        f32 refractionAmount = 0.0f;
        f32 refractionIor = 1.45f;

        f32 metalness = 0.0f;
        f32 roughness = 0.5f;
        f32 specular = 0.5f;
        f32 normalStrength = 1.0f;
        f32 aoStrength = 1.0f;
        f32 heightScale = 0.02f;
        f32 parallaxSteps = 16.0f;
        f32 emissionIntensity = 1.0f;
        f32 clearCoat = 0.0f;
        f32 clearCoatRoughness = 0.1f;
        f32 anisotropy = 0.0f;

        f32 uvTilingX = 1.0f;
        f32 uvTilingY = 1.0f;
        f32 uvOffsetX = 0.0f;
        f32 uvOffsetY = 0.0f;
        f32 uvRotation = 0.0f;
        f32 uvTriplanarScale = 1.0f;
        f32 uvAniso = 1.0f;

        f32 detailTilingX = 1.0f;
        f32 detailTilingY = 1.0f;
        f32 detailOffsetX = 0.0f;
        f32 detailOffsetY = 0.0f;
        f32 detailRotation = 0.0f;
        f32 detailStrength = 0.0f;
        f32 detailNormalStrength = 1.0f;

        f32 maxTextureSize = 2048.0f;
        f32 renderQueue = 2000.0f;
        f32 sortPriority = 0.0f;
        f32 stencilRef = 0.0f;
        f32 stencilReadMask = 255.0f;
        f32 stencilWriteMask = 255.0f;
        f32 layerMaskStrength = 1.0f;
        f32 previewExposure = 1.0f;
        f32 previewRotation = 0.0f;

        u32 preset = 0u;
        u32 renderMode = 0u;
        u32 workflow = 0u;
        u32 blendMode = 0; /**< Blending mode (implementation-specific). */
        u32 cullMode = 2;  /**< Face culling mode (none=1, clockwise=2, anticlockwise=3). */
        u32 uvSet = 0u;
        u32 uvProjection = 0u;
        u32 uvWrapU = 0u;
        u32 uvWrapV = 0u;
        u32 uvFilter = 1u;
        u32 detailBlendMode = 0u;
        u32 metallicSource = 0u;
        u32 roughnessSource = 0u;
        u32 aoSource = 0u;
        u32 opacitySource = 0u;
        u32 heightSource = 0u;
        u32 srcBlend = 0u;
        u32 dstBlend = 0u;
        u32 depthTest = 2u;
        u32 layerBlend = 0u;
        u32 materialVariant = 0u;
        u32 technique = 0u;
        u32 pcCompression = 0u;
        u32 macCompression = 0u;
        u32 iosCompression = 0u;
        u32 androidCompression = 0u;
        u32 previewShape = 0u;
        u32 previewEnvironment = 0u;

        u32 flags = 0;

        MaterialType materialType = MaterialType::Standard;

        hash_type renderTechnique = 0; /**< Render technique hash. */

        FixedString<WP_MAX_CLASSNAME> vertexShaderName;
        FixedString<WP_MAX_CLASSNAME> fragmentShaderName;
        FixedString<WP_MAX_CLASSNAME> geometryShaderName;
        FixedString<WP_MAX_PATH> shaderPath;

        FixedString<WP_MAX_PATH> keywords;
    };
}  // namespace workphone

#endif  // MaterialPassState_h__
