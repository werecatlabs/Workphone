#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>
#include <cstring>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    using namespace render;

    namespace
    {
        struct PublicationQueue
        {
            u32 depth = 0;
            bool flushing = false;
            std::array<MaterialPassStateData *, 256> recent{};
            std::vector<SmartPtr<MaterialPassStateData>> pending;
        };
        thread_local PublicationQueue publicationQueue;

        template <class T> bool sameMaterialValue( const T &a, const T &b ) { return a == b; }
        bool sameMaterialValue( f32 a, f32 b ) { return std::memcmp( &a, &b, sizeof( a ) ) == 0; }
        bool sameMaterialValue( const ColourF &a, const ColourF &b )
        {
            return sameMaterialValue( a.r, b.r ) && sameMaterialValue( a.g, b.g ) &&
                   sameMaterialValue( a.b, b.b ) && sameMaterialValue( a.a, b.a );
        }

        template <class T>
        struct EditorField
        {
            const char *name;
            T MaterialPassStateData::*member;
        };

        struct EditorBoolField
        {
            const char *name;
            u32 flag;
        };

        const EditorField<f32> FloatFields[] = {
            { "alphaClip", &MaterialPassStateData::alphaClip },
            { "opacity", &MaterialPassStateData::opacity },
            { "refractionAmount", &MaterialPassStateData::refractionAmount },
            { "refractionIor", &MaterialPassStateData::refractionIor },
            { "metalness", &MaterialPassStateData::metalness },
            { "roughness", &MaterialPassStateData::roughness },
            { "specular", &MaterialPassStateData::specular },
            { "normalStrength", &MaterialPassStateData::normalStrength },
            { "aoStrength", &MaterialPassStateData::aoStrength },
            { "heightScale", &MaterialPassStateData::heightScale },
            { "parallaxSteps", &MaterialPassStateData::parallaxSteps },
            { "emissionIntensity", &MaterialPassStateData::emissionIntensity },
            { "clearCoat", &MaterialPassStateData::clearCoat },
            { "clearCoatRoughness", &MaterialPassStateData::clearCoatRoughness },
            { "anisotropy", &MaterialPassStateData::anisotropy },
            { "uvTilingX", &MaterialPassStateData::uvTilingX },
            { "uvTilingY", &MaterialPassStateData::uvTilingY },
            { "uvOffsetX", &MaterialPassStateData::uvOffsetX },
            { "uvOffsetY", &MaterialPassStateData::uvOffsetY },
            { "uvRotation", &MaterialPassStateData::uvRotation },
            { "uvTriplanarScale", &MaterialPassStateData::uvTriplanarScale },
            { "uvAniso", &MaterialPassStateData::uvAniso },
            { "detailTilingX", &MaterialPassStateData::detailTilingX },
            { "detailTilingY", &MaterialPassStateData::detailTilingY },
            { "detailOffsetX", &MaterialPassStateData::detailOffsetX },
            { "detailOffsetY", &MaterialPassStateData::detailOffsetY },
            { "detailRotation", &MaterialPassStateData::detailRotation },
            { "detailStrength", &MaterialPassStateData::detailStrength },
            { "detailNormalStrength", &MaterialPassStateData::detailNormalStrength },
            { "maxTextureSize", &MaterialPassStateData::maxTextureSize },
            { "renderQueue", &MaterialPassStateData::renderQueue },
            { "sortPriority", &MaterialPassStateData::sortPriority },
            { "stencilRef", &MaterialPassStateData::stencilRef },
            { "stencilReadMask", &MaterialPassStateData::stencilReadMask },
            { "stencilWriteMask", &MaterialPassStateData::stencilWriteMask },
            { "layerMaskStrength", &MaterialPassStateData::layerMaskStrength },
            { "previewExposure", &MaterialPassStateData::previewExposure },
            { "previewRotation", &MaterialPassStateData::previewRotation },
        };

        const EditorField<u32> UIntFields[] = {
            { "preset", &MaterialPassStateData::preset },
            { "renderMode", &MaterialPassStateData::renderMode },
            { "workflow", &MaterialPassStateData::workflow },
            { "cullMode", &MaterialPassStateData::cullMode },
            { "uvSet", &MaterialPassStateData::uvSet },
            { "uvProjection", &MaterialPassStateData::uvProjection },
            { "uvWrapU", &MaterialPassStateData::uvWrapU },
            { "uvWrapV", &MaterialPassStateData::uvWrapV },
            { "uvFilter", &MaterialPassStateData::uvFilter },
            { "detailBlendMode", &MaterialPassStateData::detailBlendMode },
            { "metallicSource", &MaterialPassStateData::metallicSource },
            { "roughnessSource", &MaterialPassStateData::roughnessSource },
            { "aoSource", &MaterialPassStateData::aoSource },
            { "opacitySource", &MaterialPassStateData::opacitySource },
            { "heightSource", &MaterialPassStateData::heightSource },
            { "blendMode", &MaterialPassStateData::blendMode },
            { "srcBlend", &MaterialPassStateData::srcBlend },
            { "dstBlend", &MaterialPassStateData::dstBlend },
            { "depthTest", &MaterialPassStateData::depthTest },
            { "layerBlend", &MaterialPassStateData::layerBlend },
            { "materialVariant", &MaterialPassStateData::materialVariant },
            { "technique", &MaterialPassStateData::technique },
            { "pcCompression", &MaterialPassStateData::pcCompression },
            { "macCompression", &MaterialPassStateData::macCompression },
            { "iosCompression", &MaterialPassStateData::iosCompression },
            { "androidCompression", &MaterialPassStateData::androidCompression },
            { "previewShape", &MaterialPassStateData::previewShape },
            { "previewEnvironment", &MaterialPassStateData::previewEnvironment },
        };

        const EditorBoolField BoolFields[] = {
            { "transparent", transparentFlag },
            { "cutout", cutoutFlag },
            { "emissionEnabled", emissionEnabledFlag },
            { "refractionEnabled", refractionEnabledFlag },
            { "doubleSided", doubleSidedFlag },
            { "receiveShadows", receiveShadowsFlag },
            { "castShadows", castShadowsFlag },
            { "generateMipmaps", generateMipmapsFlag },
            { "srgb", srgbFlag },
            { "normalMap", normalMapFlag },
            { "textureStreaming", textureStreamingFlag },
            { "depthWrite", depthWriteFlag },
            { "gpuInstancing", gpuInstancingFlag },
            { "srpBatcher", srpBatcherFlag },
            { "receiveDecals", receiveDecalsFlag },
            { "showUvChecker", showUvCheckerFlag },
            { "showWireframe", showWireframeFlag },
            { "showTangents", showTangentsFlag },
            { "showMipLevels", showMipLevelsFlag },
        };

        const char *StringFields[] = { "shaderPath", "keywords" };

        template <class T, size_t N>
        void writeEditorFields( const MaterialPassStateData &state, Properties &properties,
                                const EditorField<T> ( &fields )[N] )
        {
            for( const auto &field : fields )
            {
                properties.setProperty( field.name, state.*( field.member ) );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialPassStateData, StateData );

    MaterialPassStateData::MaterialPassStateData() : StateData( MaterialPassStateData::typeInfo() )
    {
        setFlag( receiveShadowsFlag, true );
        setFlag( castShadowsFlag, true );
        publishRenderState();
    }

    MaterialPassStateData::~MaterialPassStateData() = default;


    String MaterialPassStateData::getVertexShaderName() const
    { return m_authoring ? String( m_authoring->vertexShaderName.c_str() ) : String(); }
    void MaterialPassStateData::setVertexShaderName( const String &name )
    {
        if( name.empty() && !m_authoring ) return;
        if( !m_authoring ) m_authoring = std::make_unique<AuthoringData>();
        m_authoring->vertexShaderName = name.c_str();
    }

    String MaterialPassStateData::getFragmentShaderName() const
    { return m_authoring ? String( m_authoring->fragmentShaderName.c_str() ) : String(); }
    void MaterialPassStateData::setFragmentShaderName( const String &name )
    {
        if( name.empty() && !m_authoring ) return;
        if( !m_authoring ) m_authoring = std::make_unique<AuthoringData>();
        m_authoring->fragmentShaderName = name.c_str();
    }

    String MaterialPassStateData::getGeometryShaderName() const
    { return m_authoring ? String( m_authoring->geometryShaderName.c_str() ) : String(); }
    void MaterialPassStateData::setGeometryShaderName( const String &name )
    {
        if( name.empty() && !m_authoring ) return;
        if( !m_authoring ) m_authoring = std::make_unique<AuthoringData>();
        m_authoring->geometryShaderName = name.c_str();
    }

    MaterialPublicationScope::MaterialPublicationScope()
    { ++publicationQueue.depth; }

    MaterialPublicationScope::~MaterialPublicationScope() noexcept
    {
        try { commit(); }
        catch(...) { std::fputs("Material snapshot publication failed at update exit.\n", stderr); }
    }

    bool MaterialPublicationScope::isActive() noexcept
    { return publicationQueue.depth != 0 && !publicationQueue.flushing; }

    size_t MaterialPublicationScope::pendingEntryCount() noexcept
    { return publicationQueue.pending.size(); }

    size_t MaterialPublicationScope::retainedStorageBytes() noexcept
    {
        return sizeof(PublicationQueue) + publicationQueue.pending.capacity() * sizeof(SmartPtr<MaterialPassStateData>);
    }

    bool MaterialPublicationScope::defer(MaterialPassStateData &state) noexcept
    {
        // Detached/stack instances and constructor publication stay immediate.
        if(!isActive() || state.getReferences() == 0) return false;
        const auto address = reinterpret_cast<size_t>(&state);
        const auto slot = ((address >> 4) ^ (address >> 12)) & 255;
        if(publicationQueue.recent[slot] == &state) return true;
        try
        {
            publicationQueue.pending.push_back(state.getSharedFromThis<MaterialPassStateData>());
            publicationQueue.recent[slot] = &state;
            return true;
        }
        catch(...) { return false; } // Preserve the existing immediate path on queue allocation failure.
    }

    void MaterialPublicationScope::commit()
    {
        if(!m_active) return;
        m_active = false;
        if(--publicationQueue.depth || publicationQueue.flushing || publicationQueue.pending.empty()) return;
        auto pending = std::move(publicationQueue.pending);
        publicationQueue.recent.fill(nullptr);
        publicationQueue.flushing = true;
        struct ReleaseQueue
        {
            std::vector<SmartPtr<MaterialPassStateData>> &pending;
            ~ReleaseQueue()
            {
                publicationQueue.flushing = false;
                pending.clear(); // Release temporary owners outside state locks, including on exceptions.
                // Reuse ordinary updates; discard unusually large burst capacity.
                if(pending.capacity() <= 4096) publicationQueue.pending.swap(pending);
            }
        } release{pending};
        std::sort(pending.begin(), pending.end(), [](const auto &a, const auto &b) {
            return std::less<MaterialPassStateData *>{}(a.get(), b.get());
        });
        pending.erase(std::unique(pending.begin(), pending.end(), [](const auto &a, const auto &b) {
            return a.get() == b.get();
        }), pending.end());
        for(const auto &state : pending)
        {
            // Bypass the write-depth override: publication itself must not enqueue
            // another write, and a failed snapshot allocation must still unlock.
            struct StateLock {
                StateData &state;
                explicit StateLock(StateData &value) : state(value) { state.StateData::lock(); }
                ~StateLock() { state.StateData::unlock(); }
            } lock{*state};
            state->publishRenderState();
        }
    }

    void MaterialPassStateData::lock()
    { StateData::lock(); ++m_writeDepth; }

    void MaterialPassStateData::unlock()
    {
        if( m_writeDepth == 1 ) publishRenderState();
        --m_writeDepth;
        StateData::unlock();
    }

    MaterialRenderState::Ptr MaterialPassStateData::getRenderState() const
    { return std::atomic_load( &m_renderState ); }

    void MaterialPassStateData::publishRenderState()
    {
        if( MaterialPublicationScope::defer( *this ) ) return;
        auto previous = std::atomic_load( &m_renderState );
        bool changed = !previous;
        changed = changed || !sameMaterialValue( specularColour, previous->specularColour );
        changed = changed || !sameMaterialValue( ambientColour, previous->ambientColour );
        changed = changed || !sameMaterialValue( diffuseColour, previous->diffuseColour );
        changed = changed || !sameMaterialValue( emissiveColour, previous->emissiveColour );
        changed = changed || !sameMaterialValue( alphaClip, previous->alphaClip );
        changed = changed || !sameMaterialValue( opacity, previous->opacity );
        changed = changed || !sameMaterialValue( refractionAmount, previous->refractionAmount );
        changed = changed || !sameMaterialValue( refractionIor, previous->refractionIor );
        changed = changed || !sameMaterialValue( metalness, previous->metalness );
        changed = changed || !sameMaterialValue( roughness, previous->roughness );
        changed = changed || !sameMaterialValue( normalStrength, previous->normalStrength );
        changed = changed || !sameMaterialValue( aoStrength, previous->aoStrength );
        changed = changed || !sameMaterialValue( emissionIntensity, previous->emissionIntensity );
        changed = changed || !sameMaterialValue( clearCoat, previous->clearCoat );
        changed = changed || !sameMaterialValue( clearCoatRoughness, previous->clearCoatRoughness );
        changed = changed || !sameMaterialValue( uvTilingX, previous->uvTilingX );
        changed = changed || !sameMaterialValue( uvTilingY, previous->uvTilingY );
        changed = changed || !sameMaterialValue( uvOffsetX, previous->uvOffsetX );
        changed = changed || !sameMaterialValue( uvOffsetY, previous->uvOffsetY );
        changed = changed || !sameMaterialValue( uvRotation, previous->uvRotation );
        changed = changed || !sameMaterialValue( uvTriplanarScale, previous->uvTriplanarScale );
        changed = changed || !sameMaterialValue( uvAniso, previous->uvAniso );
        changed = changed || !sameMaterialValue( blendMode, previous->blendMode );
        changed = changed || !sameMaterialValue( cullMode, previous->cullMode );
        changed = changed || !sameMaterialValue( uvProjection, previous->uvProjection );
        changed = changed || !sameMaterialValue( uvWrapU, previous->uvWrapU );
        changed = changed || !sameMaterialValue( uvWrapV, previous->uvWrapV );
        changed = changed || !sameMaterialValue( uvFilter, previous->uvFilter );
        changed = changed || !sameMaterialValue( metallicSource, previous->metallicSource );
        changed = changed || !sameMaterialValue( roughnessSource, previous->roughnessSource );
        changed = changed || !sameMaterialValue( aoSource, previous->aoSource );
        changed = changed || !sameMaterialValue( opacitySource, previous->opacitySource );
        changed = changed || !sameMaterialValue( depthTest, previous->depthTest );
        changed = changed || !sameMaterialValue( flags, previous->flags );
        changed = changed || !sameMaterialValue( materialType, previous->materialType );
        for( u32 i = 0; !changed && i < MaterialRenderState::TextureCount; ++i )
        {
            const auto &texture = i < textures.size() ? textures[i] : extendedTextures[i - textures.size()];
            changed = texture != previous->textures[i];
        }
        if( !changed ) return;
        auto next = std::make_shared<MaterialRenderState>();
        next->specularColour = specularColour;
        next->ambientColour = ambientColour;
        next->diffuseColour = diffuseColour;
        next->emissiveColour = emissiveColour;
        next->alphaClip = alphaClip;
        next->opacity = opacity;
        next->refractionAmount = refractionAmount;
        next->refractionIor = refractionIor;
        next->metalness = metalness;
        next->roughness = roughness;
        next->normalStrength = normalStrength;
        next->aoStrength = aoStrength;
        next->emissionIntensity = emissionIntensity;
        next->clearCoat = clearCoat;
        next->clearCoatRoughness = clearCoatRoughness;
        next->uvTilingX = uvTilingX;
        next->uvTilingY = uvTilingY;
        next->uvOffsetX = uvOffsetX;
        next->uvOffsetY = uvOffsetY;
        next->uvRotation = uvRotation;
        next->uvTriplanarScale = uvTriplanarScale;
        next->uvAniso = uvAniso;
        next->blendMode = blendMode;
        next->cullMode = cullMode;
        next->uvProjection = uvProjection;
        next->uvWrapU = uvWrapU;
        next->uvWrapV = uvWrapV;
        next->uvFilter = uvFilter;
        next->metallicSource = metallicSource;
        next->roughnessSource = roughnessSource;
        next->aoSource = aoSource;
        next->opacitySource = opacitySource;
        next->depthTest = depthTest;
        next->flags = flags;
        next->materialType = materialType;
        for( u32 i = 0; i < MaterialRenderState::TextureCount; ++i )
            next->textures[i] = i < textures.size() ? textures[i] : extendedTextures[i - textures.size()];
        next->revision = previous ? previous->revision + 1 : 1;
        std::atomic_store( &m_renderState, MaterialRenderState::Ptr( std::move( next ) ) );
    }


    MaterialFloatSetting MaterialPassStateData::resolveFloatSetting( const String &name )
    {
        for( u32 i = 0; i < sizeof( FloatFields ) / sizeof( FloatFields[0] ); ++i )
            if( name == FloatFields[i].name ) return static_cast<MaterialFloatSetting>( i );
        return MaterialFloatSetting::Count;
    }

    bool MaterialPassStateData::setFloatSetting( MaterialFloatSetting id, f32 value )
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( FloatFields ) / sizeof( FloatFields[0] ) ) return false;
        this->*( FloatFields[i].member ) = value;
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getFloatSetting( MaterialFloatSetting id, f32 &value ) const
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( FloatFields ) / sizeof( FloatFields[0] ) ) return false;
        value = this->*( FloatFields[i].member );
        return true;
    }

    bool MaterialPassStateData::setEditorFloat( const String &name, f32 value )
    { return setFloatSetting( resolveFloatSetting( name ), value ); }

    bool MaterialPassStateData::getEditorFloat( const String &name, f32 &value ) const
    { return getFloatSetting( resolveFloatSetting( name ), value ); }

    MaterialUIntSetting MaterialPassStateData::resolveUIntSetting( const String &name )
    {
        if( name == "materialType" ) return MaterialUIntSetting::materialType;
        for( u32 i = 0; i < sizeof( UIntFields ) / sizeof( UIntFields[0] ); ++i )
            if( name == UIntFields[i].name ) return static_cast<MaterialUIntSetting>( i );
        return MaterialUIntSetting::Count;
    }

    bool MaterialPassStateData::setUIntSetting( MaterialUIntSetting id, u32 value )
    {
        if( id == MaterialUIntSetting::materialType )
        {
            if( value >= static_cast<u32>( MaterialType::Count ) ) return false;
            materialType = static_cast<MaterialType>( value );
            if( !m_writeDepth ) publishRenderState();
            return true;
        }
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( UIntFields ) / sizeof( UIntFields[0] ) ) return false;
        this->*( UIntFields[i].member ) = value;
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getUIntSetting( MaterialUIntSetting id, u32 &value ) const
    {
        if( id == MaterialUIntSetting::materialType )
        { value = static_cast<u32>( materialType ); return true; }
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( UIntFields ) / sizeof( UIntFields[0] ) ) return false;
        value = this->*( UIntFields[i].member );
        return true;
    }

    bool MaterialPassStateData::setEditorUInt( const String &name, u32 value )
    { return setUIntSetting( resolveUIntSetting( name ), value ); }

    bool MaterialPassStateData::getEditorUInt( const String &name, u32 &value ) const
    { return getUIntSetting( resolveUIntSetting( name ), value ); }

    MaterialBoolSetting MaterialPassStateData::resolveBoolSetting( const String &name )
    {
        for( u32 i = 0; i < sizeof( BoolFields ) / sizeof( BoolFields[0] ); ++i )
            if( name == BoolFields[i].name ) return static_cast<MaterialBoolSetting>( i );
        return MaterialBoolSetting::Count;
    }

    bool MaterialPassStateData::setBoolSetting( MaterialBoolSetting id, bool value )
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( BoolFields ) / sizeof( BoolFields[0] ) ) return false;
        setFlag( BoolFields[i].flag, value );
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getBoolSetting( MaterialBoolSetting id, bool &value ) const
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( BoolFields ) / sizeof( BoolFields[0] ) ) return false;
        value = getFlag( BoolFields[i].flag );
        return true;
    }

    bool MaterialPassStateData::setEditorBool( const String &name, bool value )
    { return setBoolSetting( resolveBoolSetting( name ), value ); }

    bool MaterialPassStateData::getEditorBool( const String &name, bool &value ) const
    { return getBoolSetting( resolveBoolSetting( name ), value ); }

    MaterialStringSetting MaterialPassStateData::resolveStringSetting( const String &name )
    {
        for( u32 i = 0; i < sizeof( StringFields ) / sizeof( StringFields[0] ); ++i )
            if( name == StringFields[i] ) return static_cast<MaterialStringSetting>( i );
        return MaterialStringSetting::Count;
    }

    bool MaterialPassStateData::setStringSetting( MaterialStringSetting id, const String & value )
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( StringFields ) / sizeof( StringFields[0] ) ) return false;
        if( value.empty() && !m_authoring ) return true;
        if( !m_authoring ) m_authoring = std::make_unique<AuthoringData>();
        auto &target = id == MaterialStringSetting::shaderPath ? m_authoring->shaderPath : m_authoring->keywords;
        target = value.c_str();
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getStringSetting( MaterialStringSetting id, String &value ) const
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( StringFields ) / sizeof( StringFields[0] ) ) return false;
        value = !m_authoring ? String() : String( id == MaterialStringSetting::shaderPath ?
                  m_authoring->shaderPath.c_str() : m_authoring->keywords.c_str() );
        return true;
    }

    bool MaterialPassStateData::setEditorString( const String &name, const String & value )
    { return setStringSetting( resolveStringSetting( name ), value ); }

    bool MaterialPassStateData::getEditorString( const String &name, String &value ) const
    { return getStringSetting( resolveStringSetting( name ), value ); }

    void MaterialPassStateData::setFlag( u32 flag, bool value )
    {
        flags = BitUtil::setFlagValue( flags, flag, value );
        if( !m_writeDepth ) publishRenderState();
    }

    bool MaterialPassStateData::getFlag( u32 flag ) const
    {
        return BitUtil::getFlagValue( flags, flag );
    }

    void MaterialPassStateData::writeEditorSettings( Properties &floats, Properties &uints,
                                                     Properties &bools, Properties &strings ) const
    {
        writeEditorFields( *this, floats, FloatFields );
        writeEditorFields( *this, uints, UIntFields );
        for( const auto &field : BoolFields )
        {
            bools.setProperty( field.name, getFlag( field.flag ) );
        }
        for( u32 i = 0; i < sizeof( StringFields ) / sizeof( StringFields[0] ); ++i )
        {
            String value;
            getStringSetting( static_cast<MaterialStringSetting>( i ), value );
            strings.setProperty( StringFields[i], value );
        }
    }

}  // namespace workphone
