#include <Workphone/WorkphonePCH.hpp>
#include <iostream>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/State/Messages/StateMessageVector4.hpp>
#include <Workphone/Graphics/GraphicsUtil.hpp>

namespace workphone::render
{
    namespace
    {
        const String EditorSettingsStr = String( "editorSettings" );
        const String EditorFloatsStr = String( "floats" );
        const String EditorUIntsStr = String( "uints" );
        const String EditorBoolsStr = String( "bools" );
        const String EditorStringsStr = String( "strings" );
        const String MaterialTypeDataStr = String( "materialType" );
        constexpr u32 LightingEnabledFlag = 1u << 19u;

        void restoreSpecularAmount( const Properties &properties, MaterialPass &pass )
        {
            // The editor slider updates both the scalar setting and the legacy
            // colour consumed by renderers. Restore that same colour after the
            // legacy fields, otherwise a saved slider value only updates the UI.
            if( auto editor = properties.getChild( EditorSettingsStr ) )
            {
                if( auto floats = editor->getChild( EditorFloatsStr ) )
                {
                    f32 amount;
                    if( floats->getPropertyValue( "specular", amount ) )
                    {
                        pass.setSpecular( ColourF( amount, amount, amount, 1.0f ) );
                    }
                }
            }
        }

        void writePassState( Properties &properties, const MaterialPassStateData &state )
        {
            properties.setProperty(
                MaterialTypeDataStr,
                static_cast<s32>( static_cast<MaterialType>( state.getSurfaceState().materialType ) ) );

            auto editorProperties = workphone::make_ptr<Properties>();
            editorProperties->setName( EditorSettingsStr );

            auto floats = workphone::make_ptr<Properties>();
            floats->setName( EditorFloatsStr );
            auto uints = workphone::make_ptr<Properties>();
            uints->setName( EditorUIntsStr );
            auto bools = workphone::make_ptr<Properties>();
            bools->setName( EditorBoolsStr );
            auto strings = workphone::make_ptr<Properties>();
            strings->setName( EditorStringsStr );

            state.writeEditorSettings( *floats, *uints, *bools, *strings );

            editorProperties->addChild( floats );
            editorProperties->addChild( uints );
            editorProperties->addChild( bools );
            editorProperties->addChild( strings );
            properties.addChild( editorProperties );
        }

        void readPassState( const Properties &properties, MaterialPassStateData &state )
        {
            auto materialType = static_cast<s32>( static_cast<MaterialType>( state.getSurfaceState().materialType ) );
            if( properties.getPropertyValue( MaterialTypeDataStr, materialType ) && materialType >= 0 &&
                materialType < static_cast<s32>( MaterialType::Count ) )
            {
                state.setSurfaceValue( &MaterialRenderState::materialType, static_cast<MaterialType>( materialType ) );
            }

            if( auto editorProperties = properties.getChild( EditorSettingsStr ) )
            {
                if( auto values = editorProperties->getChild( EditorFloatsStr ) )
                {
                    for( const auto &property : values->getPropertiesAsArray() )
                    {
                        state.setEditorFloat( property.getName(), property.getValueAsFloat() );
                    }
                }

                if( auto values = editorProperties->getChild( EditorUIntsStr ) )
                {
                    for( const auto &property : values->getPropertiesAsArray() )
                    {
                        state.setEditorUInt( property.getName(),
                                             static_cast<u32>( property.getValueAsInt() ) );
                    }
                }

                if( auto values = editorProperties->getChild( EditorBoolsStr ) )
                {
                    for( const auto &property : values->getPropertiesAsArray() )
                    {
                        state.setEditorBool( property.getName(), property.getValueAsBool() );
                    }
                }

                if( auto values = editorProperties->getChild( EditorStringsStr ) )
                {
                    for( const auto &property : values->getPropertiesAsArray() )
                    {
                        state.setEditorString( property.getName(), property.getValue() );
                    }
                }
            }
        }
    }  // namespace

    const String MaterialPass::nameStr = String( "name" );
    const String MaterialPass::materialNameStr = String( "Material Name" );
    const String MaterialPass::mainTextureStr = String( "Main Texture" );
    const String MaterialPass::vertexShaderStr = String( "Vertex Shader" );
    const String MaterialPass::fragmentShaderStr = String( "Fragment Shader" );
    const String MaterialPass::geometryShaderStr = String( "Geometry Shader" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialPass, MaterialNode<IMaterialPass> );

    MaterialPass::MaterialPass()
    {
        // Ensure state data is registered up-front so the property accessors
        // (isEmissionEnabled, isCutout, setRefractionEnabled, etc.) work for callers
        // that construct a stand-alone MaterialPass without explicit setup. Subclasses
        // (ClawMaterialPass, etc.) take over the load step.
        load( nullptr );
    }

