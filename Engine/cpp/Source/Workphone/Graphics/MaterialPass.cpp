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

        void writePassState( Properties &properties, const MaterialPassStateData &state )
        {
            properties.setProperty(
                MaterialTypeDataStr,
                static_cast<s32>( static_cast<MaterialType>( state.materialType ) ) );

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
            auto materialType = static_cast<s32>( static_cast<MaterialType>( state.materialType ) );
            if( properties.getPropertyValue( MaterialTypeDataStr, materialType ) && materialType >= 0 &&
                materialType < static_cast<s32>( MaterialType::Count ) )
            {
                state.materialType = static_cast<MaterialType>( materialType );
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

    void MaterialPass::setStateContext( SmartPtr<IStateContext> stateContext )
    {
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
                                // Seed the freshly-created state record with the
                                // MaterialPass member defaults so existing callers see the
                                // documented default values (metalness 0.5, roughness 0.01, etc.)
                                // rather than MaterialPassStateData's raw defaults.
                                stateData->metalness = m_metalness;
                                stateData->roughness = m_roughness;
                                stateData->setFlag( depthWriteFlag, true );
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

            createTextureSlots();

            setupMaterial();

            //WP_ASSERT( m_pass );

            for( auto &t : m_textures )
            {
                t->load( nullptr );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialPass::createTextureSlots()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto parent = getParent();
        if( !parent )
        {
            return;
        }

        auto technique = workphone::static_pointer_cast<MaterialTechnique>( parent );
        if( technique )
        {
            auto owner = technique->getMaterial();
            auto material = workphone::static_pointer_cast<Material>( owner );
            if( material )
            {
                auto materialType = MaterialType::Standard;
                if( auto stateContext = getStateContextPtr() )
                {
                    if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
                    {
                        materialType = state->materialType;
                    }
                }

                switch( materialType )
                {
                case MaterialType::Standard:
                {
                    auto numSlots = static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::StandardSpecular:
                {
                    auto numSlots = static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::StandardTriPlanar:
                {
                    auto numSlots = static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::TerrainStandard:
                {
                    auto numSlots = static_cast<size_t>( TerrainTextureTypes::Count );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<TerrainTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getTerrainTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                texture->setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::TerrainSpecular:
                {
                    auto numSlots = static_cast<size_t>( TerrainTextureTypes::Count );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<TerrainTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getTerrainTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                texture->setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::TerrainDiffuse:
                {
                    auto numSlots = static_cast<size_t>( TerrainTextureTypes::Count );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<TerrainTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getTerrainTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::Skybox:
                {
                    auto numSlots = static_cast<size_t>( SkyboxTextureTypes::Count );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<SkyboxTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getSkyboxTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::SkyboxCubemap:
                {
                    auto numSlots = static_cast<size_t>( SkyboxCubeTextureTypes::Count );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                auto textureType = static_cast<SkyboxCubeTextureTypes>( i );
                                auto textureTypeStr =
                                    GraphicsUtil::getSkyboxCubeTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );
                                texture->setMaterial( material );

                                setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::UI:
                {
                    auto numSlots = static_cast<size_t>( 1 );
                    if( numSlots != getNumTexturesNodes() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<MaterialTexture>();
                                texture->setParent( this );

                                // auto textureType = (PbsTextureTypes)i;
                                auto textureTypeStr =
                                    String( "Base" );  // GraphicsUtil::getPbsTextureType(textureType);

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );

                                texture->setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::Custom:
                {
                }
                break;
                default:
                {
                }
                break;
                }
            }
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
        m_textures = { textures.begin(), textures.end() };
    }

    void MaterialPass::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            releaseLocalStateContext();
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto textures = m_textures.snapshot();
                for( auto &t : textures )
                {
                    t->unload( nullptr );
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
                state->blendMode = blendType;
            }
        }
    }

    auto MaterialPass::isDepthCheckEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->depthTest != 0u;
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
                state->depthTest = enabled ? ( state->depthTest != 0u ? state->depthTest : 2u ) : 0u;
            }
        }
    }

    auto MaterialPass::isDepthWriteEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, static_cast<u32>( render::depthWriteFlag ) );
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
                state->flags = BitUtil::setFlagValue(
                    state->flags, static_cast<u32>( render::depthWriteFlag ), enabled );
            }
        }
    }

    auto MaterialPass::getCullingMode() const -> u32
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->cullMode;
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
                state->cullMode = mode;
            }
        }
    }

    void MaterialPass::setLightingEnabled( bool enabled )
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
            {
                state->flags = BitUtil::setFlagValue( state->flags, LightingEnabledFlag, enabled );
            }
        }
    }

    auto MaterialPass::getLightingEnabled() const -> bool
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, LightingEnabledFlag );
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
        m_textures.push_back( textureUnit );
    }

    void MaterialPass::removeTextureUnit( SmartPtr<IMaterialTexture> textureUnit )
    {
        m_textures.erase( std::remove( m_textures.begin(), m_textures.end(), textureUnit ),
                          m_textures.end() );
    }

    auto MaterialPass::getTextureUnits() const -> Array<SmartPtr<IMaterialTexture>>
    {
        return m_textures.snapshot();
    }

    auto MaterialPass::getNumTexturesNodes() const -> size_Num
    {
        return m_textures.size();
    }

    void MaterialPass::setTexture( SmartPtr<ITexture> tex, u32 layerIdx /*= 0 */ )
    {
        try
        {
            if( auto stateContext = getStateContextPtr() )
            {
                if( auto state =
                        stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                {
                    if( layerIdx < state->textures.size() )
                    {
                        state->textures[layerIdx] = tex;
                        state->textureDirty[layerIdx] = true;
                    }
                }
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            auto factoryManager = applicationManager->getFactoryManagerPtr();

            if( layerIdx >= getNumTexturesNodes() )
            {
                m_textures.resize( layerIdx + 1 );

                for( auto &texture : m_textures )
                {
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<MaterialTexture>();
                    }
                }

                auto &texture = m_textures[layerIdx];
                if( !texture )
                {
                    texture = factoryManager->make_ptr<MaterialTexture>();
                }

                if( texture )
                {
                    texture->setParent( this );
                    texture->setTexture( tex );

                    graphicsSystem->loadObject( texture );
                }
            }
            else
            {
                WP_ASSERT( layerIdx < getNumTexturesNodes() );

                if( layerIdx < getNumTexturesNodes() )
                {
                    auto &texture = m_textures[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<MaterialTexture>();
                    }

                    if( texture )
                    {
                        texture->setParent( this );
                        texture->setTexture( tex );

                        graphicsSystem->loadObject( texture );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialPass::setTexture( const String &fileName, u32 layerIdx /*= 0 */ )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();

            if( layerIdx >= getNumTexturesNodes() )
            {
                m_textures.clear();
                m_textures.resize( layerIdx + 1 );

                for( size_t i = 0; i < m_textures.size(); ++i )
                {
                    auto &texture = m_textures[i];
                    if( texture )
                    {
                        m_textures[i] = texture;
                    }
                }

                for( auto &texture : m_textures )
                {
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<MaterialTexture>();
                    }
                }

                auto &texture = m_textures[layerIdx];
                if( !texture )
                {
                    texture = factoryManager->make_ptr<MaterialTexture>();
                }

                if( texture )
                {
                    texture->setParent( this );
                    texture->setTextureName( fileName );
                }
            }
            else
            {
                WP_ASSERT( layerIdx < getNumTexturesNodes() );

                if( layerIdx < getNumTexturesNodes() )
                {
                    auto &texture = m_textures[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<MaterialTexture>();
                    }

                    if( texture )
                    {
                        texture->setParent( this );
                        texture->setTextureName( fileName );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
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
                return state->vertexShaderName;
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
                state->vertexShaderName = name;
            }
        }
    }

    auto MaterialPass::getFragmentShaderName() const -> String
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->fragmentShaderName;
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
                state->fragmentShaderName = name;
            }
        }
    }

    auto MaterialPass::getGeometryShaderName() const -> String
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->geometryShaderName;
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
                state->geometryShaderName = name;
            }
        }
    }

    auto MaterialPass::getRenderTechnique() const -> hash_type
    {
        if( auto stateContext = getStateContextPtr() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->renderTechnique;
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
                    state->renderTechnique = renderTechnique;
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
                return state->ambientColour;
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
                    state->ambientColour = ambient;
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
                return state->diffuseColour;
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
                    state->diffuseColour = diffuse;
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
                return state->specularColour;
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
                    state->specularColour = specular;
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
                return state->emissiveColour;
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
                    state->emissiveColour = emissive;
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
                return state->tintColour;
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
                    state->tintColour = tint;
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
                return state->metalness;
            }
        }

        return m_metalness;
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
                    state->metalness = metalness;
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
                return state->roughness;
            }
        }

        return m_roughness;
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
                    state->roughness = roughness;
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

        auto textureUnits = getTextureUnits();
        for( auto textureUnit : textureUnits )
        {
            auto textureData = workphone::static_pointer_cast<Properties>( textureUnit->toData() );
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
            auto textureUnits = getTextureUnits();
            if( textureIndex < textureUnits.size() )
            {
                auto pTexture = textureUnits[textureIndex];

                pTexture->fromData( texture );

                if( auto stateContext = getStateContextPtr() )
                {
                    if( auto state =
                            stateContext->invalidateStateDataById<MaterialPassStateData>( getId() ) )
                    {
                        if( textureIndex < state->textures.size() )
                        {
                            state->textures[textureIndex] = pTexture->getTexture();
                            state->textureDirty[textureIndex] = true;
                        }
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

        reload( nullptr );
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
                state->flags = BitUtil::setFlagValue( state->flags,
                                                      static_cast<u32>( render::cutoutFlag ), cutout );
            }
        }
    }

    bool MaterialPass::isCutout() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags, static_cast<u32>( render::cutoutFlag ) );
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
                state->flags = BitUtil::setFlagValue(
                    state->flags, static_cast<u32>( render::transparentFlag ), transparent );
            }
        }
    }

    bool MaterialPass::isTransparent() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags,
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
                state->flags = BitUtil::setFlagValue(
                    state->flags, static_cast<u32>( render::emissionEnabledFlag ), enabled );
            }
        }
    }

    bool MaterialPass::isEmissionEnabled() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return BitUtil::getFlagValue( state->flags,
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
                return BitUtil::getFlagValue( state->flags,
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
                state->flags = BitUtil::setFlagValue(
                    state->flags, static_cast<u32>( render::refractionEnabledFlag ), enabled );
            }
        }
    }

    u32 MaterialPass::getFlags() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<MaterialPassStateData>( getId() ) )
            {
                return state->flags;
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
                state->flags = flags;
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
