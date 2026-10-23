#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <vector>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <limits>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    using namespace render;

    namespace
    {
        struct PublicationQueue;
        thread_local PublicationQueue *activePublicationQueue = nullptr;
        struct PublicationQueue
        {
            PublicationQueue() { activePublicationQueue = this; }
            ~PublicationQueue() { activePublicationQueue = nullptr; }
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

        bool sameSurface( const MaterialRenderState &a, const MaterialRenderState &b )
        {
            if( !sameMaterialValue( a.specularColour, b.specularColour ) ) return false;
            if( !sameMaterialValue( a.ambientColour, b.ambientColour ) ) return false;
            if( !sameMaterialValue( a.diffuseColour, b.diffuseColour ) ) return false;
            if( !sameMaterialValue( a.emissiveColour, b.emissiveColour ) ) return false;
            if( !sameMaterialValue( a.alphaClip, b.alphaClip ) ) return false;
            if( !sameMaterialValue( a.opacity, b.opacity ) ) return false;
            if( !sameMaterialValue( a.refractionAmount, b.refractionAmount ) ) return false;
            if( !sameMaterialValue( a.refractionIor, b.refractionIor ) ) return false;
            if( !sameMaterialValue( a.metalness, b.metalness ) ) return false;
            if( !sameMaterialValue( a.roughness, b.roughness ) ) return false;
            if( !sameMaterialValue( a.normalStrength, b.normalStrength ) ) return false;
            if( !sameMaterialValue( a.aoStrength, b.aoStrength ) ) return false;
            if( !sameMaterialValue( a.emissionIntensity, b.emissionIntensity ) ) return false;
            if( !sameMaterialValue( a.clearCoat, b.clearCoat ) ) return false;
            if( !sameMaterialValue( a.clearCoatRoughness, b.clearCoatRoughness ) ) return false;
            if( !sameMaterialValue( a.uvTilingX, b.uvTilingX ) ) return false;
            if( !sameMaterialValue( a.uvTilingY, b.uvTilingY ) ) return false;
            if( !sameMaterialValue( a.uvOffsetX, b.uvOffsetX ) ) return false;
            if( !sameMaterialValue( a.uvOffsetY, b.uvOffsetY ) ) return false;
            if( !sameMaterialValue( a.uvRotation, b.uvRotation ) ) return false;
            if( !sameMaterialValue( a.uvTriplanarScale, b.uvTriplanarScale ) ) return false;
            if( !sameMaterialValue( a.uvAniso, b.uvAniso ) ) return false;
            if( !sameMaterialValue( a.blendMode, b.blendMode ) ) return false;
            if( !sameMaterialValue( a.cullMode, b.cullMode ) ) return false;
            if( !sameMaterialValue( a.uvProjection, b.uvProjection ) ) return false;
            if( !sameMaterialValue( a.uvWrapU, b.uvWrapU ) ) return false;
            if( !sameMaterialValue( a.uvWrapV, b.uvWrapV ) ) return false;
            if( !sameMaterialValue( a.uvFilter, b.uvFilter ) ) return false;
            if( !sameMaterialValue( a.metallicSource, b.metallicSource ) ) return false;
            if( !sameMaterialValue( a.roughnessSource, b.roughnessSource ) ) return false;
            if( !sameMaterialValue( a.aoSource, b.aoSource ) ) return false;
            if( !sameMaterialValue( a.opacitySource, b.opacitySource ) ) return false;
            if( !sameMaterialValue( a.depthTest, b.depthTest ) ) return false;
            if( !sameMaterialValue( a.flags, b.flags ) ) return false;
            if( !sameMaterialValue( a.materialType, b.materialType ) ) return false;
            for( u32 i = 0; i < MaterialRenderState::TextureCount; ++i )
                if( a.textures[i] != b.textures[i] ) return false;
            return true;
        }

        static_assert( static_cast<u32>( MaterialStringSetting::Count ) == 2, "packed editor string schema" );
        static_assert( ( u64( WP_MAX_CLASSNAME ) + 1 ) * 3 + ( u64( WP_MAX_PATH ) + 1 ) * 2 +
                           sizeof( u32 ) * 5 <= std::numeric_limits<u32>::max(),
                       "packed authoring offsets fit configured string capacities" );

        constexpr u32 ColdFloatCount = 20;
        constexpr u32 ColdUIntCount = 17;
        static_assert( ColdFloatCount + ColdUIntCount <= 64, "numeric presence mask capacity" );
        static_assert( sizeof( f32 ) == sizeof( u32 ), "numeric authoring word size" );

        u32 floatWord( f32 value )
        {
            u32 result;
            std::memcpy( &result, &value, sizeof( result ) );
            return result;
        }
        f32 wordFloat( u32 value )
        {
            f32 result;
            std::memcpy( &result, &value, sizeof( result ) );
            return result;
        }
        u64 numericMask( const u32 *words )
        {
            u64 result = 0;
            if( words ) std::memcpy( &result, words, sizeof( result ) );
            return result;
        }
        u32 numericWordCount( u64 mask )
        {
            u32 count = 0;
            while( mask ) { mask &= mask - 1; ++count; }
            return count;
        }

        template <class T>
        struct EditorField
        {
            const char *name;
            T MaterialRenderState::*member;
            u32 coldIndex = 0;
            T defaultValue = {};
        };

        struct EditorBoolField
        {
            const char *name;
            u32 flag;
        };

        constexpr EditorField<f32> FloatFields[] = {
            { "alphaClip", &MaterialRenderState::alphaClip },
            { "opacity", &MaterialRenderState::opacity },
            { "refractionAmount", &MaterialRenderState::refractionAmount },
            { "refractionIor", &MaterialRenderState::refractionIor },
            { "metalness", &MaterialRenderState::metalness },
            { "roughness", &MaterialRenderState::roughness },
            { "specular", nullptr, 9u, 0.05f },
            { "normalStrength", &MaterialRenderState::normalStrength },
            { "aoStrength", &MaterialRenderState::aoStrength },
            { "heightScale", nullptr, 10u, 0.02f },
            { "parallaxSteps", nullptr, 11u, 16.0f },
            { "emissionIntensity", &MaterialRenderState::emissionIntensity },
            { "clearCoat", &MaterialRenderState::clearCoat },
            { "clearCoatRoughness", &MaterialRenderState::clearCoatRoughness },
            { "anisotropy", nullptr, 12u, 0.0f },
            { "uvTilingX", &MaterialRenderState::uvTilingX },
            { "uvTilingY", &MaterialRenderState::uvTilingY },
            { "uvOffsetX", &MaterialRenderState::uvOffsetX },
            { "uvOffsetY", &MaterialRenderState::uvOffsetY },
            { "uvRotation", &MaterialRenderState::uvRotation },
            { "uvTriplanarScale", &MaterialRenderState::uvTriplanarScale },
            { "uvAniso", &MaterialRenderState::uvAniso },
            { "detailTilingX", nullptr, 13u, 1.0f },
            { "detailTilingY", nullptr, 14u, 1.0f },
            { "detailOffsetX", nullptr, 15u, 0.0f },
            { "detailOffsetY", nullptr, 16u, 0.0f },
            { "detailRotation", nullptr, 17u, 0.0f },
            { "detailStrength", nullptr, 18u, 0.0f },
            { "detailNormalStrength", nullptr, 19u, 1.0f },
            { "maxTextureSize", nullptr, 0u, 2048.0f },
            { "renderQueue", nullptr, 1u, 2000.0f },
            { "sortPriority", nullptr, 2u, 0.0f },
            { "stencilRef", nullptr, 3u, 0.0f },
            { "stencilReadMask", nullptr, 4u, 255.0f },
            { "stencilWriteMask", nullptr, 5u, 255.0f },
            { "layerMaskStrength", nullptr, 6u, 1.0f },
            { "previewExposure", nullptr, 7u, 1.0f },
            { "previewRotation", nullptr, 8u, 0.0f },
        };

        constexpr EditorField<u32> UIntFields[] = {
            { "preset", nullptr, 8u, 0u },
            { "renderMode", nullptr, 9u, 0u },
            { "workflow", nullptr, 10u, 0u },
            { "cullMode", &MaterialRenderState::cullMode },
            { "uvSet", nullptr, 11u, 0u },
            { "uvProjection", &MaterialRenderState::uvProjection },
            { "uvWrapU", &MaterialRenderState::uvWrapU },
            { "uvWrapV", &MaterialRenderState::uvWrapV },
            { "uvFilter", &MaterialRenderState::uvFilter },
            { "detailBlendMode", nullptr, 12u, 0u },
            { "metallicSource", &MaterialRenderState::metallicSource },
            { "roughnessSource", &MaterialRenderState::roughnessSource },
            { "aoSource", &MaterialRenderState::aoSource },
            { "opacitySource", &MaterialRenderState::opacitySource },
            { "heightSource", nullptr, 13u, 0u },
            { "blendMode", &MaterialRenderState::blendMode },
            { "srcBlend", nullptr, 14u, 0u },
            { "dstBlend", nullptr, 15u, 0u },
            { "depthTest", &MaterialRenderState::depthTest },
            { "layerBlend", nullptr, 16u, 0u },
            { "materialVariant", nullptr, 0u, 0u },
            { "technique", nullptr, 1u, 0u },
            { "pcCompression", nullptr, 2u, 0u },
            { "macCompression", nullptr, 3u, 0u },
            { "iosCompression", nullptr, 4u, 0u },
            { "androidCompression", nullptr, 5u, 0u },
            { "previewShape", nullptr, 6u, 0u },
            { "previewEnvironment", nullptr, 7u, 0u },
        };

        template <class T, size_t N>
        constexpr bool validColdIndices( const EditorField<T> ( &fields )[N], u32 count )
        {
            u64 seen = 0;
            for( const auto &field : fields )
                if( !field.member )
                {
                    if( field.coldIndex >= count ) return false;
                    const auto bit = u64( 1 ) << field.coldIndex;
                    if( seen & bit ) return false;
                    seen |= bit;
                }
            return seen == ( ( u64( 1 ) << count ) - 1 );
        }
        static_assert( validColdIndices( FloatFields, ColdFloatCount ), "unique packed float slots" );
        static_assert( validColdIndices( UIntFields, ColdUIntCount ), "unique packed uint slots" );

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
            for( u32 i = 0; i < N; ++i )
            {
                T value{};
                if constexpr( std::is_same_v<T, f32> )
                    state.getFloatSetting( static_cast<MaterialFloatSetting>( i ), value );
                else
                    state.getUIntSetting( static_cast<MaterialUIntSetting>( i ), value );
                properties.setProperty( fields[i].name, value );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialPassStateData, StateData );

    MaterialPassStateData::MaterialPassStateData() : StateData( MaterialPassStateData::typeInfo() )
    {
        auto initial = std::make_shared<MaterialRenderState>();
        initial->flags = receiveShadowsFlag | castShadowsFlag;
        initial->revision = 1;
        m_renderState = std::move( initial );
    }

    MaterialPassStateData::~MaterialPassStateData()
    {
        // Stack/explicitly destroyed states can end before their update scope.
        MaterialPublicationScope::forget(*this);
    }


    const char *MaterialPassStateData::getAuthoringText( u32 slot ) const
    {
        u32 offset = 0;
        if( m_authoring ) std::memcpy( &offset, m_authoring.get() + slot * sizeof( offset ), sizeof( offset ) );
        return offset ? m_authoring.get() + offset : "";
    }

    void MaterialPassStateData::setAuthoringText( u32 slot, const String &value, size_t limit )
    {
        // FixedString's C-string assignment contract: embedded null ends the value.
        const auto *text = value.c_str();
        const auto length = std::strlen( text );
        WP_ASSERT( length < limit );
        if( length > limit ) throw std::length_error( "FixedStringBase: length exceeds maximum size" );
        if( std::strcmp( getAuthoringText( slot ), text ) == 0 ) return;
        const char *texts[5];
        size_t bytes = sizeof( u32 ) * 5;
        for( u32 i = 0; i < 5; ++i )
        {
            texts[i] = i == slot ? text : getAuthoringText( i );
            if( texts[i][0] ) bytes += std::strlen( texts[i] ) + 1;
        }
        if( bytes == sizeof( u32 ) * 5 ) { m_authoring.reset(); return; }
        auto next = std::make_unique<char[]>( bytes );
        size_t cursor = sizeof( u32 ) * 5;
        for( u32 i = 0; i < 5; ++i )
        {
            const u32 offset = texts[i][0] ? static_cast<u32>( cursor ) : 0;
            std::memcpy( next.get() + i * sizeof( offset ), &offset, sizeof( offset ) );
            if( offset )
            {
                const auto count = std::strlen( texts[i] ) + 1;
                std::memcpy( next.get() + cursor, texts[i], count );
                cursor += count;
            }
        }
        // Retain the original buffer until all allocation/copy work succeeds.
        m_authoring = std::move( next );
    }

    size_t MaterialPassStateData::getStringAuthoringStorageBytes() const
    {
        if( !m_authoring ) return 0;
        size_t bytes = sizeof( u32 ) * 5;
        for( u32 i = 0; i < 5; ++i )
        {
            const auto *text = getAuthoringText( i );
            if( text[0] ) bytes += std::strlen( text ) + 1;
        }
        return bytes;
    }

    String MaterialPassStateData::getVertexShaderName() const
    { return String( getAuthoringText( 0 ) ); }
    void MaterialPassStateData::setVertexShaderName( const String &name )
    { setAuthoringText( 0, name, WP_MAX_CLASSNAME ); }

    String MaterialPassStateData::getFragmentShaderName() const
    { return String( getAuthoringText( 1 ) ); }
    void MaterialPassStateData::setFragmentShaderName( const String &name )
    { setAuthoringText( 1, name, WP_MAX_CLASSNAME ); }

    String MaterialPassStateData::getGeometryShaderName() const
    { return String( getAuthoringText( 2 ) ); }
    void MaterialPassStateData::setGeometryShaderName( const String &name )
    { setAuthoringText( 2, name, WP_MAX_CLASSNAME ); }

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
        return sizeof(PublicationQueue) + sizeof(activePublicationQueue) +
            publicationQueue.pending.capacity() * sizeof(SmartPtr<MaterialPassStateData>);
    }

    bool MaterialPublicationScope::defer(MaterialPassStateData &state) noexcept
    {
        if(!isActive()) return false;
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

    void MaterialPublicationScope::forget(MaterialPassStateData &state) noexcept
    {
        // Ordinary last-owner destruction cannot be present in the owning queue.
        // The trivial TLS pointer also guards late factory/TLS teardown.
        if(state.getReferences() <= 0 || !activePublicationQueue) return;
        auto &queue = *activePublicationQueue;
        auto &pending = queue.pending;
        if(pending.empty()) return;
        pending.erase(std::remove_if(pending.begin(), pending.end(), [&](const auto &entry) {
            return entry.get() == &state;
        }), pending.end());
        const auto address = reinterpret_cast<size_t>(&state);
        const auto slot = ((address >> 4) ^ (address >> 12)) & 255;
        if(queue.recent[slot] == &state) queue.recent[slot] = nullptr;
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
        for(auto &state : pending)
        {
            // Bypass the write-depth override: publication itself must not enqueue
            // another write, and a failed snapshot allocation must still unlock.
            struct StateLock {
                StateData &state;
                explicit StateLock(StateData &value) : state(value) { state.StateData::lock(); }
                ~StateLock() { state.StateData::unlock(); }
            } lock{*state};
            // An enclosing explicit write/batch still owns the final boundary.
            // Its outer unlock will publish after the remaining edits finish.
            if(!state->m_writeDepth) state->publishRenderState();
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


    const MaterialRenderState &MaterialPassStateData::getSurfaceState() const
    {
        return m_pendingSurface ? *m_pendingSurface : *m_renderState;
    }

    MaterialRenderState &MaterialPassStateData::writableSurfaceState()
    {
        if( !m_pendingSurface )
            m_pendingSurface = std::make_shared<MaterialRenderState>( *m_renderState );
        return *m_pendingSurface;
    }

    bool MaterialPassStateData::setSurfaceState( const MaterialRenderState &value )
    {
        static_assert( MaterialRenderState::TextureCount <= 32, "binding change mask capacity" );
        if( sameSurface( getSurfaceState(), value ) ) return false;
        u32 changedBindings = 0;
        for( u32 i = 0; i < MaterialRenderState::TextureCount; ++i )
            if( getSurfaceState().textures[i] != value.textures[i] ) changedBindings |= u32( 1 ) << i;
        auto &next = writableSurfaceState();
        next = value;
        next.revision = m_renderState->revision;
        for( u32 i = 0; i < MaterialRenderState::TextureCount; ++i )
            if( changedBindings & ( u32( 1 ) << i ) ) notifyTextureBindingChanged( i );
        return true;
    }

    SmartPtr<ITexture> MaterialPassStateData::getTextureBinding( u32 slot ) const
    {
        return slot < MaterialRenderState::TextureCount ? getSurfaceState().textures[slot] : nullptr;
    }

    bool MaterialPassStateData::setTextureBinding( u32 slot, SmartPtr<ITexture> texture )
    {
        if( slot >= MaterialRenderState::TextureCount ||
            getSurfaceState().textures[slot] == texture ) return false;
        writableSurfaceState().textures[slot] = texture;
        notifyTextureBindingChanged( slot );
        return true;
    }

    void MaterialPassStateData::setTextureBindingObserver( render::IMaterialPass *owner,
                                                          void ( *observer )( void *, u32 ) )
    {
        m_textureBindingOwner = owner;
        m_textureBindingObserver = observer;
    }

    void MaterialPassStateData::notifyTextureBindingChanged( u32 slot )
    {
        if( m_textureBindingObserver )
            if( auto owner = m_textureBindingOwner.lock() ) m_textureBindingObserver( owner.get(), slot );
    }

    size_t MaterialPassStateData::getPendingSurfaceStorageBytes() const
    { return m_pendingSurface ? sizeof( MaterialRenderState ) : 0; }

    size_t MaterialPassStateData::getSurfaceAuthoringStorageBytes() const
    { return m_surfaceAuthoring ? sizeof( SurfaceAuthoringData ) : 0; }

    ColourF MaterialPassStateData::getTintColour() const
    { return m_surfaceAuthoring ? m_surfaceAuthoring->tintColour : ColourF::White; }

    void MaterialPassStateData::setTintColour( const ColourF &value )
    {
        if( sameMaterialValue( getTintColour(), value ) ) return;
        if( !m_surfaceAuthoring ) m_surfaceAuthoring = std::make_unique<SurfaceAuthoringData>();
        m_surfaceAuthoring->tintColour = value;
        if( sameMaterialValue( value, ColourF::White ) && !m_surfaceAuthoring->renderTechnique )
            m_surfaceAuthoring.reset();
    }

    hash_type MaterialPassStateData::getRenderTechnique() const
    { return m_surfaceAuthoring ? m_surfaceAuthoring->renderTechnique : 0; }

    void MaterialPassStateData::setRenderTechnique( hash_type value )
    {
        if( getRenderTechnique() == value ) return;
        if( !m_surfaceAuthoring ) m_surfaceAuthoring = std::make_unique<SurfaceAuthoringData>();
        m_surfaceAuthoring->renderTechnique = value;
        if( !value && sameMaterialValue( m_surfaceAuthoring->tintColour, ColourF::White ) )
            m_surfaceAuthoring.reset();
    }

    void MaterialPassStateData::publishRenderState()
    {
        if( !m_pendingSurface ) return;
        if( MaterialPublicationScope::defer( *this ) ) return;
        auto previous = std::atomic_load( &m_renderState );
        if( sameSurface( *m_pendingSurface, *previous ) )
        {
            m_pendingSurface.reset();
            return;
        }
        m_pendingSurface->revision = previous->revision + 1;
        MaterialRenderState::Ptr next = std::move( m_pendingSurface );
        std::atomic_store( &m_renderState, std::move( next ) );
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
        const auto &field = FloatFields[i];
        if( field.member ) setSurfaceValue( field.member, value );
        else
        {
            setNumericWord( field.coldIndex, floatWord( value ), floatWord( field.defaultValue ) );
        }
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getFloatSetting( MaterialFloatSetting id, f32 &value ) const
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( FloatFields ) / sizeof( FloatFields[0] ) ) return false;
        const auto &field = FloatFields[i];
        value = field.member ? getSurfaceState().*( field.member ) :
            wordFloat( getNumericWord( field.coldIndex, floatWord( field.defaultValue ) ) );
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
            setSurfaceValue( &MaterialRenderState::materialType, static_cast<MaterialType>( value ) );
            if( !m_writeDepth ) publishRenderState();
            return true;
        }
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( UIntFields ) / sizeof( UIntFields[0] ) ) return false;
        const auto &field = UIntFields[i];
        if( field.member ) setSurfaceValue( field.member, value );
        else
        {
            setNumericWord( ColdFloatCount + field.coldIndex, value, field.defaultValue );
        }
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getUIntSetting( MaterialUIntSetting id, u32 &value ) const
    {
        if( id == MaterialUIntSetting::materialType )
        { value = static_cast<u32>( getSurfaceState().materialType ); return true; }
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( UIntFields ) / sizeof( UIntFields[0] ) ) return false;
        const auto &field = UIntFields[i];
        value = field.member ? getSurfaceState().*( field.member ) :
            getNumericWord( ColdFloatCount + field.coldIndex, field.defaultValue );
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
        setAuthoringText( 3 + i, value, WP_MAX_PATH );
        if( !m_writeDepth ) publishRenderState();
        return true;
    }

    bool MaterialPassStateData::getStringSetting( MaterialStringSetting id, String &value ) const
    {
        const auto i = static_cast<u32>( id );
        if( i >= sizeof( StringFields ) / sizeof( StringFields[0] ) ) return false;
        value = String( getAuthoringText( 3 + i ) );
        return true;
    }

    bool MaterialPassStateData::setEditorString( const String &name, const String & value )
    { return setStringSetting( resolveStringSetting( name ), value ); }

    bool MaterialPassStateData::getEditorString( const String &name, String &value ) const
    { return getStringSetting( resolveStringSetting( name ), value ); }

    void MaterialPassStateData::setFlag( u32 flag, bool value )
    {
        setSurfaceValue( &MaterialRenderState::flags,
                         BitUtil::setFlagValue( getSurfaceState().flags, flag, value ) );
        if( !m_writeDepth ) publishRenderState();
    }

    bool MaterialPassStateData::getFlag( u32 flag ) const
    {
        return BitUtil::getFlagValue( getSurfaceState().flags, flag );
    }

    u32 MaterialPassStateData::getNumericWord( u32 index, u32 defaultValue ) const
    {
        const auto mask = numericMask( m_numericAuthoring.get() );
        const auto bit = u64( 1 ) << index;
        if( !( mask & bit ) ) return defaultValue;
        return m_numericAuthoring[2 + numericWordCount( mask & ( bit - 1 ) )];
    }

    void MaterialPassStateData::setNumericWord( u32 index, u32 value, u32 defaultValue )
    {
        const auto mask = numericMask( m_numericAuthoring.get() );
        const auto bit = u64( 1 ) << index;
        const bool present = ( mask & bit ) != 0;
        const bool remove = value == defaultValue;
        if( !present && remove ) return;
        const auto rank = numericWordCount( mask & ( bit - 1 ) );
        if( present && !remove )
        {
            m_numericAuthoring[2 + rank] = value;
            return;
        }
        const auto nextMask = remove ? mask & ~bit : mask | bit;
        if( !nextMask ) { m_numericAuthoring.reset(); return; }
        const auto count = numericWordCount( mask );
        const auto nextCount = remove ? count - 1 : count + 1;
        auto next = std::make_unique<u32[]>( 2 + nextCount );
        std::memcpy( next.get(), &nextMask, sizeof( nextMask ) );
        if( rank ) std::memcpy( next.get() + 2, m_numericAuthoring.get() + 2, rank * sizeof( u32 ) );
        if( remove )
        {
            const auto tail = count - rank - 1;
            if( tail ) std::memcpy( next.get() + 2 + rank, m_numericAuthoring.get() + 3 + rank,
                                   tail * sizeof( u32 ) );
        }
        else
        {
            next[2 + rank] = value;
            const auto tail = count - rank;
            if( tail ) std::memcpy( next.get() + 3 + rank, m_numericAuthoring.get() + 2 + rank,
                                   tail * sizeof( u32 ) );
        }
        // Publish the replacement only after allocation/copy succeeds.
        m_numericAuthoring = std::move( next );
    }

    size_t MaterialPassStateData::getNumericAuthoringStorageBytes() const
    {
        return m_numericAuthoring ? ( 2 + numericWordCount( numericMask( m_numericAuthoring.get() ) ) ) *
                                       sizeof( u32 ) : 0;
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
