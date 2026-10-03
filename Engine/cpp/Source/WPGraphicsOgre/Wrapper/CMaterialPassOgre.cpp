#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CMaterialPassOgre, MaterialPass );

        const hash_type CMaterialPassOgre::DIFFUSE_HASH = StringUtil::getHash( "diffuse" );
        const hash_type CMaterialPassOgre::EMISSION_HASH = StringUtil::getHash( "emmision" );

        CMaterialPassOgre::CMaterialPassOgre()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto stateContext = stateManager->addStateContext();
            setStateContext( stateContext );

            auto stateListener = factoryManager->make_ptr<MaterialPassOgreStateListener>();
            stateListener->setOwner( this );
            setStateListener( stateListener );
            stateContext->addStateListener( stateListener );

            auto state = factoryManager->make_ptr<State>();
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<MaterialPassStateData>();
            state->setData( stateData );
        }

        CMaterialPassOgre::~CMaterialPassOgre()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            if( auto stateContext = getStateContext() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                stateManager->removeStateContext( stateContext );

                stateContext->unload( nullptr );
                setStateContext( nullptr );
            }

            if( auto stateListener = getStateListener() )
            {
                stateListener->unload( nullptr );
                setStateListener( nullptr );
            }
        }

        void CMaterialPassOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto pMaterial = getMaterial();
                WP_ASSERT( pMaterial );

                auto material = workphone::static_pointer_cast<CMaterialOgre>( pMaterial );

                auto pParent = getParent();
                WP_ASSERT( pParent );

                auto parent = workphone::static_pointer_cast<CMaterialTechniqueOgre>( pParent );

                auto ogreMaterial = material->getMaterial();
                auto technique = parent->getTechnique();

                if( !m_pass )
                {
                    auto pass = technique->createPass();
                    setPass( pass );
                }

                createTextureSlots();

                //WP_ASSERT( getTexturesPtr() && !getTexturesPtr()->empty() );

                setupMaterial();

                WP_ASSERT( m_pass );

                auto ogreTextureUnits = m_pass->getTextureUnitStates();

                auto count = 0;
                for( auto &texture : m_textures )
                {
                    auto pTexture = workphone::static_pointer_cast<CMaterialTextureOgre>( texture );

                    if( count < ogreTextureUnits.size() )
                    {
                        pTexture->setTextureUnitState( ogreTextureUnits[count] );
                    }

                    count++;
                }

                for( auto t : m_textures )
                {
                    t->load( nullptr );
                }

                for( auto t : m_textures )
                {
                    if( auto stateContext = t->getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialPassOgre::createTextureSlots()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto resourceDatabase = applicationManager->getResourceDatabase();

            if( auto parent = getParent() )
            {
                auto technique = workphone::static_pointer_cast<CMaterialTechniqueOgre>( parent );
                if( technique )
                {
                    auto owner = technique->getMaterial();
                    auto material = workphone::static_pointer_cast<CMaterialOgre>( owner );
                    if( material )
                    {
                        switch( auto materialType = material->getMaterialType() )
                        {
                        case MaterialType::Standard:
                        {
                            if( m_pass )
                            {
                                auto ogreTextureUnitStates = m_pass->getTextureUnitStates();
                                if( ogreTextureUnitStates.empty() )
                                {
                                    auto numSlots =
                                        static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                                    if( numSlots != getNumTexturesNodes() )
                                    {
                                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                                        textures.resize( numSlots );

                                        for( size_t i = 0; i < textures.size(); ++i )
                                        {
                                            auto &texture = textures[i];
                                            if( !texture )
                                            {
                                                auto pTexture =
                                                    factoryManager->make_ptr<CMaterialTextureOgre>();
                                                pTexture->setParent( this );
                                                pTexture->setEnabled( i ==
                                                                      0 );  // todo temp while no PBR
                                                texture = pTexture;

                                                auto textureType = static_cast<PbsTextureTypes>( i );
                                                auto textureTypeStr =
                                                    GraphicsUtil::getPbsTextureType( textureType );

                                                texture->setTextureType( (u32)i );

                                                texture->setMaterial( material );

                                                texture->setName( textureTypeStr );
                                            }
                                        }

                                        setTextureUnits( textures );
                                    }
                                }
                                else
                                {
                                    auto numSlots =
                                        static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );

                                    auto textures = Array<SmartPtr<IMaterialTexture>>();
                                    textures.reserve( numSlots );

                                    for( auto textureUnitState : ogreTextureUnitStates )
                                    {
                                        auto pTexture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        pTexture->setTextureUnitState( textureUnitState );
                                        pTexture->setMaterial( material );

                                        pTexture->setName( textureUnitState->getName().c_str() );

                                        auto textureName = textureUnitState->getTextureName();
                                        if( auto tex = resourceDatabase->loadResourceByType<ITexture>(
                                                textureName.c_str() ) )
                                        {
                                            pTexture->setTexture( tex );
                                        }

                                        textures.push_back( pTexture );
                                    }

                                    setTextureUnits( textures );
                                }
                            }
                        }
                        break;
                        case MaterialType::StandardSpecular:
                        {
                            auto numSlots =
                                static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                            if( numSlots != getNumTexturesNodes() )
                            {
                                auto textures = Array<SmartPtr<IMaterialTexture>>();
                                textures.resize( numSlots );

                                for( size_t i = 0; i < textures.size(); ++i )
                                {
                                    auto &texture = textures[i];
                                    if( !texture )
                                    {
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );
                                        texture->setEnabled( i == 0 );  // todo temp while no PBR

                                        auto textureType = static_cast<PbsTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getPbsTextureType( textureType );

                                        texture->setTextureType( (u32)i );

                                        texture->setMaterial( material );
                                        texture->setName( textureTypeStr );
                                    }
                                }

                                setTextureUnits( textures );
                            }
                        }
                        break;
                        case MaterialType::StandardTriPlanar:
                        {
                            auto numSlots =
                                static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                            if( numSlots != getNumTexturesNodes() )
                            {
                                auto textures = Array<SmartPtr<IMaterialTexture>>();
                                textures.resize( numSlots );

                                for( size_t i = 0; i < textures.size(); ++i )
                                {
                                    auto &texture = textures[i];
                                    if( !texture )
                                    {
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        auto textureType = static_cast<PbsTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getPbsTextureType( textureType );

                                        texture->setTextureType( (u32)i );

                                        texture->setMaterial( material );
                                        texture->setName( textureTypeStr );
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
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        auto textureType = static_cast<TerrainTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getTerrainTextureType( textureType );

                                        texture->setTextureType( (u32)i );

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
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        auto textureType = static_cast<TerrainTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getTerrainTextureType( textureType );

                                        texture->setTextureType( (u32)i );

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
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        auto textureType = static_cast<TerrainTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getTerrainTextureType( textureType );

                                        texture->setTextureType( (u32)i );

                                        texture->setMaterial( material );
                                        texture->setName( textureTypeStr );
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
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        auto textureType = static_cast<SkyboxTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getSkyboxTextureType( textureType );

                                        texture->setTextureType( (u32)i );

                                        texture->setMaterial( material );
                                        texture->setName( textureTypeStr );
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
                                        auto pTexture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        pTexture->setParent( this );

                                        texture = pTexture;

                                        auto textureType = static_cast<SkyboxCubeTextureTypes>( i );
                                        auto textureTypeStr =
                                            GraphicsUtil::getSkyboxCubeTextureType( textureType );

                                        texture->setTextureType( (u32)i );
                                        texture->setMaterial( material );
                                        texture->setName( textureTypeStr );
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
                                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                                        texture->setParent( this );

                                        // auto textureType = (PbsTextureTypes)i;
                                        auto textureTypeStr = String(
                                            "Base" );  // GraphicsUtil::getPbsTextureType(textureType);

                                        texture->setTextureType( (u32)i );

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
        }

        void CMaterialPassOgre::reload( SmartPtr<ISharedObject> data )
        {
            auto parent = getParent();

            unload( data );

            setParent( parent );
            load( data );

            setupMaterial();
        }

        void CMaterialPassOgre::setupMaterial()
        {
            try
            {
                auto parent = getParent();
                auto technique = workphone::static_pointer_cast<CMaterialTechniqueOgre>( parent );
                if( technique )
                {
                    auto owner = technique->getMaterial();
                    auto material = workphone::static_pointer_cast<CMaterialOgre>( owner );

                    setMaterial( material );
                }

                auto d = getDiffuse();
                auto e = getEmissive();

                if( auto pass = getPass() )
                {
                    auto diffuse = Ogre::ColourValue( d.r, d.g, d.b, d.a );
                    pass->setDiffuse( diffuse );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialPassOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    MaterialPass::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialPassOgre::initialise( Ogre::Pass *pass )
        {
            m_pass = pass;
        }

        void CMaterialPassOgre::setSceneBlending( u32 blendType )
        {
        }

        bool CMaterialPassOgre::isDepthCheckEnabled() const
        {
            return false;
        }

        void CMaterialPassOgre::setDepthCheckEnabled( bool enabled )
        {
        }

        bool CMaterialPassOgre::isDepthWriteEnabled() const
        {
            return false;
        }

        void CMaterialPassOgre::setDepthWriteEnabled( bool enabled )
        {
        }

        u32 CMaterialPassOgre::getCullingMode() const
        {
            return 0;
        }

        void CMaterialPassOgre::setCullingMode( u32 mode )
        {
        }

        SmartPtr<IMaterialTexture> CMaterialPassOgre::createTextureUnit()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            auto materialTexture = factoryManager->make_ptr<CMaterialTextureOgre>();

            auto material = getMaterial();
            WP_ASSERT( material );

            materialTexture->setMaterial( material );

            addTextureUnit( materialTexture );

            return materialTexture;
        }

        void CMaterialPassOgre::setTexture( SmartPtr<ITexture> tex, u32 layerIdx /*= 0 */ )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();

                auto factoryManager = applicationManager->getFactoryManager();

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

                    for( size_t i = 0; i < m_textures.size(); ++i )
                    {
                        auto &texture = m_textures[i];
                        if( !texture )
                        {
                            texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                        }
                    }

                    auto &texture = m_textures[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
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
                            texture = factoryManager->make_ptr<CMaterialTextureOgre>();
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

        void CMaterialPassOgre::setTexture( const String &fileName, u32 layerIdx /*= 0 */ )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();

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

                    for( size_t i = 0; i < m_textures.size(); ++i )
                    {
                        auto &texture = m_textures[i];
                        if( !texture )
                        {
                            texture = factoryManager->make_ptr<CMaterialTextureOgre>();
                        }
                    }

                    auto &texture = m_textures[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgre>();
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
                            texture = factoryManager->make_ptr<CMaterialTextureOgre>();
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

        void CMaterialPassOgre::setCubicTexture( const String &fileName, bool uvw,
                                                 u32 layerIdx /*= 0 */ )
        {
        }

        void CMaterialPassOgre::setFragmentParam( const String &name, f32 value )
        {
        }

        void CMaterialPassOgre::setFragmentParam( const String &name, const Vector2F &value )
        {
        }

        void CMaterialPassOgre::setFragmentParam( const String &name, const Vector3F &value )
        {
        }

        void CMaterialPassOgre::setFragmentParam( const String &name, const Vector4F &value )
        {
        }

        void CMaterialPassOgre::setFragmentParam( const String &name, const ColourF &value )
        {
        }

        SmartPtr<Properties> CMaterialPassOgre::getProperties() const
        {
            auto properties = MaterialNode<IMaterialPass>::getProperties();

            // auto handle = getHandle();
            // properties->setProperty("name", handle->getName());

            static const auto mainTextureStr = String( "Main Texture" );

            // properties->setProperty("Material Name", m_materialName);
            // properties->setProperty(mainTextureStr, m_mainTexturePath);

            auto ambient = getAmbient();
            auto diffuse = getDiffuse();
            auto specular = getSpecular();
            auto emissive = getEmissive();
            auto tint = getTint();

            auto metalness = getMetalness();
            auto roughness = getRoughness();

            properties->setProperty( "ambient", ambient );
            properties->setProperty( "diffuse", diffuse );
            properties->setProperty( "specular", specular );
            properties->setProperty( "emissive", emissive );
            properties->setProperty( "tint", tint );
            properties->setProperty( "metalness", metalness );
            properties->setProperty( "roughness", roughness );

            return properties;
        }

        void CMaterialPassOgre::setProperties( SmartPtr<Properties> properties )
        {
            auto ambient = getAmbient();
            auto diffuse = getDiffuse();
            auto specular = getSpecular();
            auto emissive = getEmissive();
            auto tint = getTint();

            auto metalness = getMetalness();
            auto roughness = getRoughness();

            // properties->getPropertyValue("Material Name", m_materialName);
            // properties->getPropertyValue("Main Texture", m_mainTexturePath);
            properties->getPropertyValue( "ambient", ambient );
            properties->getPropertyValue( "diffuse", diffuse );
            properties->getPropertyValue( "specular", specular );
            properties->getPropertyValue( "emissive", emissive );
            properties->getPropertyValue( "tint", tint );

            properties->getPropertyValue( "metalness", metalness );
            properties->getPropertyValue( "roughness", roughness );

            setAmbient( ambient );
            setDiffuse( diffuse );
            setSpecular( specular );
            setEmissive( emissive );
            setTint( tint );

            setMetalness( metalness );
            setRoughness( roughness );
        }

        Array<SmartPtr<ISharedObject>> CMaterialPassOgre::getChildObjects() const
        {
            auto textures = getTextureUnits();

            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( textures.size() );

            for( auto texture : textures )
            {
                objects.push_back( texture );
            }

            return objects;
        }

        bool CMaterialPassOgre::MaterialPassOgreStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            if( auto owner = workphone::static_pointer_cast<CMaterialPassOgre>( getOwner() ) )
            {
                const auto &loadingState = owner->getLoadingState();
                if( loadingState == LoadingState::Loaded )
                {
                    auto passState =
                        workphone::static_pointer_cast<MaterialPassStateData>( state->getData() );
                    auto diffuse = OgreUtil::convertToOgre( passState->diffuseColour );
                    auto specular = OgreUtil::convertToOgre( passState->specularColour );
                    auto emissive = OgreUtil::convertToOgre( passState->emissiveColour );
                    auto tint = OgreUtil::convertToOgre( passState->tintColour );
                    auto ambient = OgreUtil::convertToOgre( passState->ambientColour );
                    auto lightingEnabled = passState->getFlag( IMaterial::lightingEnabledFlag );

                    if( auto pass = owner->getPass() )
                    {
                        pass->setDiffuse( diffuse );
                        pass->setSpecular( specular );
                        pass->setEmissive( emissive );
                        //pass->setTint( tint );
                        //pass->setDiffuse( diffuse );

                        pass->setLightingEnabled( lightingEnabled );
                    }

                    state->setDirty( false );
                }
            }

            return false;
        }

        bool CMaterialPassOgre::MaterialPassOgreStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isExactly<StateMessageVector4>() )
                {
                    auto vectorMessage = workphone::static_pointer_cast<StateMessageVector4>( message );
                    auto value = vectorMessage->getValue();
                    auto type = vectorMessage->getType();

                    if( type == DIFFUSE_HASH )
                    {
                        owner->setDiffuse( ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                    }
                    else if( type == EMISSION_HASH )
                    {
                        owner->setEmissive( ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                    }
                }
            }

            return false;
        }

        Ogre::Pass *CMaterialPassOgre::getPass() const
        {
            return m_pass;
        }

        void CMaterialPassOgre::setPass( Ogre::Pass *pass )
        {
            m_pass = pass;
        }

    }  // end namespace render
}  // namespace workphone