    MaterialPass::~MaterialPass()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Unloaded )
        {
            unload( nullptr );
        }
    }

    void MaterialPass::seedShaderSurface( MaterialPassStateData &state ) const
    {
        auto material = dynamic_pointer_cast<Material>( getMaterial() );
        if( !material ) return;
        const bool pbr = material->m_shaderWorkflow == Material::ShaderWorkflow::Pbr;
        const bool colorOnly = material->m_shaderWorkflow == Material::ShaderWorkflow::ColorOnly;
        if( material->m_shaderWorkflow == Material::ShaderWorkflow::Legacy ) return;
        auto surface = state.getSurfaceState();
        const auto &parameters = material->getShaderParameters();
        const auto seed = [&]( MaterialBuiltinParameter semantic, f32 &value ) {
            const auto handle = parameters.GetBuiltin( semantic );
            if( parameters.IsHandleValid( handle, MaterialShaderParameterType::Scalar, sizeof( f32 ) ) )
                value = parameters.GetScalar( handle );
        };
        if( pbr )
        {
            seed( MaterialBuiltinParameter::Metalness, surface.metalness );
            seed( MaterialBuiltinParameter::Roughness, surface.roughness );
            seed( MaterialBuiltinParameter::AlphaClip, surface.alphaClip );
        }
        const auto seedColour = [&]( MaterialBuiltinParameter semantic, ColourF &value ) {
            const auto handle = parameters.GetBuiltin( semantic );
            if( !parameters.IsHandleValid( handle, MaterialShaderParameterType::Colour, sizeof( ColourF ) ) )
                return false;
            value = parameters.GetColour( handle );
            return true;
        };
        const auto hasAlbedo = pbr && seedColour( MaterialBuiltinParameter::Albedo, surface.diffuseColour );
        const auto hasColor = colorOnly && seedColour( MaterialBuiltinParameter::Color, surface.diffuseColour );
        if( hasColor ) surface.emissiveColour = surface.diffuseColour;
        else if( !colorOnly ) seedColour( MaterialBuiltinParameter::Emissive, surface.emissiveColour );
        const auto opacity = parameters.GetBuiltin( MaterialBuiltinParameter::Opacity );
        const auto hasOpacity = parameters.IsHandleValid( opacity, MaterialShaderParameterType::Scalar, sizeof( f32 ) );
        if( hasOpacity || hasAlbedo || hasColor )
        {
            surface.opacity = Math<f32>::clamp( hasOpacity ? parameters.GetScalar( opacity ) :
                                             ( colorOnly ? surface.opacity : surface.diffuseColour.a ),
                                             0.0f, 1.0f );
            surface.diffuseColour.a = surface.opacity;
            if( surface.opacity < 0.999f )
            {
                surface.flags = BitUtil::setFlagValue( surface.flags, static_cast<u32>( transparentFlag ), true );
                u32 renderMode = 0;
                state.getUIntSetting( MaterialUIntSetting::renderMode, renderMode );
                if( renderMode < 2u )
                {
                    state.setUIntSetting( MaterialUIntSetting::renderMode, 3u );
                    surface.blendMode = 1u;
                    surface.flags = BitUtil::setFlagValue( surface.flags, static_cast<u32>( cutoutFlag ), false );
                }
            }
        }
        state.setSurfaceState( surface );
    }

    void MaterialPass::setMaterial( SmartPtr<IMaterial> material )
    {
        const auto changed = getMaterial() != material;
        MaterialNode<IMaterialPass>::setMaterial( material );
        if( changed && material )
            if( auto context = getStateContextPtr() )
                if( auto state = context->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    if( auto owner = dynamic_pointer_cast<Material>( material ) ) owner->seedLegacySurface( *state );
                    seedShaderSurface( *state );
                }
    }

    void MaterialPass::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        if( auto material = getMaterial() ) material->setPass( nullptr );
        // The base MaterialNode<IMaterialPass>::setStateContext() is a no-op,
        // so persist the context in our own atomic storage. This keeps the
        // state-context get/set round-trip working for standalone MaterialPass
        // instances (used in unit tests and editor scenarios).
        if( m_localStateContext.load() != stateContext )
        {
            releaseLocalStateContext();
            m_localStateContext.store( stateContext );
        }
    }

    void MaterialPass::releaseLocalStateContext()
    {
        if( auto material = getMaterial() ) material->setPass( nullptr );
        // Retained facades must capture the live binding before the state retires.
        detachTextureUnits();
        if( auto context = getStateContextPtr() )
            if( auto data = context->getStateDataById<MaterialPassStateData>( getId() ) )
                data->setTextureBindingObserver( nullptr, nullptr );
        // SmartPtr's rvalue constructor retains its source; clear ownership
        // explicitly before cleanup or a later shared context could inherit it.
        auto context = m_createdStateContext;
        auto state = m_createdState;
        auto manager = m_createdContextManager;
        m_createdStateContext = nullptr;
        m_createdState = nullptr;
        m_createdContextManager = nullptr;
        m_localStateContext.store( nullptr );
        if( state )
        {
            if( auto data = dynamic_pointer_cast<MaterialPassStateData>( state->getData() ) )
                data->setTextureBindingObserver( nullptr, nullptr );
            if( context )
                context->removeState( state );
            state->setOwner( nullptr );
            state->setStateContext( nullptr );
            state->unload( nullptr );
        }
        // A supplied context can be shared by other passes. Only its creator
        // removes the context itself; this pass removes only its own record.
        if( manager && context )
            manager->removeStateContext( context );
    }

    SmartPtr<IStateContext> MaterialPass::getStateContext() const
    {
        // Prefer a directly-attached state context; fall back to the parent
        // material's context (if any) for compatibility with the existing
        // MaterialNode chain.
        auto local = m_localStateContext.load();
        if( local )
        {
            return local;
        }
        return MaterialNode<IMaterialPass>::getStateContext();
    }

    IStateContext *MaterialPass::getStateContextPtr() const
    {
        if( auto ctx = m_localStateContext.load() )
        {
            return ctx.get();
        }
        return MaterialNode<IMaterialPass>::getStateContextPtr();
    }
    void MaterialPass::load( SmartPtr<ISharedObject> data )
    {
        if( auto material = getMaterial() ) material->setPass( nullptr );
        try
        {
            setLoadingState( LoadingState::Loading );

            // Ensure a state context + MaterialPassStateData are registered so the
            // set*/get* property accessors below operate on a real record. The
            // GraphicsObject<T>::setupStateObject() helper is not available on this
            // inheritance chain, so re-implement the same logic inline.
            try
            {
                if( !getStateContextPtr() )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    if( applicationManager )
                    {
                        auto stateManager = applicationManager->getStateManagerPtr();
                        auto factoryManager = applicationManager->getFactoryManagerPtr();
                        if( stateManager && factoryManager )
                        {
                            auto stateContext = stateManager->addStateContext();
                            m_createdContextManager = applicationManager->getStateManager();
                            m_createdStateContext = stateContext;
                            m_localStateContext.store( stateContext );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            if( auto stateContext = getStateContextPtr() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                if( applicationManager )
                {
                    auto factoryManager = applicationManager->getFactoryManager();
                    if( factoryManager &&
                        !stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
                    {
                        auto state = factoryManager->make_ptr<State>();
                        if( state )
                        {
                            auto stateData = factoryManager->make_ptr<MaterialPassStateData>();
                            if( stateData )
                            {
                                ScopedLock<MaterialPassStateData> seedScope( stateData );
                                // State data owns the documented surface defaults.
                                if( auto material = dynamic_pointer_cast<Material>( getMaterial() ) )
                                    material->seedLegacySurface( *stateData );
                                seedShaderSurface( *stateData );
                                stateData->setFlag( depthWriteFlag, true );
                                for( u32 i = 0; i < m_textures.size() && i < MaterialRenderState::TextureCount; ++i )
                                    if( auto unit = m_textures[i] )
                                        if( !unit->getParent() || unit->getParent().get() == this )
                                            stateData->setTextureBinding( i, unit->getTexture() );
                            }
                            state->setId( getId() );
                            state->setOwner( this );
                            state->setData( stateData );
                            stateContext->addState( state );
                            m_createdStateContext = stateContext;
                            m_createdState = state;
                        }
                    }
                }
            }

            if( auto context = getStateContextPtr() )
                if( auto state = context->getStateDataById<MaterialPassStateData>( getId() ) )
                    state->setTextureBindingObserver( this, &MaterialPass::textureBindingChanged );

            createTextureSlots();

            setupMaterial();

            //WP_ASSERT( m_pass );

            for( auto &t : m_textures )
            {
                if( t && ( !t->getParent() || t->getParent().get() == this ) )
                {
                    t->setParent( this );
                    t->load( nullptr );
                }
            }

            // Authored texture units may precede the pass's state context.
            for(auto &unit : m_textures)
                if( unit && unit->getParent().get() == this )
                    updateRenderTexture( unit.get(), unit->getTexture() );

            setLoadingState( LoadingState::Loaded );
            if( auto material = getMaterial() ) material->setPass( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialPass::createTextureSlots()
    {
        if(!getParent() || !getMaterial()) return;
        auto type = MaterialType::Standard;
        if(auto context = getStateContextPtr())
            if(auto state = context->getStateDataById<MaterialPassStateData>(getId())) type = state->getSurfaceState().materialType;
        size_t slots = 0;
        String passName;
        switch(type)
        {
        case MaterialType::Standard:
        case MaterialType::StandardSpecular:
        case MaterialType::StandardTriPlanar:
            slots = static_cast<size_t>(PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES);
            passName = GraphicsUtil::getPbsTextureType(static_cast<PbsTextureTypes>(slots - 1)); break;
        case MaterialType::TerrainStandard:
        case MaterialType::TerrainSpecular:
        case MaterialType::TerrainDiffuse:
            slots = static_cast<size_t>(TerrainTextureTypes::Count);
            if(type == MaterialType::TerrainDiffuse)
                passName = GraphicsUtil::getTerrainTextureType(static_cast<TerrainTextureTypes>(slots - 1));
            break;
        case MaterialType::Skybox:
            slots = static_cast<size_t>(SkyboxTextureTypes::Count);
            passName = GraphicsUtil::getSkyboxTextureType(static_cast<SkyboxTextureTypes>(slots - 1)); break;
        case MaterialType::SkyboxCubemap:
            slots = static_cast<size_t>(SkyboxCubeTextureTypes::Count);
            passName = GraphicsUtil::getSkyboxCubeTextureType(static_cast<SkyboxCubeTextureTypes>(slots - 1)); break;
        case MaterialType::UI: slots = 1; break;
        default: return;
        }
        if(slots != getNumTexturesNodes())
        {
            // Logical empty slots need no authoring object. Keep authored units
            // when the schema changes size; texture bindings stay in state data.
            for( size_t i = slots; i < m_textures.size(); ++i )
                if( auto unit = m_textures[i] )
                    if( unit->getParent().get() == this ) unit->setParent( nullptr );
            m_textures.resize(slots);
            if(auto context = getStateContextPtr())
                if(auto state = context->invalidateStateDataById<MaterialPassStateData>(getId()))
                    for(u32 i = static_cast<u32>(slots); i < MaterialRenderState::TextureCount; ++i)
                    {
                        state->setTextureBinding( i, nullptr );
                    }
            if(!passName.empty()) setName(passName);
        }
    }

    void MaterialPass::reload( SmartPtr<ISharedObject> data )
    {
        auto parent = getParent();

        unload( data );

        setParent( parent );
        load( data );

        setupMaterial();
    }

    void MaterialPass::setupMaterial()
    {
        try
        {
            auto parent = getParent();
            auto technique = workphone::static_pointer_cast<MaterialTechnique>( parent );
            if( technique )
            {
                auto owner = technique->getMaterial();
                auto material = workphone::static_pointer_cast<Material>( owner );
            }

            auto d = getDiffuse();
            auto e = getEmissive();

            //if( auto pass = getPass() )
            //{
            //    auto diffuse = Ogre::ColourValue( d.r, d.g, d.b, d.a );
            //    pass->setDiffuse( diffuse );
            //}
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialPass::setTextureUnits( const Array<SmartPtr<IMaterialTexture>> &textures )
    {
        FixedArray<SmartPtr<ITexture>, MaterialRenderState::TextureCount> bindings{};
        for( u32 i = 0; i < textures.size() && i < bindings.size(); ++i )
            if( textures[i] ) bindings[i] = textures[i]->getTexture();
        for( const auto &unit : textures )
            if( unit )
                if( auto previous = dynamic_pointer_cast<MaterialPass>( unit->getParent() ) )
                    if( previous.get() != this ) previous->removeTextureUnit( unit );
        detachTextureUnits();
        m_textures = { textures.begin(), textures.end() };
        if( auto context = getStateContextPtr() )
            if( auto state = context->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                for( u32 i = 0; i < bindings.size(); ++i ) state->setTextureBinding( i, bindings[i] );
        for( auto unit : textures ) if( unit ) unit->setParent( this );
    }

    void MaterialPass::detachTextureUnits()
    {
        for( auto unit : m_textures )
            if( unit && unit->getParent().get() == this ) unit->setParent( nullptr );
    }

    void MaterialPass::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            Array<SmartPtr<IMaterialTexture>> ownedUnits;
            for( const auto &unit : m_textures )
                if( unit && ( !unit->getParent() || unit->getParent().get() == this ) )
                    ownedUnits.push_back( unit );
            releaseLocalStateContext();
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                for( auto &t : ownedUnits )
                {
                    if(t) t->unload( nullptr );
                }

                m_textures.clear();

                MaterialNode<IMaterialPass>::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialPass::setSceneBlending( u32 blendType )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::blendMode, blendType );
            }
        }
    }

    auto MaterialPass::isDepthCheckEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().depthTest != 0u;
            }
        }

        return true;
    }

    void MaterialPass::setDepthCheckEnabled( bool enabled )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::depthTest, enabled ? ( state->getSurfaceState().depthTest != 0u ? state->getSurfaceState().depthTest : 2u ) : 0u );
            }
        }
    }

    auto MaterialPass::isDepthWriteEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags, static_cast<u32>( render::depthWriteFlag ) );
            }
        }

        return false;
    }

    void MaterialPass::setDepthWriteEnabled( bool enabled )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue(
                    state->getSurfaceState().flags, static_cast<u32>( render::depthWriteFlag ), enabled ) );
            }
        }
    }

    auto MaterialPass::getCullingMode() const -> u32
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().cullMode;
            }
        }

        return 2;
    }

    void MaterialPass::setCullingMode( u32 mode )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::cullMode, mode );
            }
        }
    }

    void MaterialPass::setLightingEnabled( bool enabled )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue( state->getSurfaceState().flags, LightingEnabledFlag, enabled ) );
            }
        }
    }

    auto MaterialPass::getLightingEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags, LightingEnabledFlag );
            }
        }

        return false;
    }

    auto MaterialPass::createTextureUnit() -> SmartPtr<IMaterialTexture>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto materialTexture = factoryManager->make_ptr<MaterialTexture>();
        addTextureUnit( materialTexture );

        return materialTexture;
    }

    void MaterialPass::addTextureUnit( SmartPtr<IMaterialTexture> textureUnit )
    {
        const auto texture = textureUnit ? textureUnit->getTexture() : nullptr;
        if( textureUnit )
            if( auto previous = dynamic_pointer_cast<MaterialPass>( textureUnit->getParent() ) )
                if( previous.get() != this ) previous->removeTextureUnit( textureUnit );
        m_textures.push_back( textureUnit );
        if( textureUnit )
        {
            updateRenderTexture( textureUnit.get(), texture );
            textureUnit->setParent( this );
        }
    }

    void MaterialPass::removeTextureUnit( SmartPtr<IMaterialTexture> textureUnit )
    {
        // Preserve bindings while compacting the authoring slot sequence.
        auto units = getTextureUnits();
        detachTextureUnits();
        units.erase( std::remove( units.begin(), units.end(), textureUnit ), units.end() );
        setTextureUnits( units );
    }

    bool MaterialPass::getRenderTextureBinding( const IMaterialTexture *unit,
                                                SmartPtr<ITexture> &texture ) const
    {
        u32 slot = MaterialRenderState::TextureCount;
        {
            ScopedLock<decltype( m_textures )> lock( &m_textures );
            for( u32 i = 0; i < m_textures.size() && i < MaterialRenderState::TextureCount; ++i )
                if( m_textures[i].get() == unit ) { slot = i; break; }
        }
        if( slot == MaterialRenderState::TextureCount ) return false;
        if( auto context = getStateContextPtr() )
            if( auto state = context->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                texture = state->getTextureBinding( slot );
                return true;
            }
        return false;
    }

    void MaterialPass::textureBindingChanged( void *owner, u32 slot )
    {
        auto pass = dynamic_cast<MaterialPass *>( static_cast<IMaterialPass *>( owner ) );
        if( !pass ) return;
        SmartPtr<IMaterialTexture> unit;
        {
            ScopedLock<decltype( m_textures )> lock( &pass->m_textures );
            if( slot < pass->m_textures.size() ) unit = pass->m_textures[slot];
        }
        if( unit && unit->getParent().get() == pass )
            if( auto concrete = dynamic_pointer_cast<MaterialTexture>( unit ) )
                concrete->onCanonicalBindingChanged();
    }

    void MaterialPass::updateRenderTexture( IMaterialTexture *unit, SmartPtr<ITexture> texture )
    {
        const auto units = m_textures.snapshot();
        for( u32 i = 0; i < units.size() && i < MaterialRenderState::TextureCount; ++i )
            if( units[i].get() == unit )
            {
                if( auto context = getStateContextPtr() )
                    if( auto state = context->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                    {
                        state->setTextureBinding( i, texture );
                    }
                return;
            }
    }

    void MaterialPass::refreshRenderTextures()
    {
        const auto units = m_textures.snapshot();
        if( auto context = getStateContextPtr() )
            if( auto state = context->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                for( u32 i = 0; i < MaterialRenderState::TextureCount; ++i )
                {
                    auto texture = i < units.size() && units[i] ? units[i]->getTexture() : nullptr;
                    state->setTextureBinding( i, texture );
                }
    }

    auto MaterialPass::getTextureUnits() const -> Array<SmartPtr<IMaterialTexture>>
    {
        const auto size = getNumTexturesNodes();
        for(u32 i = 0; i < size; ++i) getTextureUnit(i);
        return m_textures.snapshot();
    }

    Array<SmartPtr<IMaterialTexture>> MaterialPass::getMaterializedTextureUnits() const
    {
        return m_textures.snapshot();
    }

    size_Num MaterialPass::getMaterializedTextureUnitCount() const
    {
        const auto units = m_textures.snapshot();
        return std::count_if(units.begin(), units.end(), [](const auto &unit) { return bool(unit); });
    }

    SmartPtr<IMaterialTexture> MaterialPass::getTextureUnit(u32 index) const
    {
        {
            ScopedLock<decltype(m_textures)> lock(&m_textures);
            if(index >= m_textures.size()) return nullptr;
            if( m_textures[index] )
            {
                if( m_textures[index]->getParent().get() == this ) return m_textures[index];
                // A direct node reparent may leave an old membership behind.
                // Replace that facade without changing its new parent's binding.
                m_textures[index] = nullptr;
            }
        }
        // Read state and run factory/resource callbacks outside the array lock.
        // State writers may already hold their write lock when accessing units.
        SmartPtr<ITexture> texture;
        auto type = MaterialType::Standard;
        if(auto context = getStateContextPtr())
            if(auto state = context->getStateDataById<MaterialPassStateData>(getId()))
            {
                type = state->getSurfaceState().materialType;
                texture = state->getTextureBinding( index );
            }
        auto app = core::IApplicationManager::instancePtr();
        auto factory = app ? app->getFactoryManagerPtr() : nullptr;
        auto unit = factory ? factory->make_ptr<MaterialTexture>() : nullptr;
        if(!unit) return nullptr;
        unit->setTextureType(index);
        unit->setMaterial(getMaterial());
        if(type == MaterialType::UI && index == 0) unit->setName("Base");
        else if((type == MaterialType::TerrainStandard || type == MaterialType::TerrainSpecular) &&
                index < static_cast<u32>(TerrainTextureTypes::Count))
            unit->setName(GraphicsUtil::getTerrainTextureType(static_cast<TerrainTextureTypes>(index)));
        unit->setTexture(texture);
        SmartPtr<IMaterialTexture> result;
        {
            ScopedLock<decltype(m_textures)> lock(&m_textures);
            if(index < m_textures.size())
            {
                if(!m_textures[index]) m_textures[index] = unit;
                result = m_textures[index];
            }
        }
        // Parent callbacks must run outside the array lock, after membership exists.
        if( result ) result->setParent( const_cast<MaterialPass *>( this ) );
        return result;
    }

    auto MaterialPass::getNumTexturesNodes() const -> size_Num
    {
        return m_textures.size();
    }

    void MaterialPass::setTexture( SmartPtr<ITexture> tex, u32 layerIdx /*= 0 */ )
    {
        try
        {
            if(auto context = getStateContextPtr())
                if(auto state = context->invalidateStateDataById<MaterialPassStateData>(getId()))
                {
                    state->setTextureBinding( layerIdx, tex );
                }
            if(layerIdx >= getNumTexturesNodes()) m_textures.resize(size_t(layerIdx) + 1);
            // Clearing an implicit slot needs no authoring object or upload job.
            if(!tex && !m_textures[layerIdx]) return;
            if(auto unit = getTextureUnit(layerIdx))
            {
                unit->setTexture(tex);
                auto app = core::IApplicationManager::instancePtr();
                auto graphics = app ? app->getGraphicsSystemPtr() : nullptr;
                if(graphics) graphics->loadObject(unit);
            }
        }
        catch(std::exception &e) { WP_LOG_EXCEPTION(e); }
    }

    void MaterialPass::setTexture( const String &fileName, u32 layerIdx /*= 0 */ )
    {
        try
        {
            // Extending a slot must preserve existing authored units/bindings.
            if(layerIdx >= getNumTexturesNodes()) m_textures.resize(size_t(layerIdx) + 1);
            if(fileName.empty() && !m_textures[layerIdx])
            { setTexture(SmartPtr<ITexture>(), layerIdx); return; }
            if(auto unit = getTextureUnit(layerIdx)) unit->setTextureName(fileName);
        }
        catch(std::exception &e) { WP_LOG_EXCEPTION(e); }
    }

    void MaterialPass::setCubicTexture( const String &fileName, bool uvw, u32 layerIdx /*= 0 */ )
    {
    }

    void MaterialPass::setFragmentParam( const String &name, f32 value )
    {
    }

    void MaterialPass::setFragmentParam( const String &name, const Vector2<real_Num> &value )
    {
    }

    void MaterialPass::setFragmentParam( const String &name, const Vector3<real_Num> &value )
    {
    }

    void MaterialPass::setFragmentParam( const String &name, const Vector4F &value )
    {
    }

    void MaterialPass::setFragmentParam( const String &name, const ColourF &value )
    {
    }

    auto MaterialPass::getVertexShaderName() const -> String
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getVertexShaderName();
            }
        }

        return StringUtil::EmptyString;
    }

    void MaterialPass::setVertexShaderName( const String &name )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setVertexShaderName( name );
            }
        }
    }

    auto MaterialPass::getFragmentShaderName() const -> String
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getFragmentShaderName();
            }
        }

        return StringUtil::EmptyString;
    }

    void MaterialPass::setFragmentShaderName( const String &name )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setFragmentShaderName( name );
            }
        }
    }

    auto MaterialPass::getGeometryShaderName() const -> String
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getGeometryShaderName();
            }
        }

        return StringUtil::EmptyString;
    }

    void MaterialPass::setGeometryShaderName( const String &name )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setGeometryShaderName( name );
            }
        }
    }

    auto MaterialPass::getRenderTechnique() const -> hash_type
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getRenderTechnique();
            }
        }

        return 0;
    }

    void MaterialPass::setRenderTechnique( hash_type renderTechnique )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setRenderTechnique( renderTechnique );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getAmbient() const -> ColourF
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().ambientColour;
            }
        }

        return ColourF::White;
    }

    void MaterialPass::setAmbient( const ColourF &ambient )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::ambientColour, ambient );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getDiffuse() const -> ColourF
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().diffuseColour;
            }
        }

        return ColourF::White;
    }

    void MaterialPass::setDiffuse( const ColourF &diffuse )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::diffuseColour, diffuse );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getSpecular() const -> ColourF
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().specularColour;
            }
        }

        return ColourF::White;
    }

    void MaterialPass::setSpecular( const ColourF &specular )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::specularColour, specular );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getEmissive() const -> ColourF
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().emissiveColour;
            }
        }

        return ColourF::White;
    }

    void MaterialPass::setEmissive( const ColourF &emissive )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::emissiveColour, emissive );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getTint() const -> ColourF
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getTintColour();
            }
        }

        return ColourF::White;
    }

    void MaterialPass::setTint( const ColourF &tint )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setTintColour( tint );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getMetalness() const -> f32
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().metalness;
            }
        }

        return 0.0f;
    }

    void MaterialPass::setMetalness( f32 metalness )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::metalness, metalness );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::getRoughness() const -> f32
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().roughness;
            }
        }

        return 0.5f;
    }

    void MaterialPass::setRoughness( f32 roughness )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    state->setSurfaceValue( &MaterialRenderState::roughness, roughness );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialPass::toData() const -> SmartPtr<ISharedObject>
    {
        auto properties = getProperties();
        properties->setName( "passes" );

        auto ambient = getAmbient();
        auto diffuse = getDiffuse();
        auto specular = getSpecular();
        auto emissive = getEmissive();
        auto tint = getTint();

        auto metalness = getMetalness();
        auto roughness = getRoughness();
        auto lightingEnabled = getLightingEnabled();
        auto vertexShaderName = getVertexShaderName();
        auto fragmentShaderName = getFragmentShaderName();
        auto geometryShaderName = getGeometryShaderName();

        auto transparent = isTransparent();
        auto cutout = isCutout();
        auto enableEmission = isEmissionEnabled();
        auto enableRefraction = isRefractionEnabled();

        properties->setProperty( ambientStr, ambient );
        properties->setProperty( diffuseStr, diffuse );
        properties->setProperty( specularStr, specular );
        properties->setProperty( emissiveStr, emissive );
        properties->setProperty( tintStr, tint );
        properties->setProperty( metalnessStr, metalness );
        properties->setProperty( roughnessStr, roughness );
        properties->setProperty( vertexShaderStr, vertexShaderName );
        properties->setProperty( fragmentShaderStr, fragmentShaderName );
        properties->setProperty( geometryShaderStr, geometryShaderName );

        properties->setProperty( lightingEnabledStr, lightingEnabled );
        properties->setProperty( transparentStr, transparent );
        properties->setProperty( cutoutStr, cutout );
        properties->setProperty( enableEmissionStr, enableEmission );
        properties->setProperty( enableRefractionStr, enableRefraction );

        const auto textureUnits = m_textures.snapshot();
        FixedArray<SmartPtr<ITexture>, MaterialRenderState::TextureCount> bindings{};
        if(auto context = getStateContextPtr())
            if(auto data = context->getStateDataById<MaterialPassStateData>(getId()))
                for(u32 i = 0; i < bindings.size(); ++i)
                    bindings[i] = data->getTextureBinding( i );
        for(u32 index = 0; index < textureUnits.size(); ++index)
        {
            const auto &unit = textureUnits[index];
            SmartPtr<ITexture> texture;
            if(!unit && index < bindings.size()) texture = bindings[index];
            auto textureData = unit ? workphone::static_pointer_cast<Properties>(unit->toData()) :
                                     MaterialTexture::textureToData(texture);
            textureData->setName( "textures" );
            properties->addChild( textureData );
        }

        return properties;
    }

    void MaterialPass::fromData( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::static_pointer_cast<Properties>( data );
        if( !properties )
        {
            return;
        }

        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                readPassState( *properties, *state );
            }
        }

        auto ambient = getAmbient();
        properties->getPropertyValue( ambientStr, ambient );
        setAmbient( ambient );

        auto diffuse = getDiffuse();
        properties->getPropertyValue( diffuseStr, diffuse );

        if( getDiffuse() != diffuse )
        {
            setDiffuse( diffuse );
        }

        auto specular = getSpecular();
        properties->getPropertyValue( specularStr, specular );
        setSpecular( specular );
        restoreSpecularAmount( *properties, *this );

        auto tint = getTint();
        properties->getPropertyValue( tintStr, tint );
        setTint( tint );

        auto metalness = getMetalness();
        auto roughness = getRoughness();

        auto lightingEnabled = getLightingEnabled();

        auto transparent = isTransparent();
        auto cutout = isCutout();
        auto enableEmission = isEmissionEnabled();
        auto enableRefraction = isRefractionEnabled();
        auto vertexShaderName = getVertexShaderName();
        auto fragmentShaderName = getFragmentShaderName();
        auto geometryShaderName = getGeometryShaderName();

        auto emissive = getEmissive();
        properties->getPropertyValue( emissiveStr, emissive );
        setEmissive( emissive );

        properties->getPropertyValue( metalnessStr, metalness );
        properties->getPropertyValue( roughnessStr, roughness );

        properties->getPropertyValue( lightingEnabledStr, lightingEnabled );
        properties->getPropertyValue( transparentStr, transparent );
        properties->getPropertyValue( cutoutStr, cutout );
        properties->getPropertyValue( enableEmissionStr, enableEmission );
        properties->getPropertyValue( enableRefractionStr, enableRefraction );
        properties->getPropertyValue( vertexShaderStr, vertexShaderName );
        properties->getPropertyValue( fragmentShaderStr, fragmentShaderName );
        properties->getPropertyValue( geometryShaderStr, geometryShaderName );

        setMetalness( metalness );
        setRoughness( roughness );
        setLightingEnabled( lightingEnabled );
        setTransparent( transparent );
        setCutout( cutout );
        setEmissionEnabled( enableEmission );
        setRefractionEnabled( enableRefraction );
        setVertexShaderName( vertexShaderName );
        setFragmentShaderName( fragmentShaderName );
        setGeometryShaderName( geometryShaderName );

        createTextureSlots();

        auto textureIndex = 0;
        auto textures = properties->getChildrenByName( "textures" );
        for( auto &texture : textures )
        {
            // Serialized editor slots can extend beyond the fixed PBS slot list.
            // Create the unit before restoring it, just as setTexture does.
            if( static_cast<u32>( textureIndex ) >= getNumTexturesNodes() )
                setTexture( SmartPtr<ITexture>(), static_cast<u32>( textureIndex ) );
            // Empty legacy descriptors preserve ordinal slots without allocating
            // facades. Nonempty/custom data still reaches the normal adapter.
            if(texture->getPropertiesAsArray().empty() && texture->getChildren().empty())
            { ++textureIndex; continue; }
            if( auto pTexture = getTextureUnit(static_cast<u32>(textureIndex)) )
            {
                pTexture->fromData( texture );

                if( auto stateContext = getStateContextPtr() )
                {
                    if( auto state =
                            stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                    {
                        state->setTextureBinding( static_cast<u32>( textureIndex ), pTexture->getTexture() );
                    }
                }
            }

            textureIndex++;
        }
    }

    auto MaterialPass::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = MaterialNode<IMaterialPass>::getProperties();

        // auto handle = getHandle();
        // properties->setProperty(MaterialPass::nameStr, handle->getName());
        // properties->setProperty(MaterialPass::materialNameStr, m_materialName);
        // properties->setProperty(mainTextureStr, m_mainTexturePath);

        auto ambient = getAmbient();
        auto diffuse = getDiffuse();
        auto specular = getSpecular();
        auto emissive = getEmissive();
        auto tint = getTint();

        auto metalness = getMetalness();
        auto roughness = getRoughness();
        auto lightingEnabled = getLightingEnabled();
        auto vertexShaderName = getVertexShaderName();
        auto fragmentShaderName = getFragmentShaderName();
        auto geometryShaderName = getGeometryShaderName();

        auto transparent = isTransparent();
        auto cutout = isCutout();
        auto enableEmission = isEmissionEnabled();
        auto enableRefraction = isRefractionEnabled();

        properties->setProperty( ambientStr, ambient );
        properties->setProperty( diffuseStr, diffuse );
        properties->setProperty( specularStr, specular );
        properties->setProperty( emissiveStr, emissive );
        properties->setProperty( tintStr, tint );
        properties->setProperty( metalnessStr, metalness );
        properties->setProperty( roughnessStr, roughness );
        properties->setProperty( vertexShaderStr, vertexShaderName );
        properties->setProperty( fragmentShaderStr, fragmentShaderName );
        properties->setProperty( geometryShaderStr, geometryShaderName );
        properties->setProperty( lightingEnabledStr, lightingEnabled );
        properties->setProperty( transparentStr, transparent );
        properties->setProperty( cutoutStr, cutout );
        properties->setProperty( enableEmissionStr, enableEmission );
        properties->setProperty( enableRefractionStr, enableRefraction );

        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                writePassState( *properties, *state );
            }
        }

        return properties;
    }

    void MaterialPass::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto context = getStateContextPtr();
        if( !context || !context->getStateDataById<MaterialPassStateData>( getId() ) ) load( nullptr );

        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                readPassState( *properties, *state );
            }
        }

        auto ambient = getAmbient();
        auto diffuse = getDiffuse();
        auto specular = getSpecular();
        auto emissive = getEmissive();
        auto tint = getTint();

        auto metalness = getMetalness();
        auto roughness = getRoughness();
        auto vertexShaderName = getVertexShaderName();
        auto fragmentShaderName = getFragmentShaderName();
        auto geometryShaderName = getGeometryShaderName();

        auto transparent = isTransparent();
        auto cutout = isCutout();
        auto enableEmission = isEmissionEnabled();
        auto enableRefraction = isRefractionEnabled();

        auto lightingEnabled = getLightingEnabled();

        // properties->getPropertyValue(MaterialPass::materialNameStr, m_materialName);
        // properties->getPropertyValue(MaterialPass::mainTextureStr, m_mainTexturePath);
        properties->getPropertyValue( ambientStr, ambient );
        properties->getPropertyValue( diffuseStr, diffuse );
        properties->getPropertyValue( specularStr, specular );
        properties->getPropertyValue( emissiveStr, emissive );
        properties->getPropertyValue( tintStr, tint );

        properties->getPropertyValue( metalnessStr, metalness );
        properties->getPropertyValue( roughnessStr, roughness );
        properties->getPropertyValue( vertexShaderStr, vertexShaderName );
        properties->getPropertyValue( fragmentShaderStr, fragmentShaderName );
        properties->getPropertyValue( geometryShaderStr, geometryShaderName );
        properties->getPropertyValue( lightingEnabledStr, lightingEnabled );
        properties->getPropertyValue( transparentStr, transparent );
        properties->getPropertyValue( cutoutStr, cutout );
        properties->getPropertyValue( enableEmissionStr, enableEmission );
        properties->getPropertyValue( enableRefractionStr, enableRefraction );

        setAmbient( ambient );
        setDiffuse( diffuse );
        setSpecular( specular );
        setEmissive( emissive );
        setTint( tint );

        setMetalness( metalness );
        setRoughness( roughness );
        setVertexShaderName( vertexShaderName );
        setFragmentShaderName( fragmentShaderName );
        setGeometryShaderName( geometryShaderName );

        setLightingEnabled( lightingEnabled );

        setTransparent( transparent );
        setCutout( cutout );
        setEmissionEnabled( enableEmission );
        setRefractionEnabled( enableRefraction );

        // Reconfigure slots/backend against the authored record. A full reload
        // retires this context and would discard the properties just imported.
        load( nullptr );
    }

    auto MaterialPass::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto textures = getTextureUnits();

        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( textures.size() );

        for( auto texture : textures )
        {
            objects.emplace_back( texture );
        }

        return objects;
    }

    void MaterialPass::setCutout( bool cutout )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue( state->getSurfaceState().flags,
                                                      static_cast<u32>( render::cutoutFlag ), cutout ) );
            }
        }
    }

    bool MaterialPass::isCutout() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags, static_cast<u32>( render::cutoutFlag ) );
            }
        }

        return false;
    }

    void MaterialPass::setTransparent( bool transparent )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue(
                    state->getSurfaceState().flags, static_cast<u32>( render::transparentFlag ), transparent ) );
            }
        }
    }

    bool MaterialPass::isTransparent() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags,
                                              static_cast<u32>( render::transparentFlag ) );
            }
        }

        return false;
    }

    void MaterialPass::setEmissionEnabled( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue(
                    state->getSurfaceState().flags, static_cast<u32>( render::emissionEnabledFlag ), enabled ) );
            }
        }
    }

    bool MaterialPass::isEmissionEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags,
                                              static_cast<u32>( render::emissionEnabledFlag ) );
            }
        }

        return false;
    }

    bool MaterialPass::isRefractionEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->getSurfaceState().flags,
                                              static_cast<u32>( render::refractionEnabledFlag ) );
            }
        }

        return false;
    }

    void MaterialPass::setRefractionEnabled( bool enabled )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, BitUtil::setFlagValue(
                    state->getSurfaceState().flags, static_cast<u32>( render::refractionEnabledFlag ), enabled ) );
            }
        }
    }

    u32 MaterialPass::getFlags() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->getSurfaceState().flags;
            }
        }

        return render::receiveShadowsFlag | render::castShadowsFlag | render::generateMipmapsFlag |
               render::srgbFlag | render::textureStreamingFlag | render::depthWriteFlag |
               render::gpuInstancingFlag | render::srpBatcherFlag | render::receiveDecalsFlag;
    }

    void MaterialPass::setFlags( u32 flags )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->setSurfaceValue( &MaterialRenderState::flags, flags );
            }
        }
    }

    bool MaterialPass::MaterialPassStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = workphone::static_pointer_cast<MaterialPass>( getOwner() ) )
        {
            const auto &loadingState = owner->getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                state->setDirty( false );
            }
        }

        return false;
    }

    bool MaterialPass::MaterialPassStateListener::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = workphone::static_pointer_cast<MaterialPass>( getOwner() ) )
        {
            if( message->isExactly<StateMessageVector4>() )
            {
                auto vectorMessage = workphone::static_pointer_cast<StateMessageVector4>( message );
                auto value = vectorMessage->getValue();
                auto type = vectorMessage->getType();

                if( type == DIFFUSE_HASH )
                {
                    auto d = ColourF( value.X(), value.Y(), value.Z(), value.W() );
                    owner->setDiffuse( d );
                }
                else if( type == EMISSION_HASH )
                {
                    auto e = ColourF( value.X(), value.Y(), value.Z(), value.W() );
                    owner->setEmissive( e );
                }
            }
        }

        return false;
    }

    MaterialPass::MaterialPassStateListener::MaterialPassStateListener() = default;

    MaterialPass::MaterialPassStateListener::~MaterialPassStateListener() = default;

}  // namespace workphone::render
