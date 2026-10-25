#ifndef MaterialPassState_h__
#define MaterialPassState_h__

#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Graphics/MaterialRenderState.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <cstring>
#include <type_traits>

namespace workphone
{
    namespace render { class MaterialPass; class MaterialShaderParametersInstance; }
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

    class WPCore_API MaterialPassStateData : public StateData, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        MaterialPassStateData();
        ~MaterialPassStateData() override;
        void lock() override;
        void unlock() override;
        MaterialRenderState::Ptr getRenderState() const;
        // Call under the write lock (or while detached on the owning thread).
        void publishRenderState();

        // Owner-side borrowed reads require the state lock/external ownership
        // synchronization and expire at the next mutation/publication. Retain
        // getRenderState() instead when values must outlive that scope.
        const MaterialRenderState &getSurfaceState() const;
        bool setSurfaceState( const MaterialRenderState &value );
        template <class T>
        bool setSurfaceValue( T MaterialRenderState::*member, const T &value )
        {
            static_assert( std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, ColourF>,
                           "Use tracked texture binding or whole-surface setters for resources" );
            if constexpr( std::is_same_v<T, u64> )
                if( member == &MaterialRenderState::revision ) return false;
            const auto &current = getSurfaceState().*member;
            bool unchanged;
            if constexpr( std::is_floating_point_v<T> || std::is_same_v<T, ColourF> )
                unchanged = std::memcmp( &current, &value, sizeof( T ) ) == 0;
            else
                unchanged = current == value;
            if( unchanged ) return false;
            writableSurfaceState().*member = value;
            return true;
        }
        SmartPtr<render::ITexture> getTextureBinding( u32 slot ) const;
        bool setTextureBinding( u32 slot, SmartPtr<render::ITexture> texture );
        size_t getPendingSurfaceStorageBytes() const;
        size_t getSurfaceAuthoringStorageBytes() const;
        ColourF getTintColour() const;
        void setTintColour( const ColourF &value );
        hash_type getRenderTechnique() const;
        void setRenderTechnique( hash_type value );

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
        size_t getStringAuthoringStorageBytes() const;

        void writeEditorSettings( Properties &floats, Properties &uints, Properties &bools,
                                  Properties &strings ) const;

        WP_CLASS_REGISTER_DECL;

    private:
        friend class MaterialPublicationScope;
        friend class render::MaterialPass;
        friend class render::MaterialShaderParametersInstance;
        SourceBudgetUniqueArray<u32> prepareShaderRenderMode(u32 value) const;
        void setTextureBindingObserver( render::IMaterialPass *owner, void ( *observer )( void *, u32 ) );
        void notifyTextureBindingChanged( u32 slot );
        MaterialRenderState &writableSurfaceState();
        const char *getAuthoringText( u32 slot ) const;
        void setAuthoringText( u32 slot, const String &value, size_t limit );
        // One allocation: 64-bit presence mask followed by packed nondefault words.
        // Floats retain their exact bits; absent slots read shared schema defaults.
        u32 getNumericWord( u32 index, u32 defaultValue ) const;
        void setNumericWord( u32 index, u32 value, u32 defaultValue );
        SourceBudgetUniqueArray<u32> m_numericAuthoring;
        // Five offsets followed by only the populated, null-terminated texts.
        SourceBudgetUniqueArray<char> m_authoring;
        MaterialRenderState::Ptr m_renderState;
        // Temporary next version only while an effective edit awaits publication.
        std::shared_ptr<MaterialRenderState> m_pendingSurface;
        struct SurfaceAuthoringData
        {
            ColourF tintColour = ColourF::White;
            hash_type renderTechnique = 0;
        };
        SourceBudgetUnique<SurfaceAuthoringData> m_surfaceAuthoring;
        WeakPtr<render::IMaterialPass> m_textureBindingOwner;
        void ( *m_textureBindingObserver )( void *, u32 ) = nullptr;
        u32 m_writeDepth = 0;
    };
}  // namespace workphone

#endif  // MaterialPassState_h__
