#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTechniqueOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialPassOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreImage2.h>
#include <OgreTextureGpuManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreRoot.h>
#include <OgreResourceGroupManager.h>
#include <OgreGpuProgramParams.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlmsManager.h>

#include <cmath>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialOgreNext, Material );

    namespace
    {

        Ogre::TextureAddressingMode getTextureAddressingMode( u32 wrapMode )
        {
            switch( wrapMode )
            {
            case 1u:
                return Ogre::TAM_CLAMP;
            case 2u:
                return Ogre::TAM_MIRROR;
            case 3u:
                return Ogre::TAM_MIRROR;
            case 4u:
                return Ogre::TAM_BORDER;
            default:
                return Ogre::TAM_WRAP;
            }
        }

        Ogre::TextureFilterOptions getTextureFilterOptions( u32 filterMode )
        {
            switch( filterMode )
            {
            case 0u:
                return Ogre::TFO_NONE;
            case 2u:
                return Ogre::TFO_TRILINEAR;
            case 3u:
                return Ogre::TFO_ANISOTROPIC;
            default:
                return Ogre::TFO_BILINEAR;
            }
        }

        f32 getClampedAnisotropy( f32 value )
        {
            if( value < 1.0f )
            {
                return 1.0f;
            }

            if( value > 16.0f )
            {
                return 16.0f;
            }

            return value;
        }

        Ogre::HlmsSamplerblock getPbsSamplerblock( Ogre::PbsTextureTypes textureType,
                                                   const MaterialPassStateData &materialState )
        {
            Ogre::HlmsSamplerblock hlmsSamplerblock;
            hlmsSamplerblock.setFiltering( getTextureFilterOptions( materialState.uvFilter ) );
            hlmsSamplerblock.mMaxAnisotropy = getClampedAnisotropy( materialState.uvAniso );

            if( textureType == Ogre::PBSM_REFLECTION )
            {
                hlmsSamplerblock.mU = Ogre::TAM_CLAMP;
                hlmsSamplerblock.mV = Ogre::TAM_CLAMP;
                hlmsSamplerblock.mW = Ogre::TAM_CLAMP;
            }
            else
            {
                hlmsSamplerblock.mU = getTextureAddressingMode( materialState.uvWrapU );
                hlmsSamplerblock.mV = getTextureAddressingMode( materialState.uvWrapV );
                hlmsSamplerblock.mW = hlmsSamplerblock.mV;
            }

            return hlmsSamplerblock;
        }

        Ogre::TextureGpu *getTextureGpu( const SmartPtr<ITexture> &texture, bool &loadingPending )
        {
            auto ogreNextTexture = workphone::dynamic_pointer_cast<CTextureOgreNext>( texture );
            if( !ogreNextTexture )
            {
                return nullptr;
            }

            if( auto pTexture = ogreNextTexture->getTexture() )
            {
                return pTexture;
            }

            const auto loadingState = ogreNextTexture->getLoadingState();
            if( loadingState == LoadingState::Loading )
            {
                loadingPending = true;
                return nullptr;
            }

            if( loadingState != LoadingState::Loaded )
            {
                ogreNextTexture->load( nullptr );
            }

            if( !ogreNextTexture->getTexture() &&
                ogreNextTexture->getLoadingState() == LoadingState::Loading )
            {
                loadingPending = true;
            }

            return ogreNextTexture->getTexture();
        }

        void applyPbsUvTransform( CMaterialOgreNext &material, f32 tilingX, f32 tilingY, f32 offsetX,
                                  f32 offsetY, f32 rotationDegrees )
        {
            auto pDataBlock = material.getHlmsDatablock();
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( pDataBlock );
            if( !pbsDatablock )
            {
                return;
            }

            const auto rotationRadians = rotationDegrees * Ogre::Math::PI / 180.0f;
            const auto sinRotation = std::sin( rotationRadians );
            const auto cosRotation = std::cos( rotationRadians );

            pbsDatablock->setUserValue(
                0u, Ogre::Vector4( offsetX, offsetY, tilingX - 1.0f, tilingY - 1.0f ) );
            pbsDatablock->setUserValue( 1u,
                                        Ogre::Vector4( sinRotation, cosRotation - 1.0f, 0.0f, 0.0f ) );
        }

        void applyPbsDetailUvTransform( CMaterialOgreNext &material, f32 tilingX, f32 tilingY,
                                        f32 offsetX, f32 offsetY )
        {
            auto pDataBlock = material.getHlmsDatablock();
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( pDataBlock );
            if( !pbsDatablock )
            {
                return;
            }

            const Ogre::Vector4 offsetScale( offsetX, offsetY, tilingX, tilingY );
            for( auto i = 0u; i < 4u; ++i )
            {
                pbsDatablock->setDetailMapOffsetScale( static_cast<Ogre::uint8>( i ), offsetScale );
            }
        }

        void applyPbsEditorUvSettings( CMaterialOgreNext &material,
                                       const MaterialPassStateData &materialState )
        {
            auto pDataBlock = material.getHlmsDatablock();
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( pDataBlock );
            if( !pbsDatablock )
            {
                return;
            }

            applyPbsUvTransform( material, materialState.uvTilingX, materialState.uvTilingY,
                                 materialState.uvOffsetX, materialState.uvOffsetY,
                                 materialState.uvRotation );

            applyPbsDetailUvTransform( material, materialState.detailTilingX,
                                       materialState.detailTilingY, materialState.detailOffsetX,
                                       materialState.detailOffsetY );

            const auto storedProjection = materialState.uvProjection;
            const auto projection = storedProjection <= 6u ? storedProjection : 0u;
            const auto storedProjectionScale = materialState.uvTriplanarScale;
            const auto projectionScale = std::isfinite( storedProjectionScale )
                                             ? std::max( storedProjectionScale, 0.0001f )
                                             : 1.0f;
            pbsDatablock->setUserValue( 2u, Ogre::Vector4( static_cast<Ogre::Real>( projection ),
                                                           projectionScale, 0.0f, 0.0f ) );

            // Generated projections do not need a mesh UV stream. Keep UV0 selected so
            // Ogre does not request a missing UV channel while the shader replaces it.
            const auto storedUvSet = materialState.uvSet;
            const auto uvSet = projection == 0u && storedUvSet <= 3u ? storedUvSet : 0u;
            for( auto i = 0u; i < static_cast<u32>( Ogre::NUM_PBSM_SOURCES ); ++i )
            {
                pbsDatablock->setTextureUvSource( static_cast<Ogre::PbsTextureTypes>( i ),
                                                  static_cast<Ogre::uint8>( uvSet ) );
            }
        }

        void applyPbsEditorNormalSettings( CMaterialOgreNext &material,
                                           const MaterialPassStateData &materialState )
        {
            auto pDataBlock = material.getHlmsDatablock();
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( pDataBlock );
            if( !pbsDatablock )
            {
                return;
            }

            pbsDatablock->setNormalMapWeight( materialState.normalStrength );

            for( auto i = 0u; i < 4u; ++i )
            {
                pbsDatablock->setDetailNormalWeight( static_cast<Ogre::uint8>( i ),
                                                     materialState.detailNormalStrength );
            }
        }

        bool applyPbsTextures( CMaterialOgreNext &material, const MaterialPassStateData &materialState )
        {
            auto pDataBlock = material.getHlmsDatablock();
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( pDataBlock );
            if( !pbsDatablock )
            {
                return true;
            }

            auto allTexturesApplied = true;
            size_t count = 0;
            auto &textures = materialState.textures;
            for( auto texture : textures )
            {
                auto textureType = (Ogre::PbsTextureTypes)count;
                auto textureDirty =
                    count < materialState.textureDirty.size() && materialState.textureDirty[count];

                try
                {
                    if( texture )
                    {
                        auto loadingPending = false;
                        auto pTexture = getTextureGpu( texture, loadingPending );
                        if( pTexture )
                        {
                            auto hlmsSamplerblock = getPbsSamplerblock( textureType, materialState );
                            pbsDatablock->setTexture( textureType, pTexture, &hlmsSamplerblock );
                        }
                        else if( loadingPending )
                        {
                            allTexturesApplied = false;
                        }
                    }
                    else if( textureDirty )
                    {
                        pbsDatablock->setTexture( textureType,
                                                  static_cast<Ogre::TextureGpu *>( nullptr ), nullptr );
                    }
                }
                catch( Ogre::Exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                    allTexturesApplied = false;
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                    allTexturesApplied = false;
                }

                count++;
            }

            return allTexturesApplied;
        }

        void setupPasses( CMaterialOgreNext &material )
        {
            auto techniques = material.getTechniques();
            for( auto &technique : techniques )
            {
                if( technique )
                {
                    auto passes = technique->getPasses();
                    for( auto &pass : passes )
                    {
                        if( pass && pass->isDerived<CMaterialPassOgreNext>() )
                        {
                            auto ogreNextPass =
                                workphone::static_pointer_cast<CMaterialPassOgreNext>( pass );
                            ogreNextPass->setupMaterial();
                        }
                    }
                }
            }
        }

        bool applyMaterialState( CMaterialOgreNext &material,
                                 const MaterialPassStateData &materialState )
        {
            auto allTexturesApplied = applyPbsTextures( material, materialState );
            applyPbsEditorUvSettings( material, materialState );
            applyPbsEditorNormalSettings( material, materialState );
            setupPasses( material );
            return allTexturesApplied;
        }

        bool applyPrimaryPassState( CMaterialOgreNext &material )
        {
            if( auto stateContext = material.getStateContext() )
            {
                for( auto technique : material.getTechniques() )
                {
                    for( auto pass : technique->getPasses() )
                    {
                        if( auto state =
                                stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                        {
                            return applyMaterialState( material, *state );
                        }
                    }
                }
            }

            return true;
        }
    }  // namespace

    CMaterialOgreNext::CMaterialOgreNext()
    {
        createStateObject();
    }

    CMaterialOgreNext::CMaterialOgreNext( IResourceManager *resourceManager ) : CMaterialOgreNext()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

        setResourceManager( resourceManager );
    }

    CMaterialOgreNext::~CMaterialOgreNext()
    {
        unload( nullptr );
    }

    void CMaterialOgreNext::createStateObject()
    {
        auto pThis = getSharedFromThis<ISharedObject>();

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto graphicsFactoryManager = graphicsSystem->getFactoryManagerPtr();
        WP_ASSERT( graphicsFactoryManager );

        auto materialManager = graphicsSystem->getMaterialManagerPtr();
        WP_ASSERT( materialManager );

        auto stateContext = materialManager->getStateContext();
        WP_ASSERT( stateContext );

        if( stateContext->getStateDataById<MaterialStateData>( getId() ) )
        {
            return;
        }

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<MaterialStateData>();
        state->setData( stateData );
    }

    void CMaterialOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            WP_ASSERT( isLoaded() == false );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            WP_ASSERT( resourceGroupManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            // WP_ASSERT(getLoadingState() == LoadingState::Unloaded);
            setLoadingState( LoadingState::Loading );

            Material::load( data );

            createMaterialByType();

            auto techniques = getTechniques();
            for( auto &technique : techniques )
            {
                if( !technique->isLoaded() )
                {
                    technique->load( nullptr );
                }
            }

            if( !applyPrimaryPassState( *this ) )
            {
                if( auto stateContext = getStateContext() )
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

    void CMaterialOgreNext::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( task == renderTask )
            {
                if( m_material )
                {
                    m_material->changeGroupOwnership(
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

                    m_material->reload();

                    for( u32 techIdx = 0; techIdx < m_material->getNumTechniques(); ++techIdx )
                    {
                        Ogre::Technique *tech = m_material->getTechnique( techIdx );
                        for( u32 passIdx = 0; passIdx < tech->getNumPasses(); ++passIdx )
                        {
                            Ogre::Pass *pass = tech->getPass( passIdx );

                            for( u32 texIdx = 0; texIdx < pass->getNumTextureUnitStates(); ++texIdx )
                            {
                                Ogre::TextureUnitState *texState = pass->getTextureUnitState( texIdx );
                                texState->retryTextureLoad();
                            }
                        }
                    }
                }

                for( auto technique : m_techniques )
                {
                    technique->reload( data );
                }

                auto message = factoryManager->make_ptr<StateMessageLoad>();
                message->setType( StateMessageLoad::LOADED_HASH );
                message->setSender( this );
                message->setObject( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageLoad>();
                message->setType( StateMessageLoad::RELOAD_HASH );
                message->setSender( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &state = getLoadingState();
            if( state != LoadingState::Unloaded )
            {
                ScopedLock lock( this );

                setLoadingState( LoadingState::Unloading );

                auto techniques = m_techniques.snapshot();
                for( auto technique : techniques )
                {
                    technique->unload( data );
                }

                m_techniques.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialOgreNext::setCubicTexture( const String &fileName, bool uvw, u32 layerIdx )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageSetTexture>();
            message->setType( SET_TEXTURE_HASH );
            message->setSender( this );
            message->setTextureName( fileName );
            message->setTextureIndex( layerIdx );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    Ogre::TextureUnitState *textureUnitState = pass->getTextureUnitState( layerIdx );
                    if( textureUnitState )
                    {
                        textureUnitState->setCubicTextureName( fileName.c_str(), uvw );
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setCubicTexture( const Array<SmartPtr<ITexture>> &textures, u32 layerIdx )
    {
        WP_UNUSED( layerIdx );

        if( textures.size() < static_cast<size_t>( SkyboxTextureTypes::Count ) )
        {
            WP_LOG_ERROR( "Invalid texture array size" );
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto message = factoryManager->make_ptr<StateMessageObjectsArray>();
            message->setType( StateMessage::SET_CUBEMAP );
            message->setSender( this );
            message->setObjects( Array<SmartPtr<ISharedObject>>( textures.begin(), textures.end() ) );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }

            return;
        }

        auto textureManager = graphicsSystem->getTextureManager();
        WP_ASSERT( textureManager );

        ScopedLock lock( graphicsSystem );

        Array<String> textureNames;
        textureNames.resize( 6 );

        Array<u32> map;
        map.reserve( 6 );

        map.push_back( static_cast<u32>( SkyboxTextureTypes::Right ) );
        map.push_back( static_cast<u32>( SkyboxTextureTypes::Left ) );
        map.push_back( static_cast<u32>( SkyboxTextureTypes::Up ) );
        map.push_back( static_cast<u32>( SkyboxTextureTypes::Down ) );
        map.push_back( static_cast<u32>( SkyboxTextureTypes::Front ) );
        map.push_back( static_cast<u32>( SkyboxTextureTypes::Back ) );

        for( size_t i = 0; i < 6; ++i )
        {
            auto index = map[i];

            if( index < textures.size() )
            {
                auto pTexture = textures[index];
                if( pTexture )
                {
                    textureNames[i] = pTexture->getFilePath();
                }
                else
                {
                    textureNames[i] = "checker.png";
                }
            }
        }

        m_cubeTexture = textureManager->createCubeMap( textureNames );
    }

    void CMaterialOgreNext::setFragmentParam( const String &name, f32 value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_FLOAT_HASH );
            message->setSender( this );
            message->setName( name );
            message->setFloat( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    if( pass->hasFragmentProgram() )
                    {
                        Ogre::GpuProgramParametersSharedPtr fragmentParameters =
                            pass->getFragmentProgramParameters();
                        const Ogre::GpuNamedConstants &fragmentNamedConstants =
                            fragmentParameters->getConstantDefinitions();
                        fragmentParameters->setNamedConstant( name.c_str(), value );
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setFragmentParam( const String &name, const Vector2F &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_VECTOR2F_HASH );
            message->setSender( this );
            message->setName( name );
            message->setVector2f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    if( pass->hasFragmentProgram() )
                    {
                        Ogre::GpuProgramParametersSharedPtr fragmentParameters =
                            pass->getFragmentProgramParameters();
                        const Ogre::GpuNamedConstants &fragmentNamedConstants =
                            fragmentParameters->getConstantDefinitions();
                        // fragmentParameters->setNamedConstant(name.c_str(),
                        // Ogre::Vector2(value.X(), value.Y()));
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setFragmentParam( const String &name, const Vector3F &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_VECTOR3F_HASH );
            message->setSender( this );
            message->setName( name );
            message->setVector3f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    if( pass->hasFragmentProgram() )
                    {
                        Ogre::GpuProgramParametersSharedPtr fragmentParameters =
                            pass->getFragmentProgramParameters();
                        const Ogre::GpuNamedConstants &fragmentNamedConstants =
                            fragmentParameters->getConstantDefinitions();
                        fragmentParameters->setNamedConstant(
                            name.c_str(), Ogre::Vector3( value.X(), value.Y(), value.Z() ) );
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setFragmentParam( const String &name, const Vector4F &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_VECTOR4F_HASH );
            message->setSender( this );
            message->setName( name );
            message->setVector4f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    if( pass->hasFragmentProgram() )
                    {
                        Ogre::GpuProgramParametersSharedPtr fragmentParameters =
                            pass->getFragmentProgramParameters();
                        const Ogre::GpuNamedConstants &fragmentNamedConstants =
                            fragmentParameters->getConstantDefinitions();
                        fragmentParameters->setNamedConstant(
                            name.c_str(), Ogre::Vector4( value.X(), value.Y(), value.Z(), value.W() ) );
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setFragmentParam( const String &name, const ColourF &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task != renderTask )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_COLOUR_HASH );
            message->setSender( this );
            message->setName( name );
            message->setColourf( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
        else if( m_material )
        {
            Ogre::Technique *tech = m_material->getBestTechnique();
            if( tech )
            {
                Ogre::Pass *pass = tech->getPass( 0 );
                if( pass )
                {
                    if( pass->hasFragmentProgram() )
                    {
                        Ogre::GpuProgramParametersSharedPtr fragmentParameters =
                            pass->getFragmentProgramParameters();
                        const Ogre::GpuNamedConstants &fragmentNamedConstants =
                            fragmentParameters->getConstantDefinitions();
                        fragmentParameters->setNamedConstant(
                            name.c_str(), Ogre::ColourValue( value.r, value.g, value.b, value.a ) );
                    }
                }
            }
        }
    }

    void CMaterialOgreNext::setScale( const Vector3F &scale, u32 textureIndex /*= 0*/,
                                      u32 passIndex /*= 0*/, u32 techniqueIndex /*= 0 */ )
    {
        if( m_material )
        {
            auto t = m_material->getTechnique( techniqueIndex );
            if( t && passIndex < t->getNumPasses() )
            {
                auto pass = t->getPass( passIndex );
                if( pass && textureIndex < pass->getNumTextureUnitStates() )
                {
                    const auto offset = getUVOffset();
                    auto tex = pass->getTextureUnitState( textureIndex );
                    tex->setTextureScale( scale.X(), scale.Y() );
                    tex->setTextureScroll( offset.X(), offset.Y() );
                    tex->setTextureRotate( Ogre::Degree( getUVRotation() ) );
                }
            }
        }

        if( textureIndex == 0u )
        {
            const auto offset = getUVOffset();
            applyPbsUvTransform( *this, scale.X(), scale.Y(), offset.X(), offset.Y(), getUVRotation() );
        }
    }

    void CMaterialOgreNext::setUVTiling( const Vector2F &tiling )
    {
        Material::setUVTiling( tiling );
    }

    void CMaterialOgreNext::setUVOffset( const Vector2F &offset )
    {
        Material::setUVOffset( offset );

        const auto tiling = getUVTiling();
        setScale( Vector3F( tiling.X(), tiling.Y(), 1.0f ), 0u );
    }

    void CMaterialOgreNext::setUVRotation( f32 rotation )
    {
        Material::setUVRotation( rotation );

        const auto tiling = getUVTiling();
        setScale( Vector3F( tiling.X(), tiling.Y(), 1.0f ), 0u );
    }

    void CMaterialOgreNext::createMaterialByType()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "CMaterialOgreNext::createMaterialByType: application manager is null." );
                return;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG_ERROR( "CMaterialOgreNext::createMaterialByType: graphics system is null." );
                return;
            }

            TryLockGuard lock( graphicsSystem );
            if( !lock.locked() )
            {
                WP_LOG_WARNING(
                    "CMaterialOgreNext::createMaterialByType could not lock the graphics system; "
                    "skipping this material creation attempt to avoid a deadlock." );
                return;
            }

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
            {
                WP_LOG_ERROR( "CMaterialOgreNext::createMaterialByType: Ogre root is null." );
                return;
            }

            auto hlmsManager = root->getHlmsManager();
            if( !hlmsManager )
            {
                WP_LOG_ERROR( "CMaterialOgreNext::createMaterialByType: HLMS manager is null." );
                return;
            }

            const auto materialType = getMaterialType();
            auto materialTypeApplied = false;

            switch( materialType )
            {
            case MaterialType::Standard:
            case MaterialType::StandardSpecular:
            case MaterialType::StandardTriPlanar:
            {
                auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( m_pbsDatablock.load() );
                if( !datablock )
                {
                    auto hlmsPbs =
                        dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
                    if( hlmsPbs )
                    {
                        m_pbsDatablockName = StringUtil::getUUID();
                        auto createdDatablock = hlmsPbs->createDatablock(
                            m_pbsDatablockName.c_str(), m_pbsDatablockName.c_str(),
                            Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() );
                        datablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( createdDatablock );
                        m_pbsDatablock = datablock;
                    }
                }

                if( datablock )
                {
                    datablock->setReceiveShadows( true );
                    setDatablockName( m_pbsDatablockName );
                    setHlmsDatablock( datablock );
                    materialTypeApplied = true;
                }
                else
                {
                    WP_LOG_ERROR(
                        "CMaterialOgreNext::createMaterialByType: PBS datablock unavailable." );
                }
            }
            break;
            case MaterialType::TerrainStandard:
            case MaterialType::TerrainSpecular:
            case MaterialType::TerrainDiffuse:
            {
                // Terrain materials use a separate renderer path. Do not leave a stale PBS or
                // Unlit datablock attached when one is selected through a generic property grid.
                setDatablockName( String() );
                setHlmsDatablock( nullptr );
                materialTypeApplied = true;
            }
            break;
            case MaterialType::Skybox:
            {
                setDatablockName( String() );
                setHlmsDatablock( nullptr );
                materialTypeApplied = true;

                /*for( auto t : m_techniques )
                {
                    auto technique = fb::static_pointer_cast<CMaterialTechniqueOgreNext>( t );

                    auto pTechnique = technique->getTechnique();
                    auto materialPasses = pTechnique->getPasses();
                    auto passes = t->getPasses();

                    auto passCount = 0;
                    for( auto p : passes )
                    {
                        auto pass = fb::static_pointer_cast<CMaterialPassOgreNext>( p );

                        auto pPass = materialPasses[passCount];
                        pass->setPass( pPass );
                        passCount++;

                        if( passCount == materialPasses.size() )
                        {
                            break;
                        }
                    }

                    for( auto p : passes )
                    {
                        auto numTextureUnits = p->getNumTexturesNodes();

                        for( size_t i = numTextureUnits > 0 ? numTextureUnits - 1 : 0;
                             i < static_cast<size_t>( 6 ); ++i )
                        {
                            p->createTextureUnit();
                        }
                    }
                }*/
            }
            break;
            case MaterialType::SkyboxCubemap:
            {
                auto techniques = m_techniques.snapshot();
                for( auto t : techniques )
                {
                    auto technique = workphone::static_pointer_cast<CMaterialTechniqueOgreNext>( t );

                    if( auto pTechnique = technique->getTechnique() )
                    {
                        auto materialPasses = Array<Ogre::Pass *>();
                        auto passIt = pTechnique->getPassIterator();
                        while( passIt.peekNext() )
                        {
                            auto p = *passIt.current();
                            materialPasses.push_back( p );
                            passIt.moveNext();
                        }

                        auto passes = t->getPasses();

                        auto passCount = 0;
                        for( auto p : passes )
                        {
                            if( passCount >= materialPasses.size() )
                            {
                                break;
                            }

                            auto pass = workphone::static_pointer_cast<CMaterialPassOgreNext>( p );

                            auto pPass = materialPasses[passCount];
                            pass->setPass( pPass );
                            passCount++;

                            //pPass->setLightingEnabled( false );
                            //pPass->setDepthWriteEnabled( false );

                            if( passCount == materialPasses.size() )
                            {
                                break;
                            }
                        }

                        for( auto p : passes )
                        {
                            auto numTextureUnits = p->getNumTexturesNodes();

                            if( numTextureUnits != 1 )
                            {
                                auto pPass = workphone::static_pointer_cast<CMaterialPassOgreNext>( p );
                                pPass->reload( nullptr );

                                auto ogrePass = pPass->getPass();
                                if( !ogrePass )
                                {
                                    continue;
                                }

                                auto tu = ogrePass->createTextureUnitState();

                                auto textureUnit = p->createTextureUnit();
                                auto pTextureUnit =
                                    workphone::static_pointer_cast<CMaterialTextureOgreNext>(
                                        textureUnit );
                                pTextureUnit->setTextureUnitState( tu );
                                textureUnit->load( nullptr );
                            }
                        }
                    }
                }

                setDatablockName( String() );
                setHlmsDatablock( nullptr );
                materialTypeApplied = true;
            }
            break;
            case MaterialType::UI:
            {
                auto datablock = dynamic_cast<Ogre::HlmsUnlitDatablock *>( m_unlitDatablock.load() );
                if( !datablock )
                {
                    if( auto hlmsUnlit = hlmsManager->getHlms( Ogre::HLMS_UNLIT ) )
                    {
                        Ogre::HlmsMacroblock macroblock;
                        macroblock.mDepthCheck = false;

                        m_unlitDatablockName = StringUtil::getUUID();
                        auto createdDatablock = hlmsUnlit->createDatablock(
                            m_unlitDatablockName.c_str(), m_unlitDatablockName.c_str(), macroblock,
                            Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() );
                        datablock = dynamic_cast<Ogre::HlmsUnlitDatablock *>( createdDatablock );
                        m_unlitDatablock = datablock;
                    }
                }

                if( datablock )
                {
                    setDatablockName( m_unlitDatablockName );
                    setHlmsDatablock( datablock );
                    materialTypeApplied = true;
                }
                else
                {
                    WP_LOG_ERROR(
                        "CMaterialOgreNext::createMaterialByType: Unlit datablock unavailable." );
                }
            }
            break;
            case MaterialType::Custom:
            {
                if( m_material.isNull() )
                {
                    auto materialName = getName();
                    if( StringUtil::isNullOrEmpty( materialName ) )
                    {
                        materialName = StringUtil::getUUID();
                    }

                    auto &materialManager = Ogre::MaterialManager::getSingleton();
                    auto resourceName = Ogre::String( materialName.c_str() );
                    auto resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                    if( materialManager.resourceExists( resourceName ) )
                    {
                        m_material = materialManager.getByName( resourceName, resourceGroup );
                    }
                    else
                    {
                        m_material = materialManager.create( resourceName, resourceGroup );
                    }

                    if( m_material )
                    {
                        m_material->removeAllTechniques();
                    }
                }

                setDatablockName( String() );
                setHlmsDatablock( nullptr );
                materialTypeApplied = static_cast<bool>( m_material );
            }
            break;
            default:
            {
                WP_LOG_WARNING(
                    "CMaterialOgreNext::createMaterialByType ignored an invalid material type." );
            }
            break;
            }

            if( materialTypeApplied )
            {
                m_appliedMaterialType = materialType;
            }

            auto techniques = getTechniques();
            if( techniques.empty() )
            {
                auto technique = createTechnique();
                if( technique )
                {
                    technique->load( nullptr );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CMaterialOgreNext::createTechnique() -> SmartPtr<IMaterialTechnique>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto technique = factoryManager->make_ptr<CMaterialTechniqueOgreNext>();
        technique->setMaterial( this );

        if( m_material )
        {
            auto ogreTechnique = m_material->createTechnique();
            technique->initialise( ogreTechnique );
        }

        addTechnique( technique );
        return technique;
    }

    auto CMaterialOgreNext::getHlmsDatablock() const -> Ogre::HlmsDatablock *
    {
        return m_hlmsDatablock;
    }

    void CMaterialOgreNext::setHlmsDatablock( Ogre::HlmsDatablock *hlmsDatablock )
    {
        m_hlmsDatablock = hlmsDatablock;
    }

    auto CMaterialOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto techniques = getTechniques();

        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( techniques.size() );

        for( auto technique : techniques )
        {
            objects.emplace_back( technique );
        }

        return objects;
    }

    auto CMaterialOgreNext::getDatablockName() const -> String
    {
        return m_datablockName;
    }

    void CMaterialOgreNext::setDatablockName( const String &datablockName )
    {
        m_datablockName = datablockName;
    }

    bool CMaterialOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto techniques = getTechniques();
        for( auto &technique : techniques )
        {
            if( technique )
            {
                if( technique->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        if( message->getSender() == this )
        {
            if( message->isExactly<StateMessageUIntValue>() )
            {
                auto valueMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );
                auto messageType = valueMessage->getType();

                if( messageType == StringUtil::getHash( "materialType" ) )
                {
                    auto materialType = static_cast<MaterialType>( valueMessage->getValue() );
                    if( materialType < MaterialType::Count )
                    {
                        setMaterialType( materialType );
                    }
                }
            }
            else if( message->isExactly<StateMessageLoad>() )
            {
                auto loadMessage = workphone::static_pointer_cast<StateMessageLoad>( message );
                auto messageType = loadMessage->getType();

                if( messageType == StateMessageLoad::LOAD_HASH )
                {
                    load( nullptr );
                }
                else if( messageType == StateMessageLoad::RELOAD_HASH )
                {
                    reload( nullptr );
                }
            }
            else if( message->isExactly<StateMessageFragmentParam>() )
            {
                auto fragmentMessage =
                    workphone::static_pointer_cast<StateMessageFragmentParam>( message );
                auto type = fragmentMessage->getType();

                if( type == FRAGMENT_FLOAT_HASH )
                {
                    setFragmentParam( fragmentMessage->getName(), fragmentMessage->getFloat() );
                }
                else if( type == FRAGMENT_VECTOR2F_HASH )
                {
                    setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector2f() );
                }
                else if( type == FRAGMENT_VECTOR3F_HASH )
                {
                    setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector3f() );
                }
                else if( type == FRAGMENT_VECTOR4F_HASH )
                {
                    setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector4f() );
                }
                else if( type == FRAGMENT_COLOUR_HASH )
                {
                    setFragmentParam( fragmentMessage->getName(), fragmentMessage->getColourf() );
                }
            }
            else if( message->isExactly<StateMessageSetTexture>() )
            {
                auto textureMessage = workphone::static_pointer_cast<StateMessageSetTexture>( message );
                if( auto texture = textureMessage->getTexture() )
                {
                    const auto textureIndex = textureMessage->getTextureIndex();
                    setTexture( texture, textureIndex );
                }
                else
                {
                    const auto textureName = textureMessage->getTextureName();
                    const auto textureIndex = textureMessage->getTextureIndex();
                    setTexture( textureName, textureIndex );
                }
            }
            else if( message->isExactly<StateMessageObjectsArray>() )
            {
                auto arrayMessage = workphone::static_pointer_cast<StateMessageObjectsArray>( message );
                auto type = arrayMessage->getType();
                auto value = arrayMessage->getObjects();

                if( type == StateMessage::SET_CUBEMAP )
                {
                    setCubicTexture( Array<SmartPtr<ITexture>>( value.begin(), value.end() ) );
                }
            }
        }

        return false;
    }

    bool CMaterialOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !state )
        {
            return false;
        }

        if( isLoaded() )
        {
            auto techniques = getTechniques();

            // Material type now belongs to a pass. Rebuild the shared Ogre datablock before
            // dispatching a primary-pass change so that the pass applies its state to the right
            // HLMS implementation.
            if( auto stateData = state->getData() )
            {
                if( stateData->isExactly<MaterialPassStateData>() )
                {
                    auto primaryPassChanged = false;
                    for( auto &technique : techniques )
                    {
                        if( technique )
                        {
                            auto passes = technique->getPasses();
                            if( !passes.empty() && passes.front() )
                            {
                                primaryPassChanged =
                                    state->getOwnerPtr() == passes.front().get();
                                break;
                            }
                        }
                    }

                    if( primaryPassChanged )
                    {
                        // This notification already carries the primary pass state. Reading the
                        // type from it avoids rescanning every pass and snapshotting the state
                        // context on the graphics thread.
                        auto materialPassState =
                            workphone::static_pointer_cast<MaterialPassStateData>( stateData );
                        if( m_appliedMaterialType.load() != materialPassState->materialType )
                        {
                            createMaterialByType();
                        }
                    }
                }
            }

            for( auto &technique : techniques )
            {
                if( technique )
                {
                    if( technique->handleStateChanged( state ) )
                    {
                        return true;
                    }
                }
            }

            if( state->getOwnerPtr() == this )
            {
                ScopedLock lock( this );

                auto stateData = state->getData();
                if( stateData && stateData->isDerived<MaterialStateData>() )
                {
                    const auto requestedMaterialType = getMaterialType();
                    const auto materialTypeWasOutOfSync =
                        m_appliedMaterialType.load() != requestedMaterialType;
                    if( materialTypeWasOutOfSync )
                    {
                        createMaterialByType();
                    }

                    const auto materialTypeApplied =
                        m_appliedMaterialType.load() == requestedMaterialType;
                    auto allTexturesApplied = applyPrimaryPassState( *this );
                    if( !materialTypeApplied || !allTexturesApplied )
                    {
                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->setDirty( true );
                        }
                    }

                    if( materialTypeWasOutOfSync && materialTypeApplied )
                    {
                        auto applicationManager = core::IApplicationManager::instancePtr();
                        if( applicationManager )
                        {
                            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                            auto factoryManager = applicationManager->getFactoryManagerPtr();
                            auto stateContext = getStateContext();
                            if( graphicsSystem && factoryManager && stateContext )
                            {
                                // Consumers such as Ogre Items cache the datablock pointer. The
                                // standard loaded notification makes them bind the newly selected
                                // backend datablock after a workflow change.
                                auto message = factoryManager->make_ptr<StateMessageLoad>();
                                message->setType( StateMessageLoad::LOADED_HASH );
                                message->setSender( this );
                                message->setObject( this );
                                stateContext->addMessage( graphicsSystem->getStateTask(), message );
                            }
                        }
                    }

                    return true;
                }
            }
        }

        return false;
    }

}  // namespace workphone::render
