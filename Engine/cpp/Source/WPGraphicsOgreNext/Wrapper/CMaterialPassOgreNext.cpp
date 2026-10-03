#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialPassOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsMeshOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTechniqueOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include "WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp"
#include <Workphone/Workphone.hpp>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>
#include <OgrePass.h>
#include <OgreTextureGpuManager.h>
#include <cmath>

namespace workphone::render
{
    namespace
    {
        Ogre::TextureAddressingMode getTextureAddressingMode( u32 wrapMode )
        {
            switch( wrapMode )
            {
            case 1u:
                return Ogre::TAM_CLAMP;
            case 2u:
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

            if( ogreNextTexture->getLoadingState() == LoadingState::Loading )
            {
                loadingPending = true;
                return nullptr;
            }

            if( ogreNextTexture->getLoadingState() != LoadingState::Loaded )
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

        bool applyPbsPassSettings( Ogre::HlmsPbsDatablock &datablock,
                                   const MaterialPassStateData &state )
        {
            const auto rotationRadians = state.uvRotation * Ogre::Math::PI / 180.0f;
            datablock.setUserValue(
                0u, Ogre::Vector4( state.uvOffsetX, state.uvOffsetY, state.uvTilingX - 1.0f,
                                   state.uvTilingY - 1.0f ) );
            datablock.setUserValue(
                1u, Ogre::Vector4( std::sin( rotationRadians ), std::cos( rotationRadians ) - 1.0f, 0.0f,
                                   0.0f ) );

            const auto projection = state.uvProjection <= 6u ? state.uvProjection : 0u;
            const auto projectionScale = std::isfinite( state.uvTriplanarScale )
                                             ? std::max( state.uvTriplanarScale, 0.0001f )
                                             : 1.0f;
            datablock.setUserValue( 2u, Ogre::Vector4( static_cast<Ogre::Real>( projection ),
                                                       projectionScale, 0.0f, 0.0f ) );

            const auto uvSet = projection == 0u && state.uvSet <= 3u ? state.uvSet : 0u;
            for( auto i = 0u; i < static_cast<u32>( Ogre::NUM_PBSM_SOURCES ); ++i )
            {
                datablock.setTextureUvSource( static_cast<Ogre::PbsTextureTypes>( i ),
                                              static_cast<Ogre::uint8>( uvSet ) );
            }

            const Ogre::Vector4 detailOffsetScale( state.detailOffsetX, state.detailOffsetY,
                                                   state.detailTilingX, state.detailTilingY );
            for( auto i = 0u; i < 4u; ++i )
            {
                datablock.setDetailMapOffsetScale( static_cast<Ogre::uint8>( i ), detailOffsetScale );
                datablock.setDetailNormalWeight( static_cast<Ogre::uint8>( i ),
                                                 state.detailNormalStrength );
            }
            datablock.setNormalMapWeight( state.normalStrength );

            auto allTexturesApplied = true;
            for( size_t i = 0; i < state.textures.size(); ++i )
            {
                const auto textureType = static_cast<Ogre::PbsTextureTypes>( i );
                const auto texture = state.textures[i];
                if( texture )
                {
                    auto loadingPending = false;
                    if( auto pTexture = getTextureGpu( texture, loadingPending ) )
                    {
                        Ogre::HlmsSamplerblock samplerblock;
                        samplerblock.setFiltering( getTextureFilterOptions( state.uvFilter ) );
                        samplerblock.mMaxAnisotropy =
                            Math<real_Num>::clamp( state.uvAniso, 1.0f, 16.0f );
                        if( textureType == Ogre::PBSM_REFLECTION )
                        {
                            samplerblock.mU = Ogre::TAM_CLAMP;
                            samplerblock.mV = Ogre::TAM_CLAMP;
                            samplerblock.mW = Ogre::TAM_CLAMP;
                        }
                        else
                        {
                            samplerblock.mU = getTextureAddressingMode( state.uvWrapU );
                            samplerblock.mV = getTextureAddressingMode( state.uvWrapV );
                            samplerblock.mW = samplerblock.mV;
                        }

                        datablock.setTexture( textureType, pTexture, &samplerblock );
                    }
                    else if( loadingPending )
                    {
                        allTexturesApplied = false;
                    }
                }
                else if( state.textureDirty[i] )
                {
                    datablock.setTexture( textureType, static_cast<Ogre::TextureGpu *>( nullptr ),
                                          nullptr );
                }
            }

            return allTexturesApplied;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialPassOgreNext, MaterialPass );

    CMaterialPassOgreNext::CMaterialPassOgreNext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();

        auto materialManager = graphicsSystem->getMaterialManagerPtr();
        WP_ASSERT( materialManager );

        auto stateContext = materialManager->getStateContext();
        WP_ASSERT( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<MaterialPassStateData>();
        state->setData( stateData );
    }

    CMaterialPassOgreNext::~CMaterialPassOgreNext()
    {
        unload( nullptr );
    }

    void CMaterialPassOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            createTextureSlots();
            setupMaterial();

            auto textureUnits = getTextureUnits();
            for( auto &tu : textureUnits )
            {
                if( tu )
                {
                    if( !tu->isLoaded() )
                    {
                        tu->load( data );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialPassOgreNext::createTextureSlots()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto parent = getParentPtr();
        WP_ASSERT( parent );

        auto textureUnits = getTextureUnits();

        auto technique = (CMaterialTechniqueOgreNext *)parent;
        if( technique )
        {
            auto owner = technique->getMaterial();
            auto material = workphone::static_pointer_cast<CMaterialOgreNext>( owner );
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

                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

                                texture->setMaterial( material );
                                texture->setName( textureTypeStr );
                            }
                        }

                        setTextureUnits( textures );
                    }
                }
                break;
                case MaterialType::StandardSpecular:
                {
                    auto numSlots = static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

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
                    auto numSlots = static_cast<size_t>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                                texture->setParent( this );

                                auto textureType = static_cast<PbsTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getPbsTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

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
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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
                case MaterialType::Skybox:
                {
                    auto numSlots = static_cast<size_t>( SkyboxTextureTypes::Count );
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                                texture->setParent( this );

                                auto textureType = static_cast<SkyboxTextureTypes>( i );
                                auto textureTypeStr = GraphicsUtil::getSkyboxTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

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
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                                texture->setParent( this );

                                auto textureType = static_cast<SkyboxCubeTextureTypes>( i );
                                auto textureTypeStr =
                                    GraphicsUtil::getSkyboxCubeTextureType( textureType );

                                texture->setTextureType( static_cast<u32>( i ) );

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
                    if( numSlots != textureUnits.size() )
                    {
                        auto textures = Array<SmartPtr<IMaterialTexture>>();
                        textures.resize( numSlots );

                        for( size_t i = 0; i < textures.size(); ++i )
                        {
                            auto &texture = textures[i];
                            if( !texture )
                            {
                                texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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

    void CMaterialPassOgreNext::reload( SmartPtr<ISharedObject> data )
    {
    }

    void CMaterialPassOgreNext::setupMaterial()
    {
        try
        {
            SafeReadPtr<MaterialPassStateData> passState;
            if( auto stateContext = getStateContextPtr() )
            {
                passState = stateContext->getStateDataById<MaterialPassStateData>( getId() );
                if( passState )
                {
                    applyCustomShaderPrograms( m_pass, *passState );
                }
            }

            auto parent = getParent();
            auto technique = workphone::static_pointer_cast<CMaterialTechniqueOgreNext>( parent );
            if( technique )
            {
                auto owner = technique->getMaterial();
                auto material = workphone::static_pointer_cast<CMaterialOgreNext>( owner );
                if( material )
                {
                    auto datablock = material->getHlmsDatablock();
                    if( datablock )
                    {
                        auto materialType = MaterialType::Standard;
                        auto workflow = 0u;
                        if( passState )
                        {
                            materialType = passState->materialType;
                            workflow = passState->workflow;
                        }
                        switch( materialType )
                        {
                        case MaterialType::Standard:
                        case MaterialType::StandardSpecular:
                        case MaterialType::StandardTriPlanar:
                        {
                            auto pDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( datablock );
                            if( !pDatablock )
                            {
                                WP_LOG_ERROR(
                                    "CMaterialPassOgreNext::setupMaterial: expected a PBS datablock." );
                                break;
                            }

                            auto d = getDiffuse();
                            auto specular = getSpecular();
                            auto e = getEmissive();

                            auto metalness = Math<real_Num>::clamp( getMetalness(), 0.0, 1.0 );
                            auto roughness = Math<real_Num>::clamp( getRoughness(), 0.001, 1.0 );

                            pDatablock->setDiffuse( Ogre::Vector3( d.r, d.g, d.b ) );
                            pDatablock->setEmissive( Ogre::Vector3( e.r, e.g, e.b ) );

                            const auto useSpecularWorkflow =
                                materialType == MaterialType::StandardSpecular || workflow == 1u;
                            if( useSpecularWorkflow )
                            {
                                pDatablock->setWorkflow(
                                    Ogre::HlmsPbsDatablock::Workflows::SpecularWorkflow );
                                pDatablock->setSpecular(
                                    Ogre::Vector3( specular.r, specular.g, specular.b ) );
                            }
                            else
                            {
                                pDatablock->setWorkflow(
                                    Ogre::HlmsPbsDatablock::Workflows::MetallicWorkflow );
                                pDatablock->setMetalness( metalness );
                            }

                            pDatablock->setRoughness( roughness );

                            if( passState )
                            {
                                const auto cutout = BitUtil::getFlagValue(
                                    passState->flags, static_cast<u32>( render::cutoutFlag ) );
                            const auto transparent = BitUtil::getFlagValue(
                                    passState->flags, static_cast<u32>( render::transparentFlag ) );
                            const auto isEmissive = BitUtil::getFlagValue(
                                    passState->flags,
                                    static_cast<u32>( render::emissionEnabledFlag ) );
                                const auto opacity =
                                    Math<real_Num>::clamp( d.a, 0.0f, 1.0f );

                            if( isEmissive )
                            {
                                pDatablock->setEmissive( Ogre::Vector3( e.r, e.g, e.b ) );
                            }
                            else
                            {
                                pDatablock->setEmissive( Ogre::Vector3::ZERO );
                            }

                            if( transparent )
                            {
                                    pDatablock->setTransparency( opacity,
                                                                 Ogre::HlmsPbsDatablock::Fade );

                                    auto macroblock =
                                        Ogre::HlmsMacroblock( *pDatablock->getMacroblock() );
                                macroblock.mDepthCheck = true;
                                macroblock.mDepthWrite = false;
                                pDatablock->setMacroblock( macroblock );

                                // Preserve the existing alpha-fade path. A cutout pass still
                                // uses the same backend mode until alpha-test support is wired.
                                (void)cutout;
                            }
                                else if( pDatablock->getTransparencyMode() !=
                                         Ogre::HlmsPbsDatablock::None )
                            {
                                    pDatablock->setTransparency(
                                        1.0f, Ogre::HlmsPbsDatablock::None );

                                Ogre::HlmsMacroblock macroblock;
                                Ogre::HlmsBlendblock blendblock;
                                pDatablock->setMacroblock( macroblock );
                                pDatablock->setBlendblock( blendblock );
                            }

                                applyCullingMode( pDatablock, passState->cullMode );
                                if( !applyPbsPassSettings( *pDatablock, *passState ) )
                                {
                                    if( auto stateContext = getStateContextPtr() )
                                    {
                                        stateContext->setDirty( true );
                                    }
                                }
                            }
                            else
                            {
                                applyCullingMode( pDatablock, getCullingMode() );
                            }

                            /*
                            auto count = 0;
                            auto textures = *material->getTextures();
                            for( const auto &texture : textures )
                            {
                                auto ogreNextTexture =
                                    fb::dynamic_pointer_cast<CTextureOgreNext>( texture );

                                if( ogreNextTexture )
                                {
                                    try
                                    {
                                        auto pTexture = ogreNextTexture->getTexture();
                                        if( pTexture )
                                        {
                                            pDatablock->setTexture( (Ogre::PbsTextureTypes)count,
                                                                    pTexture );
                                        }
                                    }
                                    catch( Ogre::Exception &e )
                                    {
                                        WP_LOG_EXCEPTION( e );
                                    }
                                    catch( std::exception &e )
                                    {
                                        WP_LOG_EXCEPTION( e );
                                    }
                                }

                                count++;
                            }
                            */
                        }
                        break;
                        case MaterialType::UI:
                        {
                            auto pDatablock = dynamic_cast<Ogre::HlmsUnlitDatablock *>( datablock );
                            if( !pDatablock )
                            {
                                WP_LOG_ERROR(
                                    "CMaterialPassOgreNext::setupMaterial: expected an Unlit "
                                    "datablock." );
                                break;
                            }

                            // auto d = getDiffuse();
                            // auto e = getEmmissive();

                            // auto metalness = getMetalness();
                            // auto roughness = getRoughness();

                            // pbsDatablock->setDiffuse(Ogre::Vector3(d.r, d.g, d.b));
                            // pbsDatablock->setEmissive(Ogre::Vector3(e.r, e.g, e.b)); // todo hack
                            // for testing

                            // pbsDatablock->setWorkflow(Ogre::HlmsPbsDatablock::Workflows::MetallicWorkflow);
                            // pbsDatablock->setMetalness(metalness);
                            // pbsDatablock->setRoughness(roughness);

                            auto textures = getTextureUnits();
                            for( const auto &texture : textures )
                            {
                                if( !texture )
                                {
                                    continue;
                                }

                                auto textureName = texture->getTextureName();
                                if( !StringUtil::isNullOrEmpty( textureName ) )
                                {
                                    try
                                    {
                                        using namespace Ogre;

                                        auto texUnit = 0;
                                        auto mCreator = pDatablock->getCreator();
                                        HlmsSamplerblock *refParams = nullptr;

                                        TextureGpuManager *textureManager =
                                            mCreator->getRenderSystem()->getTextureGpuManager();
                                        TextureGpu *texture;
                                        if( !textureName.empty() )
                                        {
                                            texture = textureManager->createOrRetrieveTexture(
                                                textureName.c_str(), GpuPageOutStrategy::Discard,
                                                TextureFlags::AutomaticBatching |
                                                    TextureFlags::PrefersLoadingFromFileAsSRGB,
                                                TextureTypes::Type2D,
                                                ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
                                        }

                                        pDatablock->setTexture( texUnit, texture, refParams );
                                    }
                                    catch( Ogre::Exception &e )
                                    {
                                        WP_LOG_EXCEPTION( e );
                                    }
                                    catch( std::exception &e )
                                    {
                                        WP_LOG_EXCEPTION( e );
                                    }
                                }
                            }
                        }
                        break;
                        default:
                        {
                        }
                        break;
                        }
                    }
                    else
                    {
                        auto name = material->getName();
                        WP_LOG_ERROR( "Datablock null: " + name );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialPassOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto textureUnits = getTextureUnits();
                for( auto &textureUnit : textureUnits )
                {
                    textureUnit->unload( data );
                }

                MaterialPass::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialPassOgreNext::initialise( Ogre::Pass *pass )
    {
        m_pass = pass;

        if( m_pass )
        {
            MaterialPass::setVertexShaderName( m_pass->getVertexProgramName() );
            MaterialPass::setFragmentShaderName( m_pass->getFragmentProgramName() );
            MaterialPass::setGeometryShaderName( m_pass->getGeometryProgramName() );
        }
    }

    auto CMaterialPassOgreNext::createTextureUnit() -> SmartPtr<IMaterialTexture>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        return factoryManager->make_ptr<CMaterialTextureOgreNext>();
    }

    void CMaterialPassOgreNext::setTexture( SmartPtr<ITexture> tex, u32 layerIdx /*= 0 */ )
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

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto factoryManager = applicationManager->getFactoryManager();

            auto textureUnits = getTextureUnits();

            if( layerIdx >= textureUnits.size() )
            {
                auto textures = Array<SmartPtr<IMaterialTexture>>();
                textures.resize( layerIdx + 1 );

                WP_ASSERT( textureUnits.size() < textures.size() );

                for( size_t i = 0; i < textureUnits.size(); ++i )
                {
                    auto &texture = textureUnits[i];
                    if( texture )
                    {
                        textures[i] = texture;
                    }
                }

                for( auto &texture : textures )
                {
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                    }
                }

                auto &texture = textures[layerIdx];
                if( !texture )
                {
                    texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                }

                if( texture )
                {
                    texture->setParent( this );
                    texture->setTexture( tex );

                    graphicsSystem->loadObject( texture );
                }

                setTextureUnits( textures );
            }
            else
            {
                WP_ASSERT( layerIdx < textureUnits.size() );

                if( layerIdx < textureUnits.size() )
                {
                    auto &texture = textureUnits[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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

    void CMaterialPassOgreNext::setTexture( const String &fileName, u32 layerIdx /*= 0 */ )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();

            auto textureUnits = getTextureUnits();

            if( layerIdx >= textureUnits.size() )
            {
                auto textures = Array<SmartPtr<IMaterialTexture>>();
                textures.resize( layerIdx + 1 );

                WP_ASSERT( textureUnits.size() < textures.size() );

                for( size_t i = 0; i < textureUnits.size(); ++i )
                {
                    auto &texture = textureUnits[i];
                    if( texture )
                    {
                        textures[i] = texture;
                    }
                }

                for( auto &texture : textures )
                {
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                    }
                }

                auto &texture = textures[layerIdx];
                if( !texture )
                {
                    texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
                }

                if( texture )
                {
                    texture->setParent( this );
                    texture->setTextureName( fileName );
                }

                setTextureUnits( textures );
            }
            else
            {
                WP_ASSERT( layerIdx < textureUnits.size() );

                if( layerIdx < textureUnits.size() )
                {
                    auto &texture = textureUnits[layerIdx];
                    if( !texture )
                    {
                        texture = factoryManager->make_ptr<CMaterialTextureOgreNext>();
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

    auto CMaterialPassOgreNext::getVertexShaderName() const -> String
    {
        if( m_pass && !m_pass->getVertexProgramName().empty() )
        {
            return m_pass->getVertexProgramName();
        }

        return MaterialPass::getVertexShaderName();
    }

    void CMaterialPassOgreNext::setVertexShaderName( const String &name )
    {
        MaterialPass::setVertexShaderName( name );

        if( m_pass )
        {
            m_pass->setVertexProgram( name );
        }
    }

    auto CMaterialPassOgreNext::getFragmentShaderName() const -> String
    {
        if( m_pass && !m_pass->getFragmentProgramName().empty() )
        {
            return m_pass->getFragmentProgramName();
        }

        return MaterialPass::getFragmentShaderName();
    }

    void CMaterialPassOgreNext::setFragmentShaderName( const String &name )
    {
        MaterialPass::setFragmentShaderName( name );

        if( m_pass )
        {
            m_pass->setFragmentProgram( name );
        }
    }

    auto CMaterialPassOgreNext::getGeometryShaderName() const -> String
    {
        if( m_pass && !m_pass->getGeometryProgramName().empty() )
        {
            return m_pass->getGeometryProgramName();
        }

        return MaterialPass::getGeometryShaderName();
    }

    void CMaterialPassOgreNext::setGeometryShaderName( const String &name )
    {
        MaterialPass::setGeometryShaderName( name );

        if( m_pass )
        {
            m_pass->setGeometryProgram( name );
        }
    }

    auto CMaterialPassOgreNext::getPass() const -> Ogre::Pass *
    {
        return m_pass;
    }

    void CMaterialPassOgreNext::setPass( Ogre::Pass *pass )
    {
        m_pass = pass;
    }

    auto CMaterialPassOgreNext::getDataBlock() const -> Ogre::HlmsDatablock *
    {
        auto pMaterial = getMaterial();
        auto material = workphone::static_pointer_cast<CMaterialOgreNext>( pMaterial );
        return material->getHlmsDatablock();
    }

    bool CMaterialPassOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto textureUnits = getTextureUnits();
        for( auto &textureUnit : textureUnits )
        {
            textureUnit->handleStateMessage( message );
        }

        if( message->isExactly<StateMessageVector4>() )
        {
            auto vectorMessage = workphone::static_pointer_cast<StateMessageVector4>( message );
            auto value = vectorMessage->getValue();
            auto type = vectorMessage->getType();

            if( type == DIFFUSE_HASH )
            {
                setDiffuse( ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                return true;
            }
            else if( type == EMISSION_HASH )
            {
                setEmissive( ColourF( value.X(), value.Y(), value.Z(), value.W() ) );
                return true;
            }
        }

        return false;
    }

    bool CMaterialPassOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !state )
        {
            return false;
        }

        auto owner = state->getOwnerPtr();

        if( owner && owner->isDerived<IMaterialTexture>() )
        {
            auto textureUnits = getTextureUnits();
            for( auto &textureUnit : textureUnits )
            {
                if( textureUnit && textureUnit->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        if( isLoaded() )
        {
            if( owner == this )
            {
                if( auto data = state->getData() )
                {
                    if( data->isExactly<MaterialPassStateData>() )
                    {
                        ScopedLock lock( this );

                        auto pMaterial = getMaterial();
                        auto material = workphone::static_pointer_cast<CMaterialOgreNext>( pMaterial );
                        if( !material )
                        {
                            WP_LOG_ERROR(
                                "CMaterialPassOgreNext::handleStateChanged: material is null." );
                            return false;
                        }

                        setupMaterial();

                        auto applicationManager = core::IApplicationManager::instancePtr();
                        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                        auto graphicsScenes = graphicsSystem->getSceneManagers();
                        for( auto &graphicsScene : graphicsScenes )
                        {
                            auto graphicsObjects = graphicsScene->getGraphicsObjects();
                            for( auto &graphicsObject : graphicsObjects )
                            {
                                if( graphicsObject->isDerived<CGraphicsMeshOgreNext>() )
                                {
                                    auto graphicsMesh =
                                        workphone::static_pointer_cast<CGraphicsMeshOgreNext>(
                                            graphicsObject );
                                    graphicsMesh->materialLoaded( material );
                                }
                            }
                        }

                        return true;
                    }
                }
            }
        }

        return false;
    }

    void CMaterialPassOgreNext::applyCullingMode( Ogre::HlmsPbsDatablock *datablock, u32 mode )
    {
        if( !datablock )
        {
            return;
        }

        const auto cullingMode = toOgreCullingMode( mode );
        const auto twoSided = cullingMode == Ogre::CULL_NONE;
        if( datablock->getTwoSidedLighting() != twoSided )
        {
            datablock->setTwoSidedLighting( twoSided, false );
        }

        auto macroblock = Ogre::HlmsMacroblock( *datablock->getMacroblock() );
        if( macroblock.mCullMode != cullingMode )
        {
            macroblock.mCullMode = cullingMode;
            datablock->setMacroblock( macroblock );
        }
    }

    Ogre::CullingMode CMaterialPassOgreNext::toOgreCullingMode( u32 mode )
    {
        switch( mode )
        {
        case Ogre::CULL_NONE:
            return Ogre::CULL_NONE;
        case Ogre::CULL_ANTICLOCKWISE:
            return Ogre::CULL_ANTICLOCKWISE;
        case Ogre::CULL_CLOCKWISE:
        default:
            return Ogre::CULL_CLOCKWISE;
        }
    }

    void CMaterialPassOgreNext::applyCustomShaderPrograms( Ogre::Pass *pass,
                                                           const MaterialPassStateData &state )
    {
        if( !pass )
        {
            return;
        }

        pass->setVertexProgram( state.vertexShaderName );
        pass->setFragmentProgram( state.fragmentShaderName );
        pass->setGeometryProgram( state.geometryShaderName );
    }

}  // namespace workphone::render
