#ifndef MaterialPassState_h__
#define MaterialPassState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>

namespace workphone
{
    enum class MaterialFloatSetting : u32
    {
        alphaClip = 0,
        opacity = 1,
        refractionAmount = 2,
        refractionIor = 3,
        metalness = 4,
        roughness = 5,
        specular = 6,
        normalStrength = 7,
        aoStrength = 8,
        heightScale = 9,
        parallaxSteps = 10,
        emissionIntensity = 11,
        clearCoat = 12,
        clearCoatRoughness = 13,
        anisotropy = 14,
        uvTilingX = 15,
        uvTilingY = 16,
        uvOffsetX = 17,
        uvOffsetY = 18,
        uvRotation = 19,
        uvTriplanarScale = 20,
        uvAniso = 21,
        detailTilingX = 22,
        detailTilingY = 23,
        detailOffsetX = 24,
        detailOffsetY = 25,
        detailRotation = 26,
        detailStrength = 27,
        detailNormalStrength = 28,
        maxTextureSize = 29,
        renderQueue = 30,
        sortPriority = 31,
        stencilRef = 32,
        stencilReadMask = 33,
        stencilWriteMask = 34,
        layerMaskStrength = 35,
        previewExposure = 36,
        previewRotation = 37,
        Count = 38
    };

    enum class MaterialUIntSetting : u32
    {
        preset = 0,
        renderMode = 1,
        workflow = 2,
        cullMode = 3,
        uvSet = 4,
        uvProjection = 5,
        uvWrapU = 6,
        uvWrapV = 7,
        uvFilter = 8,
        detailBlendMode = 9,
        metallicSource = 10,
        roughnessSource = 11,
        aoSource = 12,
        opacitySource = 13,
        heightSource = 14,
        blendMode = 15,
        srcBlend = 16,
        dstBlend = 17,
        depthTest = 18,
        layerBlend = 19,
        materialVariant = 20,
        technique = 21,
        pcCompression = 22,
        macCompression = 23,
        iosCompression = 24,
        androidCompression = 25,
        previewShape = 26,
        previewEnvironment = 27,
        materialType = 28,
        Count = 29
    };

    enum class MaterialBoolSetting : u32
    {
        transparent = 0,
        cutout = 1,
        emissionEnabled = 2,
        refractionEnabled = 3,
        doubleSided = 4,
        receiveShadows = 5,
        castShadows = 6,
        generateMipmaps = 7,
        srgb = 8,
        normalMap = 9,
        textureStreaming = 10,
        depthWrite = 11,
        gpuInstancing = 12,
        srpBatcher = 13,
        receiveDecals = 14,
        showUvChecker = 15,
        showWireframe = 16,
        showTangents = 17,
        showMipLevels = 18,
        Count = 19
    };

    enum class MaterialStringSetting : u32
    {
        shaderPath = 0,
        keywords = 1,
        Count = 2
    };

    class WPCore_API MaterialPassStateData : public StateData
    {
    public:
        MaterialPassStateData();
        ~MaterialPassStateData() override;
        void lock() override;
        void unlock() override;
        MaterialRenderState::Ptr getRenderState() const;
        // Call under the write lock (or while detached on the owning thread).
        void publishRenderState();

        String getVertexShaderName() const;
        String getFragmentShaderName() const;
        String getGeometryShaderName() const;
        void setVertexShaderName( const String &name );
        void setFragmentShaderName( const String &name );
        void setGeometryShaderName( const String &name );


        static MaterialFloatSetting resolveFloatSetting( const String &name );
        bool setFloatSetting( MaterialFloatSetting id, f32 value );
        bool getFloatSetting( MaterialFloatSetting id, f32 &value ) const;

        static MaterialUIntSetting resolveUIntSetting( const String &name );
        bool setUIntSetting( MaterialUIntSetting id, u32 value );
        bool getUIntSetting( MaterialUIntSetting id, u32 &value ) const;

        static MaterialBoolSetting resolveBoolSetting( const String &name );
        bool setBoolSetting( MaterialBoolSetting id, bool value );
        bool getBoolSetting( MaterialBoolSetting id, bool &value ) const;

        static MaterialStringSetting resolveStringSetting( const String &name );
        bool setStringSetting( MaterialStringSetting id, const String & value );
        bool getStringSetting( MaterialStringSetting id, String &value ) const;

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

        // Accounted optional numeric authoring object, excluding allocator overhead.
        size_t getNumericAuthoringStorageBytes() const;

        void writeEditorSettings( Properties &floats, Properties &uints, Properties &bools,
                                  Properties &strings ) const;

        // Extension slots avoid traversing texture-unit objects on the draw path.
        FixedArray<SmartPtr<render::ITexture>, MaterialRenderState::TextureCount -
                   static_cast<u32>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES )> extendedTextures = {};
        WP_CLASS_REGISTER_DECL;

        ColourF tintColour = ColourF::White;
        ColourF specularColour = ColourF::White;
        ColourF ambientColour = ColourF::White;
        ColourF diffuseColour = ColourF::White;
        ColourF emissiveColour = ColourF::Black;

        FixedArray<SmartPtr<render::ITexture>, (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES>
            textures = {};

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

        u32 blendMode = 0; /**< Blending mode (implementation-specific). */
        u32 cullMode = 2;  /**< Face culling mode (none=1, clockwise=2, anticlockwise=3). */
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

        hash_type renderTechnique = 0; /**< Render technique hash. */

    private:
        friend class MaterialPublicationScope;
        struct AuthoringData
        {
            FixedString<WP_MAX_CLASSNAME> vertexShaderName, fragmentShaderName, geometryShaderName;
            FixedString<WP_MAX_PATH> shaderPath, keywords;
        };
        // Values absent from the runtime snapshot share one lazy schema-default block.
        struct NumericAuthoringData
        {
            FixedArray<f32, 20> floats = {};
            FixedArray<u32, 17> uints = {};
        };
        NumericAuthoringData &ensureNumericAuthoring();
        void releaseDefaultNumericAuthoring();
        std::unique_ptr<NumericAuthoringData> m_numericAuthoring;
        std::unique_ptr<AuthoringData> m_authoring;
        MaterialRenderState::Ptr m_renderState;
        u32 m_writeDepth = 0;
    };
}  // namespace workphone

#endif  // MaterialPassState_h__
