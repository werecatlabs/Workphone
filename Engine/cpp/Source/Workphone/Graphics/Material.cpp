#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Database/AssetDocumentSave.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/State/Messages/StateMessageSetTexture.hpp>
#include <Workphone/State/Messages/StateMessagePair.hpp>
#include <Workphone/State/Messages/StateMessageLoad.hpp>
#include <Workphone/State/Messages/StateMessageObjectsArray.hpp>
#include <Workphone/State/Messages/StateMessageFragmentParam.hpp>
#include <Workphone/State/Messages/StateMessageIntValue.hpp>
#include <Workphone/State/Messages/StateMessageUIntValue.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/MaterialStateData.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Graphics/GraphicsUtil.hpp>
#include <functional>
#include <filesystem>
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone::render
{

    namespace
    {
        struct SinglePassEdit
        {
            SmartPtr<IMaterialTechnique> technique;
            SmartPtr<IMaterialPass> pass;
        };

        SinglePassEdit singlePassEdit( const IMaterial &material,
                                      const std::atomic<u64> &graphRevision )
        {
            const auto revision = graphRevision.load( std::memory_order_acquire );
            if( material.getNumTechniques() != 1 ) return {};
            auto technique = material.getTechnique( 0 );
            if( !technique || technique->getNumPasses() != 1 ) return {};
            auto pass = technique->getPass( 0 );
            if( graphRevision.load( std::memory_order_acquire ) != revision ) return {};
            // Pin both owners, then invoke the virtual setter outside container
            // locks. A callback may replace/detach the selected graph.
            return { std::move( technique ), std::move( pass ) };
        }
    }

    const String Material::materialTypeStr = String( "Material Type" );
    const String Material::materialTypeDataStr = String( "materialType" );
    const String Material::shaderIDStr = String( "Shader ID" );
    const String Material::shaderParamPrefixStr = String( "ShaderParam." );
    const String Material::shaderTexturePrefixStr = String( "ShaderTex." );
    const String Material::editorSettingsStr = String( "editorSettings" );
    const String Material::editorFloatsStr = String( "floats" );
    const String Material::editorUIntsStr = String( "uints" );
    const String Material::editorBoolsStr = String( "bools" );
    const String Material::editorStringsStr = String( "strings" );
    const String Material::schemesStr = String( "schemes" );
    const String Material::texturesStr = String( "textures" );
    const String Material::textureStr = String( "texture" );
    const String Material::uuidStr = String( "uuid" );
    const String Material::enumStr = String( "enum" );
    const String Material::defaultPBRShaderIDStr = String( "DefaultPBR" );
    const String Material::unlitShaderIDStr = String( "Unlit" );
    const String Material::colorOnlyShaderIDStr = String( "ColorOnly" );
    const String Material::shaderAlbedoParamStr = String( "Albedo" );
    const String Material::shaderMetalnessParamStr = String( "Metalness" );
    const String Material::shaderRoughnessParamStr = String( "Roughness" );
    const String Material::shaderEmissiveParamStr = String( "Emissive" );
    const String Material::shaderOpacityParamStr = String( "Opacity" );
    const String Material::shaderAlphaClipParamStr = String( "AlphaClip" );
    const String Material::shaderColorParamStr = String( "Color" );
    const String Material::shaderParamVectorXStr = String( ".x" );
    const String Material::shaderParamVectorYStr = String( ".y" );
    const String Material::shaderParamVectorZStr = String( ".z" );
    const String Material::shaderParamVectorWStr = String( ".w" );
    const String Material::shaderParamMatrixSuffixStr[16] = {
        String( ".m0" ),  String( ".m1" ),  String( ".m2" ),  String( ".m3" ),
        String( ".m4" ),  String( ".m5" ),  String( ".m6" ),  String( ".m7" ),
        String( ".m8" ),  String( ".m9" ),  String( ".m10" ), String( ".m11" ),
        String( ".m12" ), String( ".m13" ), String( ".m14" ), String( ".m15" )
    };
    const String Material::renderModeStr = String( "renderMode" );
    const String Material::workflowStr = String( "workflow" );
    const String Material::specularStr = String( "specular" );
    const String Material::normalStrengthStr = String( "normalStrength" );
    const String Material::detailNormalStrengthStr = String( "detailNormalStrength" );
    const String Material::alphaClipStr = String( "alphaClip" );
    const String Material::opacityStr = String( "opacity" );
    const String Material::blendModeStr = String( "blendMode" );
    const String Material::depthWriteStr = String( "depthWrite" );
    const String Material::depthTestStr = String( "depthTest" );
    const String Material::cullModeStr = String( "cullMode" );
    const String Material::doubleSidedStr = String( "doubleSided" );
    const String Material::uvTilingXStr = String( "uvTilingX" );
    const String Material::uvTilingYStr = String( "uvTilingY" );
    const String Material::uvOffsetXStr = String( "uvOffsetX" );
    const String Material::uvOffsetYStr = String( "uvOffsetY" );
    const String Material::uvRotationStr = String( "uvRotation" );
    const String Material::uvProjectionStr = String( "uvProjection" );
    const String Material::uvSetStr = String( "uvSet" );
    const String Material::uvTriplanarScaleStr = String( "uvTriplanarScale" );

    WP_CLASS_REGISTER_DERIVED( workphone::render, Material, ResourceGraphics<IMaterial> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, Material::MaterialStateListener, IStateListener );

    u32 Material::m_idExt = 0;

    namespace
    {
        template <class Setting>
        struct MaterialSettingAccess;

        template <>
        struct MaterialSettingAccess<MaterialFloatSetting>
        {
            using Value = f32;
            static constexpr auto read = &MaterialPassStateData::getFloatSetting;
            static constexpr auto write = &MaterialPassStateData::setFloatSetting;
        };

        template <>
        struct MaterialSettingAccess<MaterialUIntSetting>
        {
            using Value = u32;
            static constexpr auto read = &MaterialPassStateData::getUIntSetting;
            static constexpr auto write = &MaterialPassStateData::setUIntSetting;
        };

        template <>
        struct MaterialSettingAccess<MaterialBoolSetting>
        {
            using Value = bool;
            static constexpr auto read = &MaterialPassStateData::getBoolSetting;
            static constexpr auto write = &MaterialPassStateData::setBoolSetting;
        };

        // Built-in controls already know their numeric setting. Named authoring
        // adapters resolve once and share these paths; no per-material lookup map.
        template <class Setting>
        bool setNumericSetting( Material &material, Setting setting,
                                typename MaterialSettingAccess<Setting>::Value value )
        {
            bool handled = false;
            for( auto technique : material.getTechniques() )
            {
                if( technique )
                {
                    for( auto pass : technique->getPasses() )
                    {
                        if( auto context = pass ? pass->getStateContext() : nullptr )
                        {
                            if( auto data = context->invalidateStateDataById<MaterialPassStateData>(
                                    pass->getId() ) )
                            {
                                handled = ( ( *data ).*MaterialSettingAccess<Setting>::write )(
                                              setting, value ) ||
                                          handled;
                            }
                        }
                    }
                }
            }
            return handled;
        }

        template <class Setting>
        auto getNumericSetting( const Material &material, Setting setting,
                                typename MaterialSettingAccess<Setting>::Value fallback ) ->
            typename MaterialSettingAccess<Setting>::Value
        {
            // Available authored passes work even before a technique is selected.
            for( auto technique : material.getTechniques() )
            {
                if( technique )
                {
                    for( auto pass : technique->getPasses() )
                    {
                        if( auto context = pass ? pass->getStateContext() : nullptr )
                        {
                            if( auto data =
                                    context->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                            {
                                auto value = fallback;
                                if( ( ( *data ).*MaterialSettingAccess<Setting>::read )( setting,
                                                                                         value ) )
                                {
                                    return value;
                                }
                            }
                        }
                    }
                }
            }
            return fallback;
        }

        struct MaterialParameterSyncScope
        {
            bool &flag;
            bool previous;

            explicit MaterialParameterSyncScope( bool &value ) : flag( value ), previous( value )
            {
                flag = true;
            }

            ~MaterialParameterSyncScope()
            {
                flag = previous;
            }
        };

        // The seven layout semantics occupy bits 0..6. Bit 7 records an
        // alpha-only edit of Albedo in a layout without scalar Opacity.
        constexpr u8 shaderAlphaOnly = 0x80u;
        static_assert( static_cast<u32>( MaterialBuiltinParameter::Count ) == 7u,
                       "Extend the material edit mask before adding built-in semantics" );

        constexpr u8 legacySurfaceMask( MaterialBuiltinParameter semantic )
        {
            return semantic == MaterialBuiltinParameter::Opacity ||
                           semantic == MaterialBuiltinParameter::Metalness ||
                           semantic == MaterialBuiltinParameter::Roughness
                       ? static_cast<u8>( 1u << static_cast<u32>( semantic ) )
                       : 0;
        }

        bool shaderOwnsSurfaceScalar( const MaterialShaderParameterLayout::Ptr &layout, bool active,
                                      bool pbr, MaterialBuiltinParameter semantic )
        {
            if( !active || !layout || ( semantic != MaterialBuiltinParameter::Opacity && !pbr ) )
            {
                return false;
            }
            const auto hasType = [&]( MaterialBuiltinParameter parameter,
                                      MaterialShaderParameterType type ) {
                const auto handle = layout->GetBuiltin( parameter );
                return handle.IsValid() &&
                       layout->GetParameterInfo()[handle.m_parameterIndex].m_type == type;
            };
            return hasType( semantic, MaterialShaderParameterType::Scalar ) ||
                   ( semantic == MaterialBuiltinParameter::Opacity && pbr &&
                     hasType( MaterialBuiltinParameter::Albedo, MaterialShaderParameterType::Colour ) );
        }

        bool readShaderSurfaceScalar( const MaterialShaderParametersInstance &parameters, bool pbr,
                                      MaterialBuiltinParameter semantic, f32 &value )
        {
            if( semantic != MaterialBuiltinParameter::Opacity && !pbr )
            {
                return false;
            }
            const auto handle = parameters.GetBuiltin( semantic );
            if( parameters.IsHandleValid( handle, MaterialShaderParameterType::Scalar, sizeof( f32 ) ) )
            {
                value = parameters.GetScalar( handle );
            }
            else if( semantic == MaterialBuiltinParameter::Opacity && pbr )
            {
                const auto albedo = parameters.GetBuiltin( MaterialBuiltinParameter::Albedo );
                if( !parameters.IsHandleValid( albedo, MaterialShaderParameterType::Colour,
                                               sizeof( ColourF ) ) )
                {
                    return false;
                }
                value = parameters.GetColour( albedo ).a;
            }
            else
            {
                return false;
            }
            if( semantic == MaterialBuiltinParameter::Opacity )
            {
                value = Math<f32>::clamp( value, 0.0f, 1.0f );
            }
            return true;
        }

        u8 compatibleShaderEditMask( const MaterialShaderParametersInstance &parameters, bool active,
                                     bool pbr, bool colorOnly, u8 pending )
        {
            if( !active || !pending )
            {
                return 0;
            }
            u8 compatible = 0;
            const auto include = [&]( MaterialBuiltinParameter semantic,
                                      MaterialShaderParameterType type, u32 size ) {
                if( parameters.IsHandleValid( parameters.GetBuiltin( semantic ), type, size ) )
                {
                    compatible |= static_cast<u8>( 1u << static_cast<u32>( semantic ) );
                }
            };
            if( pbr )
            {
                include( MaterialBuiltinParameter::Albedo, MaterialShaderParameterType::Colour,
                         sizeof( ColourF ) );
                include( MaterialBuiltinParameter::Metalness, MaterialShaderParameterType::Scalar,
                         sizeof( f32 ) );
                include( MaterialBuiltinParameter::Roughness, MaterialShaderParameterType::Scalar,
                         sizeof( f32 ) );
                include( MaterialBuiltinParameter::AlphaClip, MaterialShaderParameterType::Scalar,
                         sizeof( f32 ) );
            }
            include( colorOnly ? MaterialBuiltinParameter::Color : MaterialBuiltinParameter::Emissive,
                     MaterialShaderParameterType::Colour, sizeof( ColourF ) );
            include( MaterialBuiltinParameter::Opacity, MaterialShaderParameterType::Scalar,
                     sizeof( f32 ) );
            auto retained = static_cast<u8>( pending & compatible );
            const auto opacityBit =
                static_cast<u8>( 1u << static_cast<u32>( MaterialBuiltinParameter::Opacity ) );
            const auto albedoBit =
                static_cast<u8>( 1u << static_cast<u32>( MaterialBuiltinParameter::Albedo ) );
            if( pending & ( opacityBit | shaderAlphaOnly ) )
            {
                if( compatible & opacityBit )
                {
                    retained |= opacityBit;
                }
                else if( ( compatible & albedoBit ) && !( retained & albedoBit ) )
                {
                    retained |= shaderAlphaOnly;
                }
            }
            return retained;
        }

        void applyCullModeToPasses( IMaterial *material, u32 mode )
        {
            if( !material )
            {
                return;
            }

            for( auto technique : material->getTechniques() )
            {
                for( auto pass : technique->getPasses() )
                {
                    pass->setCullingMode( mode );
                }
            }
        }

        bool tryParseMaterialType( const String &value, MaterialType &materialType )
        {
            const auto trimmedValue = StringUtil::trim( value );
            if( StringUtil::isNullOrEmpty( trimmedValue ) )
            {
                return false;
            }

            auto isInteger = true;
            auto characterIndex = size_t( 0 );
            if( trimmedValue[0] == '+' || trimmedValue[0] == '-' )
            {
                characterIndex = 1;
            }

            if( characterIndex == trimmedValue.size() )
            {
                isInteger = false;
            }

            for( ; isInteger && characterIndex < trimmedValue.size(); ++characterIndex )
            {
                const auto character = trimmedValue[characterIndex];
                isInteger = character >= '0' && character <= '9';
            }

            if( isInteger )
            {
                if( trimmedValue[0] == '-' )
                {
                    return false;
                }

                auto numericValue = u32( 0 );
                auto digitIndex = trimmedValue[0] == '+' ? size_t( 1 ) : size_t( 0 );
                for( ; digitIndex < trimmedValue.size(); ++digitIndex )
                {
                    numericValue =
                        numericValue * 10u + static_cast<u32>( trimmedValue[digitIndex] - '0' );
                    if( numericValue >= static_cast<u32>( MaterialType::Count ) )
                    {
                        return false;
                    }
                }

                if( numericValue < static_cast<u32>( MaterialType::Count ) )
                {
                    materialType = static_cast<MaterialType>( numericValue );
                    return true;
                }

                return false;
            }

            const auto materialTypes = GraphicsUtil::getMaterialTypes();
            for( size_t i = 0; i < materialTypes.size(); ++i )
            {
                if( StringUtil::isEqual( materialTypes[i], trimmedValue, true ) )
                {
                    materialType = static_cast<MaterialType>( i );
                    return true;
                }
            }

            return false;
        }
    }  // namespace

    Material::Material()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        // Scene material components subscribe to resource completion. Loading
        // notifications are opt-in independently of ordinary object events.
        setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        static const auto MaterialStr = String( "Material" );
        auto name = MaterialStr + StringUtil::toString( m_idExt++ );
        auto id = StringUtil::getHash( name );

        setName( name );
        setId( id );
        m_shaderParameters.m_owner = this;
    }

    Material::~Material()
    {
        unload( nullptr );
    }

    SmartPtr<IStateContext> Material::getStateContext() const
    {
        return m_materialStateContext;
    }

    IStateContext *Material::getStateContextPtr() const
    {
        return m_materialStateContext.load().get();
    }

    void Material::createStateObject()
    {
        if( getStateContext() )
        {
            return;
        }
        auto pThis = getSharedFromThis<ISharedObject>();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto graphicsFactoryManager = graphicsSystem->getFactoryManager();
        WP_ASSERT( graphicsFactoryManager );

        auto stateContext = stateManager->addStateContext();
        m_materialStateContext = stateContext;
        stateContext->setOwner( pThis.get() );

        auto stateListener = graphicsFactoryManager->make_object<Material::MaterialStateListener>();
        if( !stateListener )
        {
            // WPGraphics does not register a backend-specific material listener.
            stateListener = workphone::make_ptr<Material::MaterialStateListener>();
        }
        stateListener->setOwner( pThis );
        stateContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<MaterialStateData>();
        state->setData( stateData );

        auto renderTask = graphicsSystem->getStateTask();
        stateContext->setTaskId( renderTask );
    }

    void Material::saveToFile( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto data = toData();
        auto properties = workphone::static_pointer_cast<Properties>( data );
        auto materialStr = DataUtil::toString( properties.get(), true );

        String error;
        if( !saveAssetDocumentText( filePath, properties, materialStr, error ) )
        {
            throw std::runtime_error( error.c_str() );
        }
    }

    void Material::loadFromFile( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto stream = fileSystem->open( filePath, true, false, false, false, false );
        if( !stream )
        {
            stream = fileSystem->open( filePath, true, false, false, true, true );
        }

        if( stream )
        {
            auto materialStr = stream->getAsString();

            auto materialData = workphone::make_ptr<Properties>();
            DataUtil::parse( materialStr, materialData.get() );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( filePath, fileInfo ) )
            {
                auto fileId = fileInfo.fileId;
                setFileSystemId( fileId );
            }

            setFilePath( filePath );

            fromData( materialData );
        }
    }

    void Material::save()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto data = toData();
            auto materialData = workphone::static_pointer_cast<Properties>( data );
            WP_ASSERT( materialData );

            auto dataStr = DataUtil::toString( materialData.get(), true );
            WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );

            auto fileId = getFileSystemId();
            auto wroteFile = false;
            if( !fileId.is_nil() )
            {
                FileInfo fileInfo;
                if( fileSystem->findFileInfo( fileId, fileInfo ) )
                {
                    auto filePath = String( fileInfo.filePath.c_str() );
                    String error;
                    if( !saveAssetDocumentText( filePath, materialData, dataStr, error ) )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                    wroteFile = true;
                }
                else
                {
                    WP_LOG_ERROR( "Could not save material: " + String( fileInfo.filePath.c_str() ) );
                }
            }

            if( !wroteFile )
            {
                auto filePath = getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    String error;
                    if( !saveAssetDocumentText( filePath, materialData, dataStr, error ) )
                    {
                        throw std::runtime_error( error.c_str() );
                    }

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        setFileSystemId( fileInfo.fileId );
                    }
                }
                else
                {
                    WP_LOG_ERROR( "Could not save material: missing file path." );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::load( SmartPtr<ISharedObject> data )
    {
        // Keep the root lifecycle context for standalone materials too. Surface
        // accessors use pass state; the root MaterialStateData is a type marker.
        if( !getStateContext() )
        {
            createStateObject();
        }

        auto filePath = getFilePath();
        if( !StringUtil::isNullOrEmpty( filePath ) )
        {
            loadFromFile( filePath );
        }
        shaderParametersChanged( 0 );  // Publish edits made before the first pass existed.
    }

    void Material::reload( SmartPtr<ISharedObject> data )
    {
        unload( data );
        load( data );
    }

    void Material::unload( SmartPtr<ISharedObject> data )
    {
        invalidateRenderState();
        try
        {
            auto cachedTechnique=m_technique.load();auto cachedPass=m_pass.load();
            m_technique=nullptr;setPass(nullptr);
            if(cachedTechnique){auto owner=cachedTechnique->getMaterial();if(!owner||owner.get()==this)cachedTechnique->unload(data);}
            if(cachedPass){auto owner=cachedPass->getMaterial();if(!owner||owner.get()==this)cachedPass->unload(data);}
            for(auto technique:m_techniques)if(technique) {
                auto owner=technique->getMaterial();if(!owner||owner.get()==this)technique->unload(data);
            }
            m_techniques.releaseStorage();
            if( auto context = getStateContext() )
            {
                for( auto listener : context->getStateListeners() )
                {
                    if( auto materialListener = dynamic_pointer_cast<MaterialStateListener>( listener ) )
                    {
                        materialListener->setOwner( nullptr );
                    }
                }
                context->setOwner( nullptr );
                if( auto app = core::IApplicationManager::instancePtr() )
                {
                    if( auto manager = app->getStateManager() )
                    {
                        manager->removeStateContext( context );
                    }
                }
                context->unload( nullptr );
                m_materialStateContext = nullptr;
            }
            const auto &state = getLoadingState();
            if( state != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                ResourceGraphics<IMaterial>::unload( nullptr );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            static const auto defaultTextureName = String( "checker.png" );
            // A manually supplied GPU texture does not require an authoring database
            // (runtime packages and generated textures may have no source assets).
            auto defaultTexture =
                resourceDatabase ? resourceDatabase->loadResource( defaultTextureName ) : nullptr;
            if( defaultTexture && defaultTexture == texture )
            {
                texture = nullptr;
            }

            for( auto technique : getTechniques() )
            {
                for( auto pass : technique->getPasses() )
                {
                    pass->setTexture( texture, layerIdx );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::setTexture( const String &fileName, u32 layerIdx )
    {
        try
        {
            if( !StringUtil::isNullOrEmpty( fileName ) )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto resourceDatabase = applicationManager->getResourceDatabase();
                auto texture = resourceDatabase->loadResourceByType<ITexture>( fileName );
                if( texture )
                {
                    setTexture( texture, layerIdx );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Material::getTextures() const -> Array<SmartPtr<ITexture>>
    {
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state =
                            stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                    {
                        const auto &textures = state->getSurfaceState().textures;
                        // Preserve the public built-in texture-list extent; indexed
                        // access also supports the record's extension slots.
                        return Array<SmartPtr<ITexture>>(
                            textures.begin(),
                            textures.begin() +
                                static_cast<u32>( PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES ) );
                    }
                }
            }
        }

        return {};
    }

    void Material::setTextures( const Array<SmartPtr<ITexture>> &textures )
    {
        auto layerIdx = 0u;
        for( auto &texture : textures )
        {
            setTexture( texture, layerIdx++ );
        }
    }

    auto Material::getTextureName( u32 layerIdx /*= 0*/ ) const -> String
    {
        if( auto texture = getTexture( layerIdx ) )
        {
            return texture->getName();
        }

        return {};
    }

    auto Material::getTexture( u32 layerIdx ) const -> SmartPtr<ITexture>
    {
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state =
                            stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                    {
                        if( layerIdx < MaterialRenderState::TextureCount )
                        {
                            return state->getSurfaceState().textures[layerIdx];
                        }
                        // Third-party slots beyond the native record retain their adapter.
                        const auto units = pass->getTextureUnits();
                        if( layerIdx < units.size() && units[layerIdx] )
                        {
                            return units[layerIdx]->getTexture();
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    auto Material::getCubeTexture() const -> SmartPtr<ITexture>
    {
        return m_cubeTexture;
    }

    void Material::setCubeTexture( SmartPtr<ITexture> cubeTexture )
    {
        m_cubeTexture = cubeTexture;
    }

    void Material::setLightingEnabled( bool enabled, s32 passIdx /*= -1 */ )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        if( isThreadSafe() )
        {
            if( passIdx == -1 )
            {
                for( auto technique : m_techniques )
                {
                    WP_ASSERT( technique->isValid() );
                    auto passes = technique->getPasses();
                    for( auto pass : passes )
                    {
                        WP_ASSERT( pass->isValid() );
                        pass->setLightingEnabled( enabled );
                    }
                }
            }
            else
            {
                for( auto technique : m_techniques )
                {
                    WP_ASSERT( technique->isValid() );

                    auto passes = technique->getPasses();
                    if( passIdx < passes.size() )
                    {
                        auto pass = passes[passIdx];
                        WP_ASSERT( pass->isValid() );
                        pass->setLightingEnabled( enabled );
                    }
                }
            }
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessagePair<bool, int>>();
            message->setType( LIGHTING_ENABLED_HASH );
            message->setFirst( enabled );
            message->setSecond( passIdx );

            addMessage( message );
        }
    }

    auto Material::getLightingEnabled( s32 passIdx /*= -1 */ ) const -> bool
    {
        for( auto technique : getTechniques() )
        {
            auto passes = technique->getPasses();
            if( passIdx >= 0 )
            {
                if( static_cast<size_t>( passIdx ) < passes.size() )
                {
                    return passes[passIdx]->getLightingEnabled();
                }
            }
            else
            {
                for( auto pass : passes )
                {
                    return pass->getLightingEnabled();
                }
            }
        }

        return false;
    }

    void Material::setCubicTexture( const String &fileName, bool uvw, u32 layerIdx )
    {
    }

    void Material::setCubicTexture( const Array<SmartPtr<ITexture>> &textures, u32 layerIdx /*= 0 */ )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageObjectsArray>();
            message->setType( StateMessage::SET_CUBEMAP );
            message->setObjects( Array<SmartPtr<ISharedObject>>( textures.begin(), textures.end() ) );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( renderTask, message );
            }
        }
    }

    void Material::setFragmentParam( const String &name, f32 value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            //Ogre::Technique *tech = m_material->getBestTechnique();
            //if( tech )
            //{
            //    Ogre::Pass *pass = tech->getPass( 0 );
            //    if( pass )
            //    {
            //        if( pass->hasFragmentProgram() )
            //        {
            //            Ogre::GpuProgramParametersSharedPtr fragmentParameters =
            //                pass->getFragmentProgramParameters();
            //            const Ogre::GpuNamedConstants &fragmentNamedConstants =
            //                fragmentParameters->getConstantDefinitions();
            //            fragmentParameters->setNamedConstant( name.c_str(), value );
            //        }
            //    }
            //}
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_FLOAT_HASH );
            message->setName( name );
            message->setFloat( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( TaskId::Render, message );
            }
        }
    }

    void Material::setFragmentParam( const String &name, const Vector2<real_Num> &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            //Ogre::Technique *tech = m_material->getBestTechnique();
            //if( tech )
            //{
            //    Ogre::Pass *pass = tech->getPass( 0 );
            //    if( pass )
            //    {
            //        if( pass->hasFragmentProgram() )
            //        {
            //            Ogre::GpuProgramParametersSharedPtr fragmentParameters =
            //                pass->getFragmentProgramParameters();
            //            const Ogre::GpuNamedConstants &fragmentNamedConstants =
            //                fragmentParameters->getConstantDefinitions();
            //            // fragmentParameters->setNamedConstant(name.c_str(),
            //            // Ogre::Vector2(value.X(), value.Y()));
            //        }
            //    }
            //}
        }
        else
        {
            SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
            message->setType( FRAGMENT_VECTOR2F_HASH );
            message->setName( name );
            message->setVector2f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( TaskId::Render, message );
            }
        }
    }

    void Material::setFragmentParam( const String &name, const Vector3<real_Num> &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            //Ogre::Technique *tech = m_material->getBestTechnique();
            //if( tech )
            //{
            //    Ogre::Pass *pass = tech->getPass( 0 );
            //    if( pass )
            //    {
            //        if( pass->hasFragmentProgram() )
            //        {
            //            Ogre::GpuProgramParametersSharedPtr fragmentParameters =
            //                pass->getFragmentProgramParameters();
            //            const Ogre::GpuNamedConstants &fragmentNamedConstants =
            //                fragmentParameters->getConstantDefinitions();
            //            fragmentParameters->setNamedConstant(
            //                name.c_str(), Ogre::Vector3( value.X(), value.Y(), value.Z() ) );
            //        }
            //    }
            //}
        }
        else
        {
            auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
            message->setType( FRAGMENT_VECTOR3F_HASH );
            message->setName( name );
            message->setVector3f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( TaskId::Render, message );
            }
        }
    }

    void Material::setFragmentParam( const String &name, const Vector4F &value )
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            //Ogre::Technique *tech = m_material->getBestTechnique();
            //if( tech )
            //{
            //    Ogre::Pass *pass = tech->getPass( 0 );
            //    if( pass )
            //    {
            //        if( pass->hasFragmentProgram() )
            //        {
            //            Ogre::GpuProgramParametersSharedPtr fragmentParameters =
            //                pass->getFragmentProgramParameters();
            //            const Ogre::GpuNamedConstants &fragmentNamedConstants =
            //                fragmentParameters->getConstantDefinitions();
            //            fragmentParameters->setNamedConstant(
            //                name.c_str(),
            //                Ogre::Vector4( value.X(), value.Y(), value.Z(), value.W() ) );
            //        }
            //    }
            //}
        }
        else
        {
            SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
            message->setType( FRAGMENT_VECTOR4F_HASH );
            message->setName( name );
            message->setVector4f( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( TaskId::Render, message );
            }
        }
    }

    void Material::setFragmentParam( const String &name, const ColourF &value )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();

        auto renderTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded && task == renderTask )
        {
            //Ogre::Technique *tech = m_material->getBestTechnique();
            //if( tech )
            //{
            //    Ogre::Pass *pass = tech->getPass( 0 );
            //    if( pass )
            //    {
            //        if( pass->hasFragmentProgram() )
            //        {
            //            Ogre::GpuProgramParametersSharedPtr fragmentParameters =
            //                pass->getFragmentProgramParameters();
            //            const Ogre::GpuNamedConstants &fragmentNamedConstants =
            //                fragmentParameters->getConstantDefinitions();
            //            fragmentParameters->setNamedConstant(
            //                name.c_str(), Ogre::ColourValue( value.r, value.g, value.b, value.a ) );
            //        }
            //    }
            //}
        }
        else
        {
            SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
            message->setType( FRAGMENT_COLOUR_HASH );
            message->setName( name );
            message->setColourf( value );

            if( auto stateContext = getStateContext() )
            {
                stateContext->addMessage( TaskId::Render, message );
            }
        }
    }

    auto Material::createTechnique() -> SmartPtr<IMaterialTechnique>
    {
        invalidateRenderState();
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto factoryManager = graphicsSystem->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto technique = factoryManager->make_object<IMaterialTechnique>();
            technique->setMaterial( this );

            m_techniques.push_back( technique );
            invalidateRenderState();

            auto activeTechnique = getTechnique();
            if( !activeTechnique )
            {
                m_technique = technique;
            }

            return technique;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Material::removeTechnique( SmartPtr<IMaterialTechnique> technique )
    {
        invalidateRenderState();
        if(technique&&std::find(m_techniques.begin(),m_techniques.end(),technique)!=m_techniques.end()) {
            auto owner=technique->getMaterial();if(!owner||owner.get()==this)technique->setMaterial(nullptr);
            setPass(nullptr);
        }
        m_techniques.erase( std::remove( m_techniques.begin(), m_techniques.end(), technique ),
                            m_techniques.end() );
        if(m_technique==technique)m_technique=m_techniques.empty()?nullptr:m_techniques[0];
        invalidateRenderState();
    }

    void Material::setTechniques( const Array<SmartPtr<IMaterialTechnique>> &techniques )
    {
        SourceBudgetConcurrentArray<SmartPtr<IMaterialTechnique>> next( techniques.begin(),
                                                                        techniques.end() );
        for(auto previous:m_techniques)if(previous) {
            auto owner=previous->getMaterial();if(!owner||owner.get()==this)previous->setMaterial(nullptr);
        }
        m_techniques = std::move( next );
        m_technique=techniques.empty()?nullptr:techniques.front();setPass(nullptr);
        for( auto technique : techniques )
        {
            if( technique )
            {
                if(auto previous=technique->getMaterial();previous&&previous.get()!=this)previous->removeTechnique(technique);
                technique->setMaterial( this );
            }
        }
        invalidateRenderState();
    }

    void Material::removeAllTechniques()
    {
        invalidateRenderState();
        for(auto technique:m_techniques)if(technique) {
            auto owner=technique->getMaterial();if(!owner||owner.get()==this)technique->setMaterial(nullptr);
        }
        m_technique=nullptr;setPass(nullptr);
        m_techniques.clear();
        invalidateRenderState();
    }

    SmartPtr<IMaterialTechnique> Material::getTechnique( u32 index ) const
    {
        ScopedLock lock( &m_techniques, false );
        if( index < m_techniques.size() )
        {
            return m_techniques[index];
        }

        return nullptr;
    }

    SmartPtr<IMaterialTechnique> Material::getTechnique() const
    {
        return m_technique;
    }

    void Material::setTechnique( SmartPtr<IMaterialTechnique> technique )
    {
        m_technique = technique;
    }

    auto Material::getTechniques() const -> Array<SmartPtr<IMaterialTechnique>>
    {
        return m_techniques.snapshot();
    }

    auto Material::getTechniqueByScheme( hash32 scheme ) const -> SmartPtr<IMaterialTechnique>
    {
        return m_technique;
    }

    auto Material::getNumTechniques() const -> u32
    {
        return static_cast<u32>( m_techniques.size() );
    }

    void Material::setScale( const Vector3<real_Num> &scale, u32 textureIndex /*= 0*/,
                             u32 passIndex /*= 0*/, u32 techniqueIndex /*= 0 */ )
    {
    }

    f32 Material::getMetalness() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return pass->getMetalness();
            }
        }

        return 0.0f;
    }

    void Material::setMetalness( f32 metalness )
    {
        if( setShaderSurfaceScalar( MaterialBuiltinParameter::Metalness, metalness ) )
        {
            return;
        }
        rememberLegacySurface( MaterialBuiltinParameter::Metalness, metalness );
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setMetalness( metalness );
            }
        }
    }

    f32 Material::getRoughness() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return pass->getRoughness();
            }
        }

        return 0.0f;
    }

    void Material::setRoughness( f32 roughness )
    {
        if( setShaderSurfaceScalar( MaterialBuiltinParameter::Roughness, roughness ) )
        {
            return;
        }
        rememberLegacySurface( MaterialBuiltinParameter::Roughness, roughness );
        if( auto selected = singlePassEdit( *this, m_graphRevision ); selected.pass )
        {
            selected.pass->setRoughness( roughness );
            return;
        }
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setRoughness( roughness );
            }
        }
    }

    ColourF Material::getDiffuse() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return pass->getDiffuse();
            }
        }

        return ColourF::White;
    }

    void Material::setDiffuse( const ColourF &diffuse )
    {
        const bool colorOnly = m_shaderWorkflow == ShaderWorkflow::ColorOnly;
        const bool hasOpacity =
            colorOnly && m_shaderParameters.IsHandleValid(
                             m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Opacity ),
                             MaterialShaderParameterType::Scalar, sizeof( f32 ) );
        // ColorOnly's diffuse alias needs both slots to represent RGBA without
        // changing the existing no-Opacity workflow's opaque diffuse fallback.
        if( !m_synchronizingShaderParameters &&
            ( m_shaderWorkflow == ShaderWorkflow::Pbr || ( colorOnly && hasOpacity ) ) )
        {
            auto edit = m_shaderParameters.Edit();
            if( setShaderSurfaceColour(
                    colorOnly ? MaterialBuiltinParameter::Color : MaterialBuiltinParameter::Albedo,
                    diffuse ) )
            {
                // This API assigns RGBA. Keep a declared opacity semantic in the
                // same edit; typed colour edits retain the explicit Opacity value.
                setShaderSurfaceScalar( MaterialBuiltinParameter::Opacity,
                                        Math<f32>::clamp( diffuse.a, 0.0f, 1.0f ) );
                return;
            }
        }
        if( auto selected = singlePassEdit( *this, m_graphRevision ); selected.pass )
        {
            selected.pass->setDiffuse( diffuse );
            return;
        }
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setDiffuse( diffuse );
            }
        }
    }

    ColourF Material::getSpecular() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return pass->getSpecular();
            }
        }

        return ColourF::White;
    }

    void Material::setSpecular( const ColourF &specular )
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setSpecular( specular );
            }
        }
    }

    ColourF Material::getEmissive() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return pass->getEmissive();
            }
        }

        return ColourF::White;
    }

    void Material::setEmissive( const ColourF &emissive )
    {
        if( setShaderSurfaceColour( m_shaderWorkflow == ShaderWorkflow::ColorOnly
                                        ? MaterialBuiltinParameter::Color
                                        : MaterialBuiltinParameter::Emissive,
                                    emissive ) )
        {
            return;
        }
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setEmissive( emissive );
            }
        }
    }

    auto Material::getRoot() const -> SmartPtr<IMaterialNode>
    {
        return m_root;
    }

    void Material::setRoot( SmartPtr<IMaterialNode> root )
    {
        m_root = root;
    }

    auto Material::getRendererType() const -> hash_type
    {
        return m_rendererType;
    }

    void Material::setRendererType( hash_type rendererType )
    {
        m_rendererType = rendererType;
    }

    void Material::createMaterialByType()
    {
        try
        {
            for( auto t : m_techniques )
            {
                t->setMaterial( this );

                auto passes = t->getPasses();
                for( auto p : passes )
                {
                    // Backend techniques can own their pass array outside the
                    // generic child list. Attach even preinitialized passes;
                    // unchanged ownership preserves their local overrides.
                    p->setMaterial( this );
                    if( !p->isLoaded() )
                    {
                        p->load( nullptr );
                    }
                }
            }
            // Pass constructors may initialize state before the material owner is
            // attached. Apply pre-load values after the complete graph is ready.
            applyPendingLegacySurface();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::addTechnique( SmartPtr<IMaterialTechnique> technique )
    {
        if(m_techniques.size()==m_techniques.capacity())m_techniques.reserve(std::max(size_t(1),m_techniques.capacity()*2));
        if(technique)if(auto previous=technique->getMaterial();previous&&previous.get()!=this)previous->removeTechnique(technique);
        m_techniques.push_back( technique );
        if( technique )
        {
            technique->setMaterial( this );
        }
        invalidateRenderState();
        applyPendingLegacySurface();
    }

    auto Material::getMaterialType() const -> MaterialType
    {
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state =
                            stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                    {
                        return state->getSurfaceState().materialType;
                    }
                }
            }
        }

        return MaterialType::Standard;
    }

    void Material::setMaterialType( MaterialType materialType )
    {
        if( materialType >= MaterialType::Count )
        {
            WP_LOG_WARNING( "Material::setMaterialType ignored an out-of-range material type." );
            return;
        }

        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>(
                            pass->getId() ) )
                    {
                        state->setSurfaceValue( &MaterialRenderState::materialType, materialType );
                    }
                }
            }
        }
    }

    void Material::makeDirty()
    {
        // if (m_isDirty != dirty)
        //{
        //	m_isDirty = dirty;

        //	auto applicationManager = core::IApplicationManager::instance();
        //	WP_ASSERT(applicationManager);

        //	auto factoryManager = applicationManager->getFactoryManager();
        //	auto message = factoryManager->make_ptr<StateMessageDirty>();
        //	message->setDirty(dirty);

        //	auto graphicsSystem = applicationManager->getGraphicsSystem();
        //	auto stateTask = graphicsSystem->getStateTask();
        //	m_stateContext->addMessage(stateTask, message);
        //}
    }

    //
    // SmartPtr<render::IMaterialTechnique> CMaterial::createTechnique()
    //{
    //	auto applicationManager = core::IApplicationManager::instance();
    //	WP_ASSERT(applicationManager);

    //	auto factoryManager = applicationManager->getFactoryManager();
    //	auto technique = factoryManager->make_ptr<CMaterialTechnique>();
    //	technique->setOwner(this);
    //	m_techniques.push_back(technique);
    //	return technique;
    //}

    auto Material::toData() const -> SmartPtr<ISharedObject>
    {
        try
        {
            auto properties = workphone::make_ptr<Properties>();
            // Keep the first pass type at the root so legacy loaders can bootstrap the renderer.
            // The authoritative value for every pass is still serialized by MaterialPass.
            properties->setProperty( materialTypeDataStr, static_cast<s32>( getMaterialType() ) );

            auto techniques = getTechniques();
            for( auto technique : techniques )
            {
                auto pTechniqueData = workphone::static_pointer_cast<Properties>( technique->toData() );
                pTechniqueData->setName( schemesStr );
                properties->addChild( pTechniqueData );
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Material::fromData( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( data && data->isDerived<Properties>() )
            {
                auto properties = workphone::static_pointer_cast<Properties>( data );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto resourceDatabase = applicationManager->getResourceDatabase();

                auto iMaterialType = 0;
                properties->getPropertyValue( materialTypeDataStr, iMaterialType );

                auto materialType = static_cast<MaterialType>( iMaterialType );
                WP_ASSERT( materialType < MaterialType::Count );
                createMaterialByType();
                setMaterialType( materialType );
                createMaterialByType();

                auto editorFloatProperties = SmartPtr<Properties>();
                auto editorUIntProperties = SmartPtr<Properties>();
                auto editorBoolProperties = SmartPtr<Properties>();
                auto editorStringProperties = SmartPtr<Properties>();

                if( auto editorProperties = properties->getChild( editorSettingsStr ) )
                {
                    editorFloatProperties = editorProperties->getChild( editorFloatsStr );
                    editorUIntProperties = editorProperties->getChild( editorUIntsStr );
                    editorBoolProperties = editorProperties->getChild( editorBoolsStr );
                    editorStringProperties = editorProperties->getChild( editorStringsStr );
                }

                auto textures = Array<SmartPtr<ITexture>>();
                textures.resize( (u32)PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES );

                auto textureCount = 0u;
                if( auto texturesChild = properties->getChild( texturesStr ) )
                {
                    auto textureChildren = texturesChild->getChildrenByName( textureStr );
                    for( auto textureChild : textureChildren )
                    {
                        const auto sUUID = textureChild->getProperty( uuidStr );
                        SmartPtr<IResource> resource;
                        if( resourceDatabase && !sUUID.empty() )
                        {
                            resource = resourceDatabase->loadResourceById(
                                StringUtil::parseUUID( sUUID ) );
                        }
                        auto texture = dynamic_pointer_cast<ITexture>( resource );
                        if( resourceDatabase && !resource )
                        {
                            // Legacy root slots can carry an explicit repair path. Resolve
                            // only that registered project path; never invent an identity
                            // from a filename, and never override a live UUID of any type.
                            const auto path = textureChild->getProperty( "texturePath" );
                            const auto relative = std::filesystem::u8path( path.c_str() );
                            bool projectRelative = !path.empty() && !relative.has_root_path();
                            for( const auto &part : relative )
                                if( part == ".." ) projectRelative = false;
                            if( projectRelative )
                                texture = resourceDatabase->loadResourceByType<ITexture>( path );
                        }
                        if( textureCount < textures.size() )
                        {
                            textures[textureCount] = texture;
                        }

                        textureCount++;
                    }
                }

                auto count = 0;

                auto techniques = getTechniques();

                auto schemes = properties->getChildrenByName( schemesStr );
                for( auto &scheme : schemes )
                {
                    auto technique = SmartPtr<IMaterialTechnique>();

                    if( count < techniques.size() )
                    {
                        technique = techniques[count];
                    }
                    else
                    {
                        technique = createTechnique();
                    }

                    technique->fromData( scheme );

                    count++;
                }

                // Pass state is created by scheme deserialization, so apply the
                // root material type after the passes exist.
                setMaterialType( materialType );

                // Legacy materials stored editor/PBR values at the material root.
                // Apply those values only after techniques and passes exist; doing
                // this before scheme deserialization silently discarded every value.
                if( editorFloatProperties )
                {
                    for( const auto &property : editorFloatProperties->getPropertiesAsArray() )
                    {
                        setEditorFloat( property.getName(), property.getValueAsFloat() );
                        if( property.getName() == specularStr )
                        {
                            setSpecularAmount( property.getValueAsFloat() );
                        }
                    }
                }
                if( schemes.empty() &&
                    ( !editorFloatProperties || !editorFloatProperties->hasProperty( specularStr ) ) )
                {
                    // Root-only legacy documents have no authored pass colour. Their
                    // numeric specular default is 0.05, while the old bootstrap pass
                    // starts white. White is F0=1 in the PBR shader and suppresses
                    // diffuse textures entirely. Publish the effective numeric value
                    // to the physical colour without replacing authored scheme data.
                    setSpecularAmount( getSpecularAmount() );
                }
                if( editorUIntProperties )
                {
                    for( const auto &property : editorUIntProperties->getPropertiesAsArray() )
                    {
                        if( property.getName() != materialTypeDataStr )
                        {
                            setEditorUInt( property.getName(),
                                           static_cast<u32>( property.getValueAsInt() ) );
                        }
                    }
                }
                if( editorBoolProperties )
                {
                    for( const auto &property : editorBoolProperties->getPropertiesAsArray() )
                    {
                        setEditorBool( property.getName(), property.getValueAsBool() );
                    }
                }
                if( editorStringProperties )
                {
                    for( const auto &property : editorStringProperties->getPropertiesAsArray() )
                    {
                        setEditorString( property.getName(), property.getValue() );
                    }
                }

                auto layerIdx = 0;
                for( auto texture : textures )
                {
                    if( texture )
                    {
                        setTexture( texture, layerIdx );
                    }

                    layerIdx++;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Material::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = ResourceGraphics<IMaterial>::getProperties();
            if( !properties )
            {
                WP_LOG_ERROR( "Material::getProperties: base class returned null properties." );
                return nullptr;
            }

            auto materialType = getMaterialType();
            if( materialType >= MaterialType::Count )
            {
                WP_LOG_WARNING(
                    "Material::getProperties: replacing an invalid material type with Standard." );
                materialType = MaterialType::Standard;
            }

            auto materialTypeValueStr = GraphicsUtil::getMaterialType( materialType );
            properties->setProperty( materialTypeStr, materialTypeValueStr );

            auto &materialPathProperty = properties->getPropertyObject( materialTypeStr );
            materialPathProperty.setTypeName( enumStr );

            auto enumValues = GraphicsUtil::getMaterialTypesString();
            materialPathProperty.setAttribute( enumStr, enumValues );

            // Reflected file layouts also have typed values without a registered shader id.
            properties->setProperty( shaderIDStr, getShaderID() );
            if( m_shaderParameters.IsValid() )
            {
                auto const &paramInfo = m_shaderParameters.GetParameterInfo();
                for( u32 parameterIndex = 0; parameterIndex < paramInfo.size(); ++parameterIndex )
                {
                    const auto &p = paramInfo[parameterIndex];
                    auto handle = m_shaderParameters.GetParameter( parameterIndex );
                    if( !handle.IsValid() )
                    {
                        continue;
                    }
                    auto key = shaderParamPrefixStr + p.m_name;
                    if( p.m_type == MaterialShaderParameterType::Scalar )
                    {
                        properties->setProperty( key, m_shaderParameters.GetScalar( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Int )
                    {
                        properties->setProperty( key, m_shaderParameters.GetInt( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::UInt )
                    {
                        properties->setProperty( key, m_shaderParameters.GetUInt( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Bool )
                    {
                        properties->setProperty( key, m_shaderParameters.GetBool( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Colour )
                    {
                        properties->setProperty( key, m_shaderParameters.GetColour( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Vector2 )
                    {
                        properties->setProperty( key, m_shaderParameters.GetVector2f( handle ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Vector4 )
                    {
                        auto v = m_shaderParameters.GetVector4f( handle );
                        properties->setProperty( key + shaderParamVectorXStr, v.x );
                        properties->setProperty( key + shaderParamVectorYStr, v.y );
                        properties->setProperty( key + shaderParamVectorZStr, v.z );
                        properties->setProperty( key + shaderParamVectorWStr, v.w );
                    }
                    else if( p.m_type == MaterialShaderParameterType::IntVector2 )
                    {
                        Vector2<s32> value;
                        m_shaderParameters.Get( MaterialParameterHandle<Vector2<s32>>{ handle }, value );
                        properties->setProperty( key + shaderParamVectorXStr, value.x );
                        properties->setProperty( key + shaderParamVectorYStr, value.y );
                    }
                    else if( p.m_type == MaterialShaderParameterType::IntVector4 )
                    {
                        Vector4<s32> value;
                        m_shaderParameters.Get( MaterialParameterHandle<Vector4<s32>>{ handle }, value );
                        properties->setProperty( key + shaderParamVectorXStr, value.x );
                        properties->setProperty( key + shaderParamVectorYStr, value.y );
                        properties->setProperty( key + shaderParamVectorZStr, value.z );
                        properties->setProperty( key + shaderParamVectorWStr, value.w );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Texture )
                    {
                        properties->setProperty( shaderTexturePrefixStr + p.m_name,
                                                 getShaderTexture( p.m_name ) );
                    }
                    else if( p.m_type == MaterialShaderParameterType::Matrix )
                    {
                        auto m = m_shaderParameters.GetMatrix( handle );
                        const f32 *p = m.ptr();
                        for( u32 i = 0; i < 16; ++i )
                        {
                            properties->setProperty( key + shaderParamMatrixSuffixStr[i], p[i] );
                        }
                    }
                }
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Material::setProperties( SmartPtr<Properties> properties )
    {
        auto parameterEdit = m_shaderParameters.Edit();
        try
        {
            if( !properties )
            {
                WP_LOG_WARNING( "Material::setProperties received null properties." );
                return;
            }

            // ResourceGraphics::setProperties treats a missing name as an empty name. Property
            // grids and scripts are allowed to send partial property bags, so only delegate when
            // the resource name is actually present.
            if( properties->hasProperty( IResource::nameStr ) )
            {
                ResourceGraphics<IMaterial>::setProperties( properties );
            }

            auto materialTypeValue = String();
            if( properties->getPropertyValue( Material::materialTypeStr, materialTypeValue ) )
            {
                auto materialType = MaterialType::Standard;
                if( tryParseMaterialType( materialTypeValue, materialType ) )
                {
                    if( getMaterialType() != materialType )
                    {
                        // The renderer consumes the dirty pass state and performs the type-specific
                        // rebuild on its state task. Calling createMaterialByType synchronously here
                        // could lose the update when the graphics lock is busy.
                        setMaterialType( materialType );
                    }
                }
                else
                {
                    WP_LOG_WARNING( String( "Material::setProperties ignored invalid material type: " ) +
                                    materialTypeValue );
                }
            }

            // Rebind known registry shaders before restoring the current typed layout.
            String savedShaderID;
            if( properties->getPropertyValue( shaderIDStr, savedShaderID ) && !savedShaderID.empty() )
            {
                auto shader = MaterialShaderRegistry::instance().find( savedShaderID );
                if( shader )
                {
                    const bool initializePbr =
                        shader->getShaderID() == defaultPBRShaderIDStr &&
                        ( getShaderID() != defaultPBRShaderIDStr || !m_shaderParameters.IsValid() );
                    setShaderParameters( shader->getParameterInfo(), shader->getShaderID() );
                    if( initializePbr )
                    {
                        syncPBRToShaderParameters();
                    }
                }
            }
            restoreShaderParameterProperties( properties );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::restoreShaderParameterProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }
        auto parameterEdit = m_shaderParameters.Edit();
        if( m_shaderParameters.IsValid() )
        {
            auto const &paramInfo = m_shaderParameters.GetParameterInfo();
            for( u32 parameterIndex = 0; parameterIndex < paramInfo.size(); ++parameterIndex )
            {
                const auto &p = paramInfo[parameterIndex];
                auto handle = m_shaderParameters.GetParameter( parameterIndex );
                if( !handle.IsValid() )
                {
                    continue;
                }
                auto key = shaderParamPrefixStr + p.m_name;
                if( p.m_type == MaterialShaderParameterType::Scalar )
                {
                    f32 v = 0.0f;
                    if( properties->getPropertyValue( key, v ) )
                    {
                        m_shaderParameters.SetScalar( handle, v );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Int )
                {
                    s32 value = 0;
                    if( properties->getPropertyValue( key, value ) )
                    {
                        m_shaderParameters.SetScalar( handle, value );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::UInt )
                {
                    u32 value = 0;
                    if( properties->getPropertyValue( key, value ) )
                    {
                        m_shaderParameters.SetScalar( handle, value );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Bool )
                {
                    bool value = false;
                    if( properties->getPropertyValue( key, value ) )
                    {
                        m_shaderParameters.SetBool( handle, value );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Colour )
                {
                    ColourF c;
                    if( properties->getPropertyValue( key, c ) )
                    {
                        m_shaderParameters.SetColour( handle, c );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Vector2 )
                {
                    Vector2F v;
                    if( properties->getPropertyValue( key, v ) )
                    {
                        m_shaderParameters.SetVector2( handle, v );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Vector4 )
                {
                    auto v = m_shaderParameters.GetVector4f( handle );
                    properties->getPropertyValue( key + shaderParamVectorXStr, v.x );
                    properties->getPropertyValue( key + shaderParamVectorYStr, v.y );
                    properties->getPropertyValue( key + shaderParamVectorZStr, v.z );
                    properties->getPropertyValue( key + shaderParamVectorWStr, v.w );
                    m_shaderParameters.SetVector4( handle, v );
                }
                else if( p.m_type == MaterialShaderParameterType::IntVector2 )
                {
                    Vector2<s32> value;
                    m_shaderParameters.Get( MaterialParameterHandle<Vector2<s32>>{ handle }, value );
                    properties->getPropertyValue( key + shaderParamVectorXStr, value.x );
                    properties->getPropertyValue( key + shaderParamVectorYStr, value.y );
                    m_shaderParameters.SetVector2( handle, value );
                }
                else if( p.m_type == MaterialShaderParameterType::IntVector4 )
                {
                    Vector4<s32> value;
                    m_shaderParameters.Get( MaterialParameterHandle<Vector4<s32>>{ handle }, value );
                    properties->getPropertyValue( key + shaderParamVectorXStr, value.x );
                    properties->getPropertyValue( key + shaderParamVectorYStr, value.y );
                    properties->getPropertyValue( key + shaderParamVectorZStr, value.z );
                    properties->getPropertyValue( key + shaderParamVectorWStr, value.w );
                    m_shaderParameters.SetVector4( handle, value );
                }
                else if( p.m_type == MaterialShaderParameterType::Texture )
                {
                    String texPath;
                    if( properties->getPropertyValue( shaderTexturePrefixStr + p.m_name, texPath ) )
                    {
                        setShaderTexture( p.m_name, texPath );
                    }
                }
                else if( p.m_type == MaterialShaderParameterType::Matrix )
                {
                    auto m = m_shaderParameters.GetMatrix( handle );
                    f32 *p = m.ptr();
                    for( u32 i = 0; i < 16; ++i )
                    {
                        properties->getPropertyValue( key + shaderParamMatrixSuffixStr[i], p[i] );
                    }
                    m_shaderParameters.SetMatrix( handle, m );
                }
            }
        }
    }

    auto Material::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
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

    void Material::setCutout( bool cutout )
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setCutout( cutout );
            }
        }
    }

    void Material::setSurfaceFlags( bool transparent, bool cutout )
    {
        if( cutout )
        {
            transparent = false;
        }

        batch( [&, transparent, cutout]( Material & ) {
            auto techniques = getTechniques();
            for( auto technique : techniques )
            {
                auto passes = technique->getPasses();
                for( auto pass : passes )
                {
                    pass->setTransparent( transparent );
                    pass->setCutout( cutout );
                }
            }
        } );
    }

    bool Material::isCutout() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                if( pass->isCutout() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Material::setTransparent( bool transparent )
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setTransparent( transparent );
            }
        }
    }

    bool Material::isTransparent() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                if( pass->isTransparent() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool Material::isEmissionEnabled() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                if( pass->isEmissionEnabled() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Material::setEmissionEnabled( bool enabled )
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setEmissionEnabled( enabled );
            }
        }
    }

    bool Material::isRefractionEnabled() const
    {
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( pass->isRefractionEnabled() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Material::setRefractionEnabled( bool enabled )
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setRefractionEnabled( enabled );
            }
        }
    }

    void Material::setEditorFloat( const String &name, f32 value )
    {
        const auto setting = MaterialPassStateData::resolveFloatSetting( name );
        switch( setting )
        {
        case MaterialFloatSetting::metalness:
            if( setShaderSurfaceScalar( MaterialBuiltinParameter::Metalness, value ) )
            {
                return;
            }
            break;
        case MaterialFloatSetting::roughness:
            if( setShaderSurfaceScalar( MaterialBuiltinParameter::Roughness, value ) )
            {
                return;
            }
            break;
        case MaterialFloatSetting::alphaClip:
            if( setShaderSurfaceScalar( MaterialBuiltinParameter::AlphaClip, value ) )
            {
                return;
            }
            break;
        case MaterialFloatSetting::opacity:
            if( ( m_shaderWorkflow != ShaderWorkflow::Legacy &&
                  m_shaderParameters.IsHandleValid(
                      m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Opacity ),
                      MaterialShaderParameterType::Scalar, sizeof( f32 ) ) ) ||
                ( m_shaderWorkflow == ShaderWorkflow::Pbr &&
                  m_shaderParameters.IsHandleValid(
                      m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Albedo ),
                      MaterialShaderParameterType::Colour, sizeof( ColourF ) ) ) )
            {
                setOpacity( value );
                return;
            }
            break;
        default:
            break;
        }
        if( !setNumericSetting( *this, setting, value ) )
        {
            WP_LOG_WARNING( String( "Material::setEditorFloat ignored unknown setting: " ) + name );
        }
    }

    f32 Material::getEditorFloat( const String &name, f32 defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveFloatSetting( name );
        return getNumericSetting( *this, setting, defaultValue );
    }

    void Material::setEditorUInt( const String &name, u32 value )
    {
        const auto setting = MaterialPassStateData::resolveUIntSetting( name );
        if( !setNumericSetting( *this, setting, value ) )
        {
            WP_LOG_WARNING( String( "Material::setEditorUInt ignored unknown or invalid setting: " ) +
                            name );
        }
    }

    u32 Material::getEditorUInt( const String &name, u32 defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveUIntSetting( name );
        return getNumericSetting( *this, setting, defaultValue );
    }

    void Material::setEditorBool( const String &name, bool value )
    {
        const auto setting = MaterialPassStateData::resolveBoolSetting( name );
        if( !setNumericSetting( *this, setting, value ) )
        {
            WP_LOG_WARNING( String( "Material::setEditorBool ignored unknown setting: " ) + name );
        }

        if( name == doubleSidedStr )
        {
            constexpr auto cullNone = 1u;
            constexpr auto cullClockwise = 2u;
            const auto mode =
                value ? cullNone
                      : getNumericSetting( *this, MaterialUIntSetting::cullMode, cullClockwise );
            applyCullModeToPasses( this, mode );
        }
    }

    bool Material::getEditorBool( const String &name, bool defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveBoolSetting( name );
        return getNumericSetting( *this, setting, defaultValue );
    }

    void Material::setEditorString( const String &name, const String &value )
    {
        const auto setting = MaterialPassStateData::resolveStringSetting( name );
        auto handled = false;
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateDataById<MaterialPassStateData>(
                            pass->getId() ) )
                    {
                        handled = state->setStringSetting( setting, value ) || handled;
                    }
                }
            }
        }

        if( !handled )
        {
            WP_LOG_WARNING( String( "Material::setEditorString ignored unknown setting: " ) + name );
        }
    }

    String Material::getEditorString( const String &name, const String &defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveStringSetting( name );
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state =
                            stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                    {
                        auto value = defaultValue;
                        if( state->getStringSetting( setting, value ) )
                        {
                            return value;
                        }
                    }
                }
            }
        }

        return defaultValue;
    }

    void Material::setRenderMode( u32 mode )
    {
        if( mode > 6u )
        {
            mode = 0u;
        }

        batch( [&, mode]( Material & ) {
            setNumericSetting( *this, MaterialUIntSetting::renderMode, mode );

            switch( mode )
            {
            case 0:
                setSurfaceFlags( false, false );
                setBlendMode( 0 );
                break;
            case 1:
                setSurfaceFlags( false, true );
                setBlendMode( 0 );
                break;
            case 2:
            case 3:
                setSurfaceFlags( true, false );
                setBlendMode( 1 );
                break;
            case 4:
                setSurfaceFlags( true, false );
                setBlendMode( 3 );
                break;
            case 5:
                setSurfaceFlags( true, false );
                setBlendMode( 4 );
                break;
            case 6:
                setSurfaceFlags( true, false );
                setBlendMode( 2 );
                break;
            default:
                break;
            }
        } );
    }

    u32 Material::getRenderMode() const
    {
        auto defaultMode = isTransparent() ? 3u : ( isCutout() ? 1u : 0u );
        return getNumericSetting( *this, MaterialUIntSetting::renderMode, defaultMode );
    }

    void Material::setWorkflow( u32 workflow )
    {
        // Keep this in sync with the material editor's PBR Workflow dropdown.
        setNumericSetting( *this, MaterialUIntSetting::workflow, workflow <= 3u ? workflow : 0u );
    }

    u32 Material::getWorkflow() const
    {
        return getNumericSetting( *this, MaterialUIntSetting::workflow, 0u );
    }

    void Material::setSpecularAmount( f32 value )
    {
        batch( [&, value]( Material & ) {
            setNumericSetting( *this, MaterialFloatSetting::specular, value );
            setSpecular( ColourF( value, value, value, 1.0f ) );
        } );
    }

    f32 Material::getSpecularAmount() const
    {
        auto specular = getSpecular();
        auto defaultValue = ( specular.r + specular.g + specular.b ) / 3.0f;
        return getNumericSetting( *this, MaterialFloatSetting::specular, defaultValue );
    }

    void Material::setNormalStrength( f32 value )
    {
        setNumericSetting( *this, MaterialFloatSetting::normalStrength, value );
    }

    f32 Material::getNormalStrength() const
    {
        return getNumericSetting( *this, MaterialFloatSetting::normalStrength, 1.0f );
    }

    void Material::setDetailNormalStrength( f32 value )
    {
        setNumericSetting( *this, MaterialFloatSetting::detailNormalStrength, value );
    }

    f32 Material::getDetailNormalStrength() const
    {
        return getNumericSetting( *this, MaterialFloatSetting::detailNormalStrength, 1.0f );
    }

    void Material::setAlphaClip( f32 value )
    {
        if( setShaderSurfaceScalar( MaterialBuiltinParameter::AlphaClip, value ) )
        {
            return;
        }
        setNumericSetting( *this, MaterialFloatSetting::alphaClip, value );
    }

    f32 Material::getAlphaClip() const
    {
        return getNumericSetting( *this, MaterialFloatSetting::alphaClip, 0.5f );
    }

    void Material::batch( const std::function<void( Material & )> &update )
    {
        if( !update )
        {
            return;
        }

        if( m_batchActive )
        {
            update( *this );
            return;
        }
        // Keep the flag active through deferred application and scope destruction.
        MaterialParameterSyncScope batching( m_batchActive );

        // Render snapshots publish when a pass state's write-lock scope closes.
        // Hold each authored pass state's write scope across the whole update so
        // nested legacy setter edits publish one snapshot per pass when the batch
        // closes instead of one per setter call. Deferred typed-parameter applies
        // land inside the held scopes and coalesce into the same publication.
        // Unchanged values reuse their current version. Passes created inside the
        // batch keep their existing publication behavior. Owning-thread use,
        // matching the parameter edit-scope contract.
        // Write locks, not invalidations: a batch must not clear or set dirty
        // flags owned by earlier/future edits on the same state.
        std::array<SafePtr<MaterialPassStateData>, 4> inlineScopes;
        size_t inlineCount = 0;
        std::vector<SafePtr<MaterialPassStateData>> overflowScopes;
        for( auto technique : getTechniques() )
        {
            if( !technique )
            {
                continue;
            }
            for( auto pass : technique->getPasses() )
            {
                auto context = pass ? pass->getStateContext() : SmartPtr<IStateContext>{};
                if( !context )
                {
                    continue;
                }
                for( const auto &source : context->getStates() )
                {
                    if( !source || source->getId() != pass->getId() )
                    {
                        continue;
                    }
                    auto data = source->getData();
                    if( !data || !data->isDerived<MaterialPassStateData>() )
                    {
                        continue;
                    }
                    auto pointer = static_cast<MaterialPassStateData *>( data.get() );
                    if( inlineCount < inlineScopes.size() )
                    {
                        inlineScopes[inlineCount++] = pointer;
                    }
                    else
                    {
                        overflowScopes.emplace_back( pointer );
                    }
                    break;
                }
            }
        }

        {
            auto parameterScope = m_shaderParameters.Edit();
            update( *this );
        }
    }

    void Material::setOpacity( f32 value )
    {
        value = Math<real_Num>::clamp( value, 0.0f, 1.0f );
        if( setShaderSurfaceScalar( MaterialBuiltinParameter::Opacity, value ) )
        {
            return;
        }
        // Layouts without a scalar Opacity can still own alpha in Albedo.
        // Read pending authored RGB, rather than an older pass publication.
        if( !m_synchronizingShaderParameters && m_shaderWorkflow == ShaderWorkflow::Pbr )
        {
            const auto handle = m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Albedo );
            if( m_shaderParameters.IsHandleValid( handle, MaterialShaderParameterType::Colour,
                                                  sizeof( ColourF ) ) )
            {
                auto diffuse = m_shaderParameters.GetColour( handle );
                diffuse.a = value;
                {
                    MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );
                    m_shaderParameters.SetColour( handle, diffuse );
                }
                shaderParametersChanged( shaderAlphaOnly );
                return;
            }
        }
        batch( [&, value]( Material & ) {
            rememberLegacySurface( MaterialBuiltinParameter::Opacity, value );
            setNumericSetting( *this, MaterialFloatSetting::opacity, value );

            auto diffuse = getDiffuse();
            diffuse.a = value;
            setDiffuse( diffuse );

            if( value < 0.999f )
            {
                auto renderMode = getRenderMode();
                if( renderMode < 2u )
                {
                    setRenderMode( 3u );
                }
                else
                {
                    setTransparent( true );
                }
            }
        } );
    }

    f32 Material::getOpacity() const
    {
        return getNumericSetting( *this, MaterialFloatSetting::opacity, getDiffuse().a );
    }

    void Material::setBlendMode( u32 blendMode )
    {
        setNumericSetting( *this, MaterialUIntSetting::blendMode, blendMode );

        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setSceneBlending( blendMode );
            }
        }
    }

    u32 Material::getBlendMode() const
    {
        return getNumericSetting( *this, MaterialUIntSetting::blendMode, 0u );
    }

    void Material::setDepthWrite( bool enabled )
    {
        setNumericSetting( *this, MaterialBoolSetting::depthWrite, enabled );

        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setDepthWriteEnabled( enabled );
            }
        }
    }

    bool Material::getDepthWrite() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return getNumericSetting( *this, MaterialBoolSetting::depthWrite,
                                          pass->isDepthWriteEnabled() );
            }
        }

        return getNumericSetting( *this, MaterialBoolSetting::depthWrite, true );
    }

    void Material::setDepthTest( u32 mode )
    {
        setNumericSetting( *this, MaterialUIntSetting::depthTest, mode );

        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setDepthCheckEnabled( mode != 0u );
            }
        }
    }

    u32 Material::getDepthTest() const
    {
        return getNumericSetting( *this, MaterialUIntSetting::depthTest, 2u );
    }

    void Material::setCullMode( u32 mode )
    {
        setNumericSetting( *this, MaterialUIntSetting::cullMode, mode );

        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                pass->setCullingMode( mode );
            }
        }
    }

    u32 Material::getCullMode() const
    {
        auto techniques = getTechniques();
        for( auto technique : techniques )
        {
            auto passes = technique->getPasses();
            for( auto pass : passes )
            {
                return getNumericSetting( *this, MaterialUIntSetting::cullMode, pass->getCullingMode() );
            }
        }

        return getNumericSetting( *this, MaterialUIntSetting::cullMode, 2u );
    }

    void Material::setUVTiling( const Vector2F &tiling )
    {
        batch( [&, tiling]( Material & ) {
            setNumericSetting( *this, MaterialFloatSetting::uvTilingX, tiling.X() );
            setNumericSetting( *this, MaterialFloatSetting::uvTilingY, tiling.Y() );
            setScale( Vector3F( tiling.X(), tiling.Y(), 1.0f ), 0u );
        } );
    }

    Vector2F Material::getUVTiling() const
    {
        return Vector2F( getNumericSetting( *this, MaterialFloatSetting::uvTilingX, 1.0f ),
                         getNumericSetting( *this, MaterialFloatSetting::uvTilingY, 1.0f ) );
    }

    void Material::setUVOffset( const Vector2F &offset )
    {
        batch( [&, offset]( Material & ) {
            setNumericSetting( *this, MaterialFloatSetting::uvOffsetX, offset.X() );
            setNumericSetting( *this, MaterialFloatSetting::uvOffsetY, offset.Y() );
        } );
    }

    Vector2F Material::getUVOffset() const
    {
        return Vector2F( getNumericSetting( *this, MaterialFloatSetting::uvOffsetX, 0.0f ),
                         getNumericSetting( *this, MaterialFloatSetting::uvOffsetY, 0.0f ) );
    }

    void Material::setUVRotation( f32 rotation )
    {
        setNumericSetting( *this, MaterialFloatSetting::uvRotation, rotation );
    }

    f32 Material::getUVRotation() const
    {
        return getNumericSetting( *this, MaterialFloatSetting::uvRotation, 0.0f );
    }

    void Material::setUVProjection( u32 projection )
    {
        // Keep this in sync with the material editor's Projection dropdown.
        setNumericSetting( *this, MaterialUIntSetting::uvProjection,
                           projection <= 6u ? projection : 0u );
    }

    u32 Material::getUVProjection() const
    {
        const auto projection = getNumericSetting( *this, MaterialUIntSetting::uvProjection, 0u );
        return projection <= 6u ? projection : 0u;
    }

    void Material::setUVSet( u32 uvSet )
    {
        setNumericSetting( *this, MaterialUIntSetting::uvSet, uvSet <= 3u ? uvSet : 0u );
    }

    u32 Material::getUVSet() const
    {
        const auto uvSet = getNumericSetting( *this, MaterialUIntSetting::uvSet, 0u );
        return uvSet <= 3u ? uvSet : 0u;
    }

    void Material::setTriplanarScale( f32 scale )
    {
        const auto validScale = std::isfinite( scale ) ? Math<real_Num>::max( scale, 0.0001f ) : 1.0f;
        setNumericSetting( *this, MaterialFloatSetting::uvTriplanarScale, validScale );
    }

    f32 Material::getTriplanarScale() const
    {
        const auto scale = getNumericSetting( *this, MaterialFloatSetting::uvTriplanarScale, 1.0f );
        return std::isfinite( scale ) ? Math<real_Num>::max( scale, 0.0001f ) : 1.0f;
    }

    bool Material::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto techniques = m_techniques.snapshot();
        for( auto &technique : techniques )
        {
            if( message->getSender() == technique )
            {
                if( technique->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool Material::handleStateChanged( SmartPtr<IState> &state )
    {
        auto techniques = m_techniques.snapshot();
        for( auto &technique : techniques )
        {
            if( state->getOwnerPtr() == technique )
            {
                if( technique->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    Array<SmartPtr<ITexture>> Material::getCubicTextures() const
    {
        const auto textures = getCubicTextureSnapshot();
        return textures ? *textures : Array<SmartPtr<ITexture>>{};
    }

    Material::CubicTextureSnapshot Material::getCubicTextureSnapshot() const
    {
        return std::atomic_load( &m_cubicTextures );
    }

    void Material::setCubicTextures( const Array<SmartPtr<ITexture>> &cubicTextures )
    {
        const auto previous = getCubicTextureSnapshot();
        if( ( !previous && cubicTextures.empty() ) || ( previous && *previous == cubicTextures ) )
        {
            return;
        }
        CubicTextureSnapshot next;
        if( !cubicTextures.empty() )
        {
            next = copySourceBudgetArray( cubicTextures );
        }
        // Build before publication; retained readers keep the complete old list.
        std::atomic_store( &m_cubicTextures, std::move( next ) );
    }

    void Material::MaterialStateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    bool Material::MaterialStateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto messageType = message->getType();

        auto owner = getOwner();

        if( message->isExactly<StateMessageIntValue>() )
        {
            auto intMessage = workphone::static_pointer_cast<StateMessageIntValue>( message );
            auto value = intMessage->getValue();

            if( messageType == LIGHTING_ENABLED_HASH )
            {
                owner->setLightingEnabled( value );
            }
        }
        else if( message->isExactly<StateMessageUIntValue>() )
        {
            auto intMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );

            if( intMessage->getType() == StringUtil::getHash( materialTypeDataStr ) )
            {
                owner->setMaterialType( static_cast<MaterialType>( intMessage->getValue() ) );
            }
        }
        else if( message->isExactly<StateMessageLoad>() )
        {
            auto loadMessage = workphone::static_pointer_cast<StateMessageLoad>( message );
            if( loadMessage->getType() == StateMessageLoad::LOAD_HASH )
            {
                owner->load( nullptr );
            }
            else
            {
                owner->reload( nullptr );
            }
        }
        else if( message->isExactly<StateMessageFragmentParam>() )
        {
            auto fragmentMessage = workphone::static_pointer_cast<StateMessageFragmentParam>( message );
            auto type = fragmentMessage->getType();

            if( type == FRAGMENT_FLOAT_HASH )
            {
                owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getFloat() );
            }
            else if( type == FRAGMENT_VECTOR2F_HASH )
            {
                owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector2f() );
            }
            else if( type == FRAGMENT_VECTOR3F_HASH )
            {
                owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector3f() );
            }
            else if( type == FRAGMENT_VECTOR4F_HASH )
            {
                owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getVector4f() );
            }
            else if( type == FRAGMENT_COLOUR_HASH )
            {
                owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getColourf() );
            }
        }
        else if( message->isExactly<StateMessageSetTexture>() )
        {
            auto textureMessage = workphone::static_pointer_cast<StateMessageSetTexture>( message );
            if( auto texture = textureMessage->getTexture() )
            {
                const auto textureIndex = textureMessage->getTextureIndex();
                owner->setTexture( texture, textureIndex );
            }
            else
            {
                const auto textureName = textureMessage->getTextureName();
                const auto textureIndex = textureMessage->getTextureIndex();
                owner->setTexture( textureName, textureIndex );
            }
        }
        else if( message->isExactly<StateMessageObjectsArray>() )
        {
            auto arrayMessage = workphone::static_pointer_cast<StateMessageObjectsArray>( message );
            auto type = arrayMessage->getType();
            auto value = arrayMessage->getObjects();

            if( type == StateMessage::SET_CUBEMAP )
            {
                owner->setCubicTexture( Array<SmartPtr<ITexture>>( value.begin(), value.end() ) );
            }
        }

        return false;
    }

    bool Material::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    auto Material::MaterialStateListener::getOwner() const -> SmartPtr<Material>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Material::MaterialStateListener::setOwner( SmartPtr<Material> owner )
    {
        m_owner = owner;
    }

    Material::MaterialStateListener::~MaterialStateListener() = default;

    Material::MaterialStateListener::MaterialStateListener() = default;

    Material::MaterialStateListener::MaterialStateListener( Material *material ) : m_owner( material )
    {
    }

    void Material::syncPBRToShaderParameters()
    {
        if( !isDataDriven() || !m_shaderParameters.IsValid() )
        {
            return;
        }

        const auto published = getRenderState();
        const auto state = ( m_batchActive || MaterialPublicationScope::isActive() )
                               ? MaterialRenderState::Ptr{}
                               : published;
        MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );

        // Explicitly import legacy values for declared slots without feeding the
        // writes back to the passes. Rendering consumes immutable pass snapshots.
        auto trySetScalar = [this]( MaterialBuiltinParameter semantic, f32 value ) {
            auto handle = m_shaderParameters.GetBuiltin( semantic );
            if( handle.IsValid() )
            {
                m_shaderParameters.SetScalar( handle, value );
            }
        };
        auto trySetColour = [this]( MaterialBuiltinParameter semantic, const ColourF &value ) {
            auto handle = m_shaderParameters.GetBuiltin( semantic );
            if( handle.IsValid() )
            {
                m_shaderParameters.SetColour( handle, value );
            }
        };

        // Before graph creation, the optional legacy block is the value owner.
        // Inside a batch, published snapshots still contain the previous values.
        auto importScalar = [this, &published]( MaterialBuiltinParameter semantic, f32 value ) {
            if( !published && m_legacySurface &&
                ( m_legacySurface->mask & legacySurfaceMask( semantic ) ) )
            {
                switch( semantic )
                {
                case MaterialBuiltinParameter::Metalness:
                    return m_legacySurface->metalness;
                case MaterialBuiltinParameter::Roughness:
                    return m_legacySurface->roughness;
                case MaterialBuiltinParameter::Opacity:
                    return m_legacySurface->opacity;
                default:
                    break;
                }
            }
            return value;
        };
        auto albedo = state ? state->diffuseColour : getDiffuse();
        if( !published )
        {
            albedo.a = importScalar( MaterialBuiltinParameter::Opacity, albedo.a );
        }
        trySetColour( MaterialBuiltinParameter::Albedo, albedo );
        if( m_shaderWorkflow == ShaderWorkflow::ColorOnly )
        {
            trySetColour( MaterialBuiltinParameter::Color, albedo );
        }
        trySetScalar( MaterialBuiltinParameter::Metalness,
                      importScalar( MaterialBuiltinParameter::Metalness,
                                    state ? state->metalness : getMetalness() ) );
        trySetScalar( MaterialBuiltinParameter::Roughness,
                      importScalar( MaterialBuiltinParameter::Roughness,
                                    state ? state->roughness : getRoughness() ) );
        trySetColour( MaterialBuiltinParameter::Emissive,
                      state ? state->emissiveColour : getEmissive() );
        trySetScalar(
            MaterialBuiltinParameter::Opacity,
            importScalar( MaterialBuiltinParameter::Opacity, state ? state->opacity : getOpacity() ) );
        trySetScalar( MaterialBuiltinParameter::AlphaClip, state ? state->alphaClip : getAlphaClip() );

        if( m_shaderWorkflow == ShaderWorkflow::Pbr )
        {
            auto ownsScalar = [this]( MaterialBuiltinParameter semantic ) {
                return m_shaderParameters.IsHandleValid( m_shaderParameters.GetBuiltin( semantic ),
                                                         MaterialShaderParameterType::Scalar,
                                                         sizeof( f32 ) );
            };
            if( ownsScalar( MaterialBuiltinParameter::Metalness ) )
            {
                forgetLegacySurface( MaterialBuiltinParameter::Metalness );
            }
            if( ownsScalar( MaterialBuiltinParameter::Roughness ) )
            {
                forgetLegacySurface( MaterialBuiltinParameter::Roughness );
            }
            if( ownsScalar( MaterialBuiltinParameter::Opacity ) ||
                m_shaderParameters.IsHandleValid(
                    m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Albedo ),
                    MaterialShaderParameterType::Colour, sizeof( ColourF ) ) )
            {
                forgetLegacySurface( MaterialBuiltinParameter::Opacity );
            }
        }
        else if( ( m_shaderWorkflow == ShaderWorkflow::Unlit ||
                   m_shaderWorkflow == ShaderWorkflow::ColorOnly ) &&
                 m_shaderParameters.IsHandleValid(
                     m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Opacity ),
                     MaterialShaderParameterType::Scalar, sizeof( f32 ) ) )
        {
            forgetLegacySurface( MaterialBuiltinParameter::Opacity );
        }
        m_shaderParametersDirty = false;
    }

    //---------------------------------------------------------------------------------------
    // Publish supported shader semantics through the existing pass state and
    // virtual setters. Renderer threads continue to read immutable snapshots.
    //---------------------------------------------------------------------------------------
    void Material::routeDataDrivenMaterialType()
    {
        if( !isDataDriven() )
        {
            return;
        }
        auto target =
            ( m_shaderWorkflow != ShaderWorkflow::Pbr ) ? MaterialType::UI : MaterialType::Standard;
        if( getMaterialType() != target )
        {
            setMaterialType( target );
        }
    }

    std::size_t Material::getLegacySurfaceStorageBytes() const
    {
        return m_legacySurface ? sizeof( LegacySurfaceData ) : 0;
    }

    SourceBudgetUnique<Material::LegacySurfaceData> Material::prepareShaderSurfaceHandoff(
        const MaterialShaderParameterLayout::Ptr &nextLayout ) const
    {
        return prepareShaderSurfaceHandoff( nextLayout, m_shaderWorkflow != ShaderWorkflow::Legacy,
                                            m_shaderWorkflow == ShaderWorkflow::Pbr );
    }

    SourceBudgetUnique<Material::LegacySurfaceData> Material::prepareShaderSurfaceHandoff(
        const MaterialShaderParameterLayout::Ptr &nextLayout, bool nextActive, bool nextPbr ) const
    {
        if( m_shaderWorkflow == ShaderWorkflow::Legacy )
        {
            return {};
        }
        auto values = m_legacySurface ? *m_legacySurface : LegacySurfaceData{};
        bool changed = false;
        for( const auto semantic :
             { MaterialBuiltinParameter::Metalness, MaterialBuiltinParameter::Roughness,
               MaterialBuiltinParameter::Opacity } )
        {
            f32 value = 0.0f;
            if( shaderOwnsSurfaceScalar( nextLayout, nextActive, nextPbr, semantic ) ||
                !readShaderSurfaceScalar( m_shaderParameters, m_shaderWorkflow == ShaderWorkflow::Pbr,
                                          semantic, value ) )
            {
                continue;
            }
            auto *destination = semantic == MaterialBuiltinParameter::Metalness   ? &values.metalness
                                : semantic == MaterialBuiltinParameter::Roughness ? &values.roughness
                                                                                  : &values.opacity;
            const auto mask = legacySurfaceMask( semantic );
            changed = changed || !( values.mask & mask ) ||
                      std::memcmp( destination, &value, sizeof( value ) ) != 0;
            *destination = value;
            values.mask |= mask;
        }
        return changed ? makeSourceBudgetUnique<LegacySurfaceData>( values ) : nullptr;
    }

    void Material::commitShaderSurfaceHandoff( SourceBudgetUnique<LegacySurfaceData> values ) noexcept
    {
        if( values )
        {
            m_legacySurface = std::move( values );
        }
    }

    void Material::rememberLegacySurface( MaterialBuiltinParameter semantic, f32 value )
    {
        const auto mask = legacySurfaceMask( semantic );
        if( !mask )
        {
            return;
        }
        if( !m_legacySurface )
        {
            m_legacySurface = makeSourceBudgetUnique<LegacySurfaceData>();
        }
        switch( semantic )
        {
        case MaterialBuiltinParameter::Opacity:
            m_legacySurface->opacity = value;
            break;
        case MaterialBuiltinParameter::Metalness:
            m_legacySurface->metalness = value;
            break;
        case MaterialBuiltinParameter::Roughness:
            m_legacySurface->roughness = value;
            break;
        default:
            break;
        }
        m_legacySurface->mask |= mask;
    }

    void Material::forgetLegacySurface( MaterialBuiltinParameter semantic )
    {
        if( !m_legacySurface )
        {
            return;
        }
        m_legacySurface->mask &= static_cast<u8>( ~legacySurfaceMask( semantic ) );
        if( !m_legacySurface->mask )
        {
            m_legacySurface.reset();
        }
    }

    void Material::applyPendingLegacySurface()
    {
        // Pass attachment/load seeds retained fallback values independently.
        // Replaying a missing semantic here would overwrite existing local pass
        // overrides every time another technique is attached. Only a newly
        // compatible shader owner needs a material-wide transfer.
        if( m_legacySurface &&
            ( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Roughness ) ) )
        {
            setShaderSurfaceScalar( MaterialBuiltinParameter::Roughness, m_legacySurface->roughness );
        }
        if( m_legacySurface &&
            ( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Metalness ) ) )
        {
            setShaderSurfaceScalar( MaterialBuiltinParameter::Metalness, m_legacySurface->metalness );
        }
        if( m_legacySurface &&
            ( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Opacity ) ) &&
            shaderOwnsSurfaceScalar(
                m_shaderParameters.GetLayout(), m_shaderWorkflow != ShaderWorkflow::Legacy,
                m_shaderWorkflow == ShaderWorkflow::Pbr, MaterialBuiltinParameter::Opacity ) )
        {
            setOpacity( m_legacySurface->opacity );
        }
    }

    void Material::seedLegacySurface( MaterialPassStateData &state ) const
    {
        if( !m_legacySurface )
        {
            return;
        }
        if( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Metalness ) )
        {
            state.setSurfaceValue( &MaterialRenderState::metalness, m_legacySurface->metalness );
        }
        if( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Roughness ) )
        {
            state.setSurfaceValue( &MaterialRenderState::roughness, m_legacySurface->roughness );
        }
        if( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Opacity ) )
        {
            const auto opacity = m_legacySurface->opacity;
            state.setSurfaceValue( &MaterialRenderState::opacity, opacity );
            auto diffuse = state.getSurfaceState().diffuseColour;
            diffuse.a = opacity;
            state.setSurfaceValue( &MaterialRenderState::diffuseColour, diffuse );
            if( opacity < 0.999f )
            {
                state.setFlag( render::transparentFlag, true );
                u32 renderMode = 0;
                state.getUIntSetting( MaterialUIntSetting::renderMode, renderMode );
                if( renderMode < 2u )
                {
                    state.setUIntSetting( MaterialUIntSetting::renderMode, 3u );
                    state.setSurfaceValue( &MaterialRenderState::blendMode, 1u );
                    state.setFlag( render::cutoutFlag, false );
                }
            }
        }
    }

    bool Material::setShaderSurfaceScalar( MaterialBuiltinParameter semantic, f32 value )
    {
        if( m_synchronizingShaderParameters || !( m_shaderWorkflow == ShaderWorkflow::Pbr ||
                                                  ( ( m_shaderWorkflow == ShaderWorkflow::Unlit ||
                                                      m_shaderWorkflow == ShaderWorkflow::ColorOnly ) &&
                                                    semantic == MaterialBuiltinParameter::Opacity ) ) )
        {
            return false;
        }
        const auto handle = m_shaderParameters.GetBuiltin( semantic );
        if( !m_shaderParameters.IsHandleValid( handle, MaterialShaderParameterType::Scalar,
                                               sizeof( f32 ) ) )
        {
            return false;  // Absent/wrong-type slots keep the legacy pass contract.
        }

        const auto revision = m_shaderParameters.GetRevision();
        m_shaderParameters.SetScalar( handle, value );
        // A material-wide assignment must also restore a pass-local override when
        // the authored bytes already equal value. Only reapply this semantic;
        // unrelated overrides and custom values retain their existing owners.
        if( m_shaderParameters.GetRevision() == revision )
        {
            shaderParametersChanged( static_cast<u8>( 1u << static_cast<u32>( semantic ) ) );
        }
        return true;
    }

    bool Material::setShaderSurfaceColour( MaterialBuiltinParameter semantic, const ColourF &value )
    {
        if( m_synchronizingShaderParameters || !( m_shaderWorkflow == ShaderWorkflow::Pbr ||
                                                  ( m_shaderWorkflow == ShaderWorkflow::Unlit &&
                                                    semantic == MaterialBuiltinParameter::Emissive ) ||
                                                  ( m_shaderWorkflow == ShaderWorkflow::ColorOnly &&
                                                    semantic == MaterialBuiltinParameter::Color ) ) )
        {
            return false;
        }
        const auto handle = m_shaderParameters.GetBuiltin( semantic );
        if( !m_shaderParameters.IsHandleValid( handle, MaterialShaderParameterType::Colour,
                                               sizeof( ColourF ) ) )
        {
            return false;
        }
        const auto revision = m_shaderParameters.GetRevision();
        m_shaderParameters.SetColour( handle, value );
        if( m_shaderParameters.GetRevision() == revision )
        {
            shaderParametersChanged( static_cast<u8>( 1u << static_cast<u32>( semantic ) ) );
        }
        return true;
    }

    void Material::shaderParametersChanged( u8 mask ) noexcept
    {
        if( m_synchronizingShaderParameters )
        {
            return;
        }
        m_pendingShaderSemantics |= mask;
        if( !m_pendingShaderSemantics )
        {
            return;
        }
        m_shaderParametersDirty = true;
        if( m_shaderParameterEditDepth )
        {
            return;
        }
        try
        {
            applyShaderParametersToRenderer( m_pendingShaderSemantics );
        }
        catch( const std::exception &error )
        {
            WP_LOG_EXCEPTION( error );
        }
        catch( ... )
        {
            // Keep the pending mask so an explicit apply/load can retry. Parameter
            // writes and noexcept moves must not throw from publication callbacks.
        }
    }

    void Material::applyShaderParametersToRenderer()
    {
        if( m_shaderParameterEditDepth )
        {
            m_pendingShaderSemantics = 0xffu;
            m_shaderParametersDirty = true;
            return;
        }
        applyShaderParametersToRenderer( 0xffu );
    }

    void Material::applyShaderParametersToRenderer( u8 mask )
    {
        if( !isDataDriven() || !m_shaderParameters.IsValid() )
        {
            return;
        }
        const auto state = getRenderState();
        auto currentDiffuse = state ? state->diffuseColour : getDiffuse();
        auto currentEmissive = state ? state->emissiveColour : getEmissive();
        auto currentMetal = state ? state->metalness : getMetalness();
        auto currentRough = state ? state->roughness : getRoughness();
        auto currentClip = state ? state->alphaClip : getAlphaClip();
        auto currentOpacity = state ? state->opacity : getOpacity();
        if( m_batchActive || MaterialPublicationScope::isActive() )
        {
            // The published snapshot intentionally predates this edit boundary. Read
            // pending owner-side fallbacks once through cached primary selection,
            // so an opacity-only edit preserves new RGB without name resolution
            // or a separate graph traversal for every fallback setting.
            const auto selection = std::atomic_load( &m_primaryPassSelection );
            auto data = selection && selection->state ? selection->state->getData() : nullptr;
            if( data && data->isDerived<MaterialPassStateData>() )
            {
                SafeReadPtr<MaterialPassStateData> live(
                    static_cast<MaterialPassStateData *>( data.get() ) );
                const auto &surface = live->getSurfaceState();
                currentDiffuse = surface.diffuseColour;
                currentEmissive = surface.emissiveColour;
                currentMetal = surface.metalness;
                currentRough = surface.roughness;
                currentClip = surface.alphaClip;
                currentOpacity = surface.opacity;
            }
        }
        MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );
        const auto wants = [&]( MaterialBuiltinParameter semantic, MaterialShaderParameterType type,
                                u32 size ) {
            return mask == 0xffu || ( ( mask & ( 1u << static_cast<u32>( semantic ) ) ) &&
                                      m_shaderParameters.IsHandleValid(
                                          m_shaderParameters.GetBuiltin( semantic ), type, size ) );
        };
        const auto scalar = [&]( MaterialBuiltinParameter semantic ) {
            return wants( semantic, MaterialShaderParameterType::Scalar, sizeof( f32 ) );
        };
        const auto colour = [&]( MaterialBuiltinParameter semantic ) {
            return wants( semantic, MaterialShaderParameterType::Colour, sizeof( ColourF ) );
        };
        auto getScalar = [this]( MaterialBuiltinParameter semantic, f32 def ) -> f32 {
            auto h = m_shaderParameters.GetBuiltin( semantic );
            return m_shaderParameters.IsHandleValid( h, MaterialShaderParameterType::Scalar,
                                                     sizeof( f32 ) )
                       ? m_shaderParameters.GetScalar( h )
                       : def;
        };
        auto getColour = [this]( MaterialBuiltinParameter semantic, const ColourF &def ) -> ColourF {
            auto h = m_shaderParameters.GetBuiltin( semantic );
            return m_shaderParameters.IsHandleValid( h, MaterialShaderParameterType::Colour,
                                                     sizeof( ColourF ) )
                       ? m_shaderParameters.GetColour( h )
                       : def;
        };

        const bool pbr = m_shaderWorkflow == ShaderWorkflow::Pbr;
        const bool colorOnly = m_shaderWorkflow == ShaderWorkflow::ColorOnly;
        const bool diffuseChanged = colorOnly ? colour( MaterialBuiltinParameter::Color )
                                              : pbr && colour( MaterialBuiltinParameter::Albedo );
        const bool emissiveChanged =
            colorOnly ? diffuseChanged : colour( MaterialBuiltinParameter::Emissive );
        const bool metalChanged = pbr && scalar( MaterialBuiltinParameter::Metalness );
        const bool roughChanged = pbr && scalar( MaterialBuiltinParameter::Roughness );
        const bool clipChanged = pbr && scalar( MaterialBuiltinParameter::AlphaClip );
        const bool opacityChanged = diffuseChanged || scalar( MaterialBuiltinParameter::Opacity ) ||
                                    ( pbr && ( mask & shaderAlphaOnly ) );
        auto diffuse = diffuseChanged
                           ? getColour( colorOnly ? MaterialBuiltinParameter::Color
                                                  : MaterialBuiltinParameter::Albedo,
                                        colorOnly ? ColourF( 0.5f, 0.5f, 0.5f, 1 ) : currentDiffuse )
                           : currentDiffuse;
        const auto emissive =
            colorOnly ? getColour( MaterialBuiltinParameter::Color, ColourF( 0.5f, 0.5f, 0.5f, 1 ) )
                      : getColour( MaterialBuiltinParameter::Emissive,
                                   pbr ? currentEmissive : ColourF( 0.5f, 0.5f, 0.5f, 1 ) );
        const auto metal = getScalar( MaterialBuiltinParameter::Metalness, currentMetal );
        const auto rough = getScalar( MaterialBuiltinParameter::Roughness, currentRough );
        const auto clip = getScalar( MaterialBuiltinParameter::AlphaClip, currentClip );
        auto opacityFallback = pbr ? currentOpacity : 1.0f;
        // Once a scalar slot hands off, legacy opacity is the explicit owner.
        // A pending Color apply must not replace it with an implicit default.
        if( colorOnly && m_legacySurface &&
            ( m_legacySurface->mask & legacySurfaceMask( MaterialBuiltinParameter::Opacity ) ) )
        {
            opacityFallback = m_legacySurface->opacity;
        }
        if( pbr && diffuseChanged )
        {
            opacityFallback = diffuse.a;
        }
        else if( pbr && ( mask & shaderAlphaOnly ) )
        {
            opacityFallback = getColour( MaterialBuiltinParameter::Albedo, currentDiffuse ).a;
        }
        const auto opacity = Math<f32>::clamp(
            getScalar( MaterialBuiltinParameter::Opacity, opacityFallback ), 0.0f, 1.0f );
        const bool transparent = opacityChanged && opacity < 0.999f;
        const bool changeRenderModeFallback = transparent && getRenderMode() < 2u;
        const auto ownsScalar = [&]( MaterialBuiltinParameter semantic ) {
            return m_shaderParameters.IsHandleValid( m_shaderParameters.GetBuiltin( semantic ),
                                                     MaterialShaderParameterType::Scalar,
                                                     sizeof( f32 ) );
        };
        const auto updateFuturePassValue = [&]( MaterialBuiltinParameter semantic, f32 value,
                                                bool canonical ) {
            if( canonical )
            {
                forgetLegacySurface( semantic );
            }
            else
            {
                rememberLegacySurface( semantic, value );
            }
        };
        if( opacityChanged )
        {
            diffuse.a = opacity;
            const bool canonical =
                ( m_shaderWorkflow != ShaderWorkflow::Legacy &&
                  ownsScalar( MaterialBuiltinParameter::Opacity ) ) ||
                ( pbr && m_shaderParameters.IsHandleValid(
                             m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Albedo ),
                             MaterialShaderParameterType::Colour, sizeof( ColourF ) ) );
            updateFuturePassValue( MaterialBuiltinParameter::Opacity, opacity, canonical );
        }
        if( metalChanged )
        {
            updateFuturePassValue( MaterialBuiltinParameter::Metalness, metal,
                                   ownsScalar( MaterialBuiltinParameter::Metalness ) );
        }
        if( roughChanged )
        {
            updateFuturePassValue( MaterialBuiltinParameter::Roughness, rough,
                                   ownsScalar( MaterialBuiltinParameter::Roughness ) );
        }

        bool applied = false;
        // One graph walk and one write scope per pass. Virtual surface setters keep
        // backend behavior; the outer state lock coalesces their publications.
        for( auto technique : getTechniques() )
        {
            if( !technique )
            {
                continue;
            }
            for( auto pass : technique->getPasses() )
            {
                if( !pass )
                {
                    continue;
                }
                auto context = pass->getStateContext();
                auto data =
                    context ? context->invalidateStateDataById<MaterialPassStateData>( pass->getId() )
                            : SafePtr<MaterialPassStateData>{};
                u32 renderMode = 0;
                if( data )
                {
                    data->getUIntSetting( MaterialUIntSetting::renderMode, renderMode );
                }
                const bool changeRenderMode =
                    transparent && ( data ? renderMode < 2u : changeRenderModeFallback );
                if( diffuseChanged )
                {
                    pass->setDiffuse( diffuse );
                }
                else if( opacityChanged )
                {
                    // Alpha-only edits preserve each pass's local RGB override,
                    // including live edits inside an enclosing publication scope.
                    auto passDiffuse = pass->getDiffuse();
                    passDiffuse.a = opacity;
                    pass->setDiffuse( passDiffuse );
                }
                if( emissiveChanged )
                {
                    pass->setEmissive( emissive );
                }
                if( metalChanged )
                {
                    pass->setMetalness( metal );
                }
                if( roughChanged )
                {
                    pass->setRoughness( rough );
                }
                if( data )
                {
                    if( clipChanged )
                    {
                        data->setFloatSetting( MaterialFloatSetting::alphaClip, clip );
                    }
                    if( opacityChanged )
                    {
                        data->setFloatSetting( MaterialFloatSetting::opacity, opacity );
                    }
                    if( changeRenderMode )
                    {
                        data->setUIntSetting( MaterialUIntSetting::renderMode, 3u );
                    }
                }
                if( transparent )
                {
                    pass->setTransparent( true );
                }
                if( changeRenderMode )
                {
                    pass->setCutout( false );
                    pass->setSceneBlending( 1u );
                }
                applied = true;
            }
        }
        if( applied )
        {
            m_shaderParametersDirty = false;
            m_pendingShaderSemantics = 0;
        }
        else
        {
            m_shaderParametersDirty = true;
            m_pendingShaderSemantics |= mask;
        }
    }

    void Material::applyDataDrivenShaderToRenderer()
    {
        routeDataDrivenMaterialType();
        applyShaderParametersToRenderer();
        applyDataDrivenTextures();
    }

    void Material::setShaderTexture( const String &paramName, const String &texturePath )
    {
        if( !m_shaderAuthoring )
        {
            auto next = makeSourceBudgetUnique<ShaderAuthoringData>();
            next->textureRefs.push_back( { paramName, texturePath } );
            m_shaderAuthoring = std::move( next );
            return;
        }
        for( auto &ref : m_shaderAuthoring->textureRefs )
        {
            if( ref.paramName == paramName )
            {
                if( ref.texturePath != texturePath )
                {
                    ref.texturePath = texturePath;
                }
                return;
            }
        }
        m_shaderAuthoring->textureRefs.push_back( { paramName, texturePath } );
        m_shaderAuthoring->bindingLayoutId = 0;
    }

    String Material::getShaderTexture( const String &paramName ) const
    {
        if( !m_shaderAuthoring )
        {
            return String();
        }
        for( auto const &ref : m_shaderAuthoring->textureRefs )
        {
            if( ref.paramName == paramName )
            {
                return ref.texturePath;
            }
        }
        return String();
    }

    void Material::applyDataDrivenTextures()
    {
        if( !m_shaderAuthoring || !isDataDriven() || !m_shaderParameters.IsValid() ||
            m_shaderAuthoring->textureRefs.empty() )
        {
            return;
        }
        const auto &layout = m_shaderParameters.GetLayout();
        if( m_shaderAuthoring->bindingLayoutId != layout->GetId() )
        {
            SourceBudgetArray<ShaderTextureBinding> bindings;
            u32 layer = 0;
            for( const auto &parameter : layout->GetParameterInfo() )
            {
                if( parameter.m_type != MaterialShaderParameterType::Texture )
                {
                    continue;
                }
                for( u32 reference = 0; reference < m_shaderAuthoring->textureRefs.size(); ++reference )
                {
                    if( m_shaderAuthoring->textureRefs[reference].paramName == parameter.m_name )
                    {
                        bindings.push_back( { layer, reference } );
                        break;
                    }
                }
                ++layer;
            }
            // Commit after allocation succeeds; failed rebuilds retry next time.
            m_shaderAuthoring->textureBindings = std::move( bindings );
            m_shaderAuthoring->bindingLayoutId = layout->GetId();
        }
        // Preserve the existing backend's ordinal mapping and resource lifecycle.
        // Paths are borrowed, avoiding the former temporary String per slot.
        for( const auto &binding : m_shaderAuthoring->textureBindings )
        {
            const auto &path = m_shaderAuthoring->textureRefs[binding.reference].texturePath;
            if( !path.empty() )
            {
                setTexture( path, binding.layer );
            }
        }
    }

    struct MaterialShaderParametersInstance::Migration::Data
    {
        StreamBudget budget = reserveSourceBudgetBytes( sizeof( Data ) + sizeof( Migration ) );
        MaterialShaderParametersInstance *target = nullptr;
        MaterialShaderParametersInstance values;
        MaterialShaderParameterLayout::Ptr expectedLayout;
        u64 expectedRevision = 0, graphRevision = 0;
        Material *owner = nullptr;
        Material::ShaderWorkflow workflow = Material::ShaderWorkflow::Legacy;
        SourceBudgetString shaderID;
        bool hadAuthoring = false, dirty = false, committed = false;
        u8 pending = 0;
        bool hadFallback = false, replaceFallback = false;
        Material::LegacySurfaceData beforeFallback;
        SourceBudgetUnique<Material::LegacySurfaceData> fallback;

        struct Pass
        {
            SmartPtr<IState> source;
            SmartPtr<MaterialPassStateData> state;
            MaterialRenderState::Ptr before, after;
            std::array<u32, 66> beforeNumeric{};  // 64 presence bits plus the mask.
            size_t numericCount = 0;
            bool replaceNumeric = false;
            SourceBudgetUniqueArray<u32> numeric;
        };

        SourceBudgetArray<Pass> passes;
    };

    MaterialShaderParametersInstance::Migration::Migration() : m_data( new Data )
    {
    }

    MaterialShaderParametersInstance::Migration::~Migration() = default;

    const MaterialShaderParametersInstance &MaterialShaderParametersInstance::Migration::Values() const
    {
        return m_data->values;
    }

    bool MaterialShaderParametersInstance::Migration::IsCurrent() const noexcept
    {
        const auto &data = *m_data;
        if( data.committed || !data.target || data.target->m_owner != data.owner ||
            data.target->GetLayout() != data.expectedLayout ||
            data.target->GetRevision() != data.expectedRevision )
        {
            return false;
        }
        const auto sameFallback = []( const Material::LegacySurfaceData &a,
                                      const Material::LegacySurfaceData &b ) {
            return a.mask == b.mask && std::memcmp( &a.opacity, &b.opacity, sizeof( f32 ) ) == 0 &&
                   std::memcmp( &a.metalness, &b.metalness, sizeof( f32 ) ) == 0 &&
                   std::memcmp( &a.roughness, &b.roughness, sizeof( f32 ) ) == 0;
        };
        if( data.owner )
        {
            const auto &owner = *data.owner;
            if( owner.m_graphRevision.load( std::memory_order_acquire ) != data.graphRevision ||
                owner.m_shaderWorkflow != data.workflow || owner.m_shaderParameterEditDepth ||
                owner.m_batchActive || owner.m_synchronizingShaderParameters ||
                MaterialPublicationScope::isActive() || owner.m_pendingShaderSemantics != data.pending ||
                owner.m_shaderParametersDirty != data.dirty ||
                bool( owner.m_shaderAuthoring ) != data.hadAuthoring ||
                ( owner.m_shaderAuthoring && owner.m_shaderAuthoring->shaderID != data.shaderID ) ||
                bool( owner.m_legacySurface ) != data.hadFallback ||
                ( owner.m_legacySurface &&
                  !sameFallback( *owner.m_legacySurface, data.beforeFallback ) ) )
            {
                return false;
            }
        }
        for( const auto &pass : data.passes )
        {
            if( pass.source->getData().get() != pass.state.get() || pass.state->m_writeDepth ||
                pass.state->m_pendingSurface || pass.state->getRenderState() != pass.before ||
                pass.state->getNumericAuthoringStorageBytes() / sizeof( u32 ) != pass.numericCount ||
                ( pass.numericCount &&
                  std::memcmp( pass.state->m_numericAuthoring.get(), pass.beforeNumeric.data(),
                               pass.numericCount * sizeof( u32 ) ) != 0 ) )
            {
                return false;
            }
        }
        return true;
    }

    bool MaterialShaderParametersInstance::Migration::ConflictsWith(
        const Migration &other ) const noexcept
    {
        if( this == &other || m_data->committed || other.m_data->committed )
        {
            return false;
        }
        const auto &first = *m_data;
        const auto &second = *other.m_data;
        if( first.target == second.target || ( first.owner && first.owner == second.owner ) )
        {
            return true;
        }
        for( const auto &a : first.passes )
        {
            for( const auto &b : second.passes )
            {
                if( a.state == b.state || a.source == b.source )
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool MaterialShaderParametersInstance::Migration::Commit() noexcept
    {
        if( !IsCurrent() )
        {
            return false;
        }
        auto &data = *m_data;
        data.target->m_layout.swap( data.values.m_layout );
        data.target->m_parametersMemory.swap( data.values.m_parametersMemory );
        data.target->m_revision = data.expectedRevision + 1;
        if( data.owner )
        {
            if( data.replaceFallback )
            {
                data.owner->m_legacySurface.swap( data.fallback );
            }
            data.owner->m_pendingShaderSemantics =
                data.passes.empty()
                    ? static_cast<u8>( ( 1u << static_cast<u32>( MaterialBuiltinParameter::Count ) ) -
                                       1u )
                    : 0;
            data.owner->m_shaderParametersDirty = data.passes.empty();
        }
        for( auto &pass : data.passes )
        {
            // Existing authoring readers borrow the resting surface/numeric
            // data under the state lock. Keep that protection even though the
            // replacement itself was allocated before this commit boundary.
            pass.state->StateData::lock();
            if( pass.replaceNumeric )
            {
                pass.state->m_numericAuthoring.swap( pass.numeric );
            }
            std::atomic_store( &pass.state->m_renderState, pass.after );
            pass.state->StateData::unlock();
        }
        data.committed = true;
        return true;
    }

    bool MaterialShaderParametersInstance::IsMigrationReady() const noexcept
    {
        if( !m_owner )
        {
            return true;
        }
        if( m_owner->m_shaderParameterEditDepth || m_owner->m_batchActive ||
            m_owner->m_synchronizingShaderParameters || MaterialPublicationScope::isActive() )
        {
            return false;
        }
        try
        {
            // Indexed reads avoid the allocated getTechniques/getPasses snapshots
            // used by the actual staging walk. All graph writes are owner-thread.
            for( u32 i = 0; i < m_owner->m_techniques.size(); ++i )
            {
                const auto technique = m_owner->m_techniques[i];
                if( !technique )
                {
                    continue;
                }
                for( u32 j = 0; j < technique->getNumPasses(); ++j )
                {
                    const auto pass = technique->getPass( j );
                    if( !pass )
                    {
                        continue;
                    }
                    if( pass->getMaterial().get() != m_owner )
                    {
                        return false;
                    }
                    const auto context = pass->getStateContext();
                    const auto source = context ? context->getStateById( pass->getId() ) : nullptr;
                    auto data = source ? source->getData() : nullptr;
                    if( !data )
                    {
                        continue;
                    }
                    if( !data->isDerived<MaterialPassStateData>() )
                    {
                        return false;
                    }
                    const auto state = static_pointer_cast<MaterialPassStateData>( data );
                    if( state->m_writeDepth || state->m_pendingSurface )
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        catch( ... )
        {
            return false;
        }
    }

    MaterialShaderParametersInstance::MigrationPtr MaterialShaderParametersInstance::PrepareMigration(
        MaterialShaderParameterLayout::Ptr layout )
    {
        if( !layout )
        {
            throw std::invalid_argument( "Shader migration requires a valid replacement layout" );
        }
        if( m_revision == std::numeric_limits<u64>::max() )
        {
            throw std::overflow_error( "Shader parameter revision exhausted" );
        }
        if( !IsMigrationReady() )
        {
            throw std::runtime_error( "End material edit/publication scopes before shader migration" );
        }
        MigrationPtr result( new Migration );
        auto &data = *result->m_data;
        data.target = this;
        data.owner = m_owner;
        data.expectedLayout = m_layout;
        data.expectedRevision = m_revision;
        data.values.Reset( layout );
        const auto &memory = GetParameterMemory();
        for( u32 i = 0; i < layout->GetParameterInfo().size(); ++i )
        {
            const auto &info = layout->GetParameterInfo()[i];
            const auto previous =
                m_layout ? m_layout->FindParameter( info.m_name ) : MaterialShaderParameterHandle{};
            if( previous.IsValid() &&
                GetParameterInfo()[previous.m_parameterIndex].m_type == info.m_type )
            {
                data.values.WriteBytes( layout->GetParameter( i ),
                                        memory.data() + previous.m_offsetInBytes, info.m_strideInBytes,
                                        info.m_type );
            }
        }
        if( m_owner )
        {
            auto &owner = *m_owner;
            data.workflow = owner.m_shaderWorkflow;
            data.graphRevision = owner.m_graphRevision.load( std::memory_order_acquire );
            data.pending = owner.m_pendingShaderSemantics;
            data.dirty = owner.m_shaderParametersDirty;
            data.hadAuthoring = bool( owner.m_shaderAuthoring );
            if( owner.m_shaderAuthoring )
            {
                data.shaderID = owner.m_shaderAuthoring->shaderID;
            }
            data.hadFallback = bool( owner.m_legacySurface );
            if( owner.m_legacySurface )
            {
                data.beforeFallback = *owner.m_legacySurface;
            }
            const bool active = data.workflow != Material::ShaderWorkflow::Legacy;
            const bool pbr = data.workflow == Material::ShaderWorkflow::Pbr;
            const bool colorOnly = data.workflow == Material::ShaderWorkflow::ColorOnly;
            const auto valid = []( const MaterialShaderParametersInstance &values,
                                   MaterialBuiltinParameter semantic, MaterialShaderParameterType type,
                                   u32 bytes ) {
                return values.IsHandleValid( values.GetBuiltin( semantic ), type, bytes );
            };
            const bool nextOpacity = valid( data.values, MaterialBuiltinParameter::Opacity,
                                            MaterialShaderParameterType::Scalar, sizeof( f32 ) );
            const bool oldOpacity = valid( *this, MaterialBuiltinParameter::Opacity,
                                           MaterialShaderParameterType::Scalar, sizeof( f32 ) );
            f32 opacity = 1;
            bool opacityTransfer = false;
            if( active && nextOpacity != oldOpacity &&
                shaderOwnsSurfaceScalar( layout, active, pbr, MaterialBuiltinParameter::Opacity ) &&
                readShaderSurfaceScalar( *this, pbr, MaterialBuiltinParameter::Opacity, opacity ) )
            {
                opacityTransfer = true;
                if( nextOpacity )
                {
                    data.values.SetScalar( data.values.GetBuiltin( MaterialBuiltinParameter::Opacity ),
                                           opacity );
                }
                else
                {
                    auto colour = data.values.GetColour(
                        data.values.GetBuiltin( MaterialBuiltinParameter::Albedo ) );
                    colour.a = opacity;
                    data.values.SetColour( data.values.GetBuiltin( MaterialBuiltinParameter::Albedo ),
                                           colour );
                }
            }
            auto fallback = data.beforeFallback;
            if( auto handoff = owner.prepareShaderSurfaceHandoff( layout ) )
            {
                fallback = *handoff;
            }
            for( const auto semantic :
                 { MaterialBuiltinParameter::Metalness, MaterialBuiltinParameter::Roughness,
                   MaterialBuiltinParameter::Opacity } )
            {
                if( shaderOwnsSurfaceScalar( layout, active, pbr, semantic ) )
                {
                    fallback.mask &= static_cast<u8>( ~legacySurfaceMask( semantic ) );
                }
            }
            const auto equal = []( const Material::LegacySurfaceData &a,
                                   const Material::LegacySurfaceData &b ) {
                return a.mask == b.mask && std::memcmp( &a.opacity, &b.opacity, sizeof( f32 ) ) == 0 &&
                       std::memcmp( &a.metalness, &b.metalness, sizeof( f32 ) ) == 0 &&
                       std::memcmp( &a.roughness, &b.roughness, sizeof( f32 ) ) == 0;
            };
            data.replaceFallback = bool( fallback.mask ) != data.hadFallback ||
                                   ( fallback.mask && !equal( fallback, data.beforeFallback ) );
            if( data.replaceFallback && fallback.mask )
            {
                data.fallback = makeSourceBudgetUnique<Material::LegacySurfaceData>( fallback );
            }
            const auto changed = [&]( MaterialBuiltinParameter semantic,
                                      MaterialShaderParameterType type, u32 bytes ) {
                const auto next = data.values.GetBuiltin( semantic );
                if( !data.values.IsHandleValid( next, type, bytes ) )
                {
                    return false;
                }
                const auto old = GetBuiltin( semantic );
                return ( data.pending & ( 1u << static_cast<u32>( semantic ) ) ) ||
                       !IsHandleValid( old, type, bytes ) ||
                       std::memcmp( GetParameterMemory().data() + old.m_offsetInBytes,
                                    data.values.GetParameterMemory().data() + next.m_offsetInBytes,
                                    bytes ) != 0;
            };
            for( auto technique : owner.getTechniques() )
            {
                if( technique )
                {
                    for( auto pass : technique->getPasses() )
                    {
                        if( pass )
                        {
                            const auto context = pass->getStateContext();
                            const auto source =
                                context ? context->getStateById( pass->getId() ) : nullptr;
                            auto raw = source ? source->getData() : nullptr;
                            if( !raw )
                            {
                                continue;  // Unloaded passes seed from the committed owner later.
                            }
                            if( !raw->isDerived<MaterialPassStateData>() )
                            {
                                throw std::runtime_error(
                                    "Shader migration requires immutable material pass state" );
                            }
                            const auto state = static_pointer_cast<MaterialPassStateData>( raw );
                            if( std::find_if( data.passes.begin(), data.passes.end(),
                                              [&]( const Migration::Data::Pass &item ) {
                                                  return item.state == state;
                                              } ) != data.passes.end() )
                            {
                                continue;
                            }

                            struct Lock
                            {
                                MaterialPassStateData &state;

                                explicit Lock( MaterialPassStateData &value ) : state( value )
                                {
                                    state.StateData::lock();
                                }

                                ~Lock()
                                {
                                    state.StateData::unlock();
                                }
                            } lock{ *state };

                            if( state->m_writeDepth || state->m_pendingSurface )
                            {
                                throw std::runtime_error( "End pass writes before shader migration" );
                            }
                            Migration::Data::Pass staged;
                            staged.source = source;
                            staged.state = state;
                            staged.before = state->getRenderState();
                            staged.numericCount =
                                state->getNumericAuthoringStorageBytes() / sizeof( u32 );
                            if( staged.numericCount > staged.beforeNumeric.size() )
                            {
                                throw std::logic_error( "Material numeric mask capacity changed" );
                            }
                            if( staged.numericCount )
                            {
                                std::memcpy( staged.beforeNumeric.data(),
                                             state->m_numericAuthoring.get(),
                                             staged.numericCount * sizeof( u32 ) );
                            }
                            auto surface = *staged.before;
                            bool surfaceChanged = false;
                            const auto assign = [&]( auto &destination, const auto &value ) {
                                if( std::memcmp( &destination, &value, sizeof( value ) ) != 0 )
                                {
                                    destination = value;
                                    surfaceChanged = true;
                                }
                            };
                            if( active )
                            {
                                const auto scalar = [&]( MaterialBuiltinParameter semantic,
                                                         f32 &destination ) {
                                    if( changed( semantic, MaterialShaderParameterType::Scalar,
                                                 sizeof( f32 ) ) )
                                    {
                                        assign( destination, data.values.GetScalar(
                                                                 data.values.GetBuiltin( semantic ) ) );
                                    }
                                };
                                const auto colour = [&]( MaterialBuiltinParameter semantic,
                                                         ColourF &destination ) {
                                    if( changed( semantic, MaterialShaderParameterType::Colour,
                                                 sizeof( ColourF ) ) )
                                    {
                                        assign( destination, data.values.GetColour(
                                                                 data.values.GetBuiltin( semantic ) ) );
                                        return true;
                                    }
                                    return false;
                                };
                                if( pbr )
                                {
                                    scalar( MaterialBuiltinParameter::Metalness, surface.metalness );
                                    scalar( MaterialBuiltinParameter::Roughness, surface.roughness );
                                    scalar( MaterialBuiltinParameter::AlphaClip, surface.alphaClip );
                                }
                                // Moving opacity between a scalar and Albedo alpha is
                                // an alpha-only handoff, preserving each pass's local RGB.
                                const bool alphaOnlyTransfer =
                                    pbr && opacityTransfer && !nextOpacity &&
                                    valid( *this, MaterialBuiltinParameter::Albedo,
                                           MaterialShaderParameterType::Colour, sizeof( ColourF ) ) &&
                                    valid( data.values, MaterialBuiltinParameter::Albedo,
                                           MaterialShaderParameterType::Colour, sizeof( ColourF ) ) &&
                                    !( data.pending &
                                       ( 1u << static_cast<u32>( MaterialBuiltinParameter::Albedo ) ) );
                                const bool diffuse =
                                    !alphaOnlyTransfer && ( colorOnly || pbr ) &&
                                    colour( colorOnly ? MaterialBuiltinParameter::Color
                                                      : MaterialBuiltinParameter::Albedo,
                                            surface.diffuseColour );
                                if( colorOnly && diffuse )
                                {
                                    assign( surface.emissiveColour, surface.diffuseColour );
                                }
                                else if( !colorOnly )
                                {
                                    colour( MaterialBuiltinParameter::Emissive, surface.emissiveColour );
                                }
                                const bool opacityChanged =
                                    diffuse || opacityTransfer ||
                                    changed( MaterialBuiltinParameter::Opacity,
                                             MaterialShaderParameterType::Scalar, sizeof( f32 ) ) ||
                                    ( pbr && ( data.pending & shaderAlphaOnly ) &&
                                      valid( data.values, MaterialBuiltinParameter::Albedo,
                                             MaterialShaderParameterType::Colour, sizeof( ColourF ) ) );
                                if( opacityChanged )
                                {
                                    auto value = surface.opacity;
                                    if( nextOpacity )
                                    {
                                        value = data.values.GetScalar( data.values.GetBuiltin(
                                            MaterialBuiltinParameter::Opacity ) );
                                    }
                                    else if( pbr && valid( data.values, MaterialBuiltinParameter::Albedo,
                                                           MaterialShaderParameterType::Colour,
                                                           sizeof( ColourF ) ) )
                                    {
                                        value = data.values
                                                    .GetColour( data.values.GetBuiltin(
                                                        MaterialBuiltinParameter::Albedo ) )
                                                    .a;
                                    }
                                    value = Math<f32>::clamp( value, 0.0f, 1.0f );
                                    assign( surface.opacity, value );
                                    auto diffuseColour = surface.diffuseColour;
                                    diffuseColour.a = value;
                                    assign( surface.diffuseColour, diffuseColour );
                                    if( value < .999f )
                                    {
                                        assign( surface.flags, surface.flags | transparentFlag );
                                        u32 renderMode = 0;
                                        state->getUIntSetting( MaterialUIntSetting::renderMode,
                                                               renderMode );
                                        if( renderMode < 2u )
                                        {
                                            staged.numeric = state->prepareShaderRenderMode( 3u );
                                            staged.replaceNumeric = true;
                                            assign( surface.flags, surface.flags & ~cutoutFlag );
                                            assign( surface.blendMode, 1u );
                                        }
                                    }
                                }
                            }
                            if( surfaceChanged )
                            {
                                if( staged.before->revision == std::numeric_limits<u64>::max() )
                                {
                                    throw std::overflow_error( "Material surface revision exhausted" );
                                }
                                auto next = std::allocate_shared<MaterialRenderState>(
                                    SourceBudgetAllocator<MaterialRenderState>{}, surface );
                                next->revision = staged.before->revision + 1;
                                staged.after = std::move( next );
                            }
                            else
                            {
                                staged.after = staged.before;
                            }
                            data.passes.push_back( std::move( staged ) );
                        }
                    }
                }
            }
        }
        data.values.m_revision = data.expectedRevision + 1;
        return result;
    }

    void Material::setShaderParameters( const Array<MaterialShaderParameterInfo> &parameterInfo,
                                        const String &shaderID /*= String() */ )
    {
        const auto candidateId = shaderID.empty() ? getShaderID() : shaderID;
        auto shader = MaterialShaderRegistry::instance().find( candidateId );
        auto layout = shader ? shader->getLayout() : MaterialShaderParameterLayout::Ptr{};
        const auto matches = [&]( const MaterialShaderParameterLayout::Ptr &candidate ) {
            if( !candidate || candidate->GetParameterInfo().size() != parameterInfo.size() )
            {
                return false;
            }
            for( size_t i = 0; i < parameterInfo.size(); ++i )
            {
                const auto &existing = candidate->GetParameterInfo()[i];
                if( existing.m_name != parameterInfo[i].m_name ||
                    existing.m_type != parameterInfo[i].m_type )
                {
                    return false;
                }
            }
            return true;
        };
        // Prefer the registry's published identity, including a new generation
        // with identical fields. Explicit custom schemas can reuse their current
        // finalized layout rather than rebuilding its names/defaults each time.
        if( !matches( layout ) )
        {
            layout = matches( m_shaderParameters.GetLayout() )
                         ? m_shaderParameters.GetLayout()
                         : MaterialShaderParameterLayout::Create( parameterInfo );
        }
        if( !layout )
        {
            WP_LOG_WARNING( "Material rejected an invalid shader parameter schema." );
            return;  // Keep the last valid shader/layout and values.
        }
        if( layout == m_shaderParameters.GetLayout() )
        {
            if( !shaderID.empty() )
            {
                setShaderID( shaderID );
            }
            m_shaderParametersDirty = true;
            return;  // Retain value sharing, revision, borrowed bytes and observers.
        }
        f32 migratedOpacity = 1.0f;
        bool migrateOpacity = false;
        // A removed/inactive owner must hand its last value back before Reset
        // discards the slot. Future passes still use the legacy fallback there.
        if( m_shaderWorkflow != ShaderWorkflow::Legacy )
        {
            const bool nextActive = !candidateId.empty();
            const bool nextPbr =
                nextActive && candidateId != unlitShaderIDStr && candidateId != colorOnlyShaderIDStr;
            // Read the authored owner, including writes inside an open edit;
            // a primary pass can contain an independent local override.
            commitShaderSurfaceHandoff( prepareShaderSurfaceHandoff( layout, nextActive, nextPbr ) );
            // Opacity can change its representation between a scalar slot and
            // PBR Albedo alpha. Name-based schema copies cannot transfer that
            // value when the old representation disappears or a scalar is added.
            const auto nextOpacity = layout->GetBuiltin( MaterialBuiltinParameter::Opacity );
            const bool nextScalar = nextOpacity.IsValid() &&
                                    layout->GetParameterInfo()[nextOpacity.m_parameterIndex].m_type ==
                                        MaterialShaderParameterType::Scalar;
            const bool previousScalar = m_shaderParameters.IsHandleValid(
                m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Opacity ),
                MaterialShaderParameterType::Scalar, sizeof( f32 ) );
            migrateOpacity =
                previousScalar != nextScalar &&
                shaderOwnsSurfaceScalar( layout, nextActive, nextPbr,
                                         MaterialBuiltinParameter::Opacity ) &&
                readShaderSurfaceScalar( m_shaderParameters, m_shaderWorkflow == ShaderWorkflow::Pbr,
                                         MaterialBuiltinParameter::Opacity, migratedOpacity );
        }
        MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );
        auto edit = m_shaderParameters.Edit();
        auto previous = std::move( m_shaderParameters );
        m_shaderParameters.Reset( std::move( layout ) );
        if( !shaderID.empty() )
        {
            setShaderID( shaderID );
        }
        // Layout replacement is an authoring/load operation. Resolve names here only;
        // compatible values survive reordered/extended schemas and partial updates.
        for( u32 i = 0; i < m_shaderParameters.GetParameterInfo().size(); ++i )
        {
            const auto &info = m_shaderParameters.GetParameterInfo()[i];
            const auto old = previous.FindParameter( info.m_name );
            if( !old.IsValid() ||
                previous.GetParameterInfo()[old.m_parameterIndex].m_type != info.m_type )
            {
                continue;
            }
            const auto next = m_shaderParameters.GetParameter( i );
            switch( info.m_type )
            {
            case MaterialShaderParameterType::Scalar:
                m_shaderParameters.SetScalar( next, previous.GetScalar( old ) );
                break;
            case MaterialShaderParameterType::Int:
                m_shaderParameters.SetScalar( next, previous.GetInt( old ) );
                break;
            case MaterialShaderParameterType::UInt:
                m_shaderParameters.SetScalar( next, previous.GetUInt( old ) );
                break;
            case MaterialShaderParameterType::Bool:
                m_shaderParameters.SetBool( next, previous.GetBool( old ) );
                break;
            case MaterialShaderParameterType::Colour:
                m_shaderParameters.SetColour( next, previous.GetColour( old ) );
                break;
            case MaterialShaderParameterType::Vector2:
                m_shaderParameters.SetVector2( next, previous.GetVector2f( old ) );
                break;
            case MaterialShaderParameterType::Vector4:
                m_shaderParameters.SetVector4( next, previous.GetVector4f( old ) );
                break;
            case MaterialShaderParameterType::IntVector2:
            {
                Vector2<s32> value;
                previous.Get( MaterialParameterHandle<Vector2<s32>>{ old }, value );
                m_shaderParameters.SetVector2( next, value );
                break;
            }
            case MaterialShaderParameterType::IntVector4:
            {
                Vector4<s32> value;
                previous.Get( MaterialParameterHandle<Vector4<s32>>{ old }, value );
                m_shaderParameters.SetVector4( next, value );
                break;
            }
            case MaterialShaderParameterType::Matrix:
                m_shaderParameters.SetMatrix( next, previous.GetMatrix( old ) );
                break;
            case MaterialShaderParameterType::Texture:
                m_shaderParameters.SetTexture( next, previous.GetResource( old ) );
                break;
            case MaterialShaderParameterType::Buffer:
                m_shaderParameters.SetBuffer( next, previous.GetResource( old ) );
                break;
            case MaterialShaderParameterType::Sampler:
                m_shaderParameters.SetSampler( next, previous.GetResource( old ) );
                break;
            }
        }
        if( migrateOpacity )
        {
            const auto opacity = m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Opacity );
            if( m_shaderParameters.IsHandleValid( opacity, MaterialShaderParameterType::Scalar,
                                                  sizeof( f32 ) ) )
            {
                m_shaderParameters.SetScalar( opacity, migratedOpacity );
            }
            else
            {
                const auto albedo = m_shaderParameters.GetBuiltin( MaterialBuiltinParameter::Albedo );
                auto colour = m_shaderParameters.GetColour( albedo );
                colour.a = migratedOpacity;
                m_shaderParameters.SetColour( albedo, colour );
            }
        }
        // Preserve queued edits only where the replacement still represents
        // them. In particular, an old Albedo-alpha edit must not read a missing
        // new slot and overwrite the fallback handed off above.
        m_pendingShaderSemantics = compatibleShaderEditMask(
            m_shaderParameters, m_shaderWorkflow != ShaderWorkflow::Legacy,
            m_shaderWorkflow == ShaderWorkflow::Pbr, m_shaderWorkflow == ShaderWorkflow::ColorOnly,
            m_pendingShaderSemantics );
        m_shaderParametersDirty = true;
    }

    const MaterialShaderParametersInstance &Material::getShaderParameters() const
    {
        return m_shaderParameters;
    }

    MaterialShaderParametersInstance &Material::getShaderParameters()
    {
        return m_shaderParameters;
    }

    void Material::setShaderID( const String &shaderID )
    {
        const auto previousWorkflow = m_shaderWorkflow;
        const bool idChanged =
            m_shaderAuthoring ? m_shaderAuthoring->shaderID != shaderID : !shaderID.empty();
        SourceBudgetString nextID;
        if( idChanged )
        {
            nextID = shaderID;
        }
        SourceBudgetUnique<ShaderAuthoringData> next;
        if( !shaderID.empty() && !m_shaderAuthoring )
        {
            next = makeSourceBudgetUnique<ShaderAuthoringData>();
        }
        auto handoff =
            !m_synchronizingShaderParameters && m_shaderWorkflow != ShaderWorkflow::Legacy
                ? prepareShaderSurfaceHandoff( m_shaderParameters.GetLayout(), !shaderID.empty(),
                                               !shaderID.empty() && shaderID != unlitShaderIDStr &&
                                                   shaderID != colorOnlyShaderIDStr )
                : nullptr;
        if( next )
        {
            m_shaderAuthoring = std::move( next );
        }
        if( m_shaderAuthoring && idChanged )
        {
            m_shaderAuthoring->shaderID.swap( nextID );
            ++m_shaderAuthoring->shaderIDRevision;
        }
        commitShaderSurfaceHandoff( std::move( handoff ) );
        m_shaderWorkflow = shaderID.empty()                   ? ShaderWorkflow::Legacy
                           : shaderID == unlitShaderIDStr     ? ShaderWorkflow::Unlit
                           : shaderID == colorOnlyShaderIDStr ? ShaderWorkflow::ColorOnly
                                                              : ShaderWorkflow::Pbr;
        if( previousWorkflow != m_shaderWorkflow )
        {
            m_pendingShaderSemantics = compatibleShaderEditMask(
                m_shaderParameters, m_shaderWorkflow != ShaderWorkflow::Legacy,
                m_shaderWorkflow == ShaderWorkflow::Pbr, m_shaderWorkflow == ShaderWorkflow::ColorOnly,
                m_pendingShaderSemantics );
        }
        m_shaderParametersDirty = true;
    }

    String Material::getShaderID() const
    {
        return m_shaderAuthoring ? String( m_shaderAuthoring->shaderID ) : String();
    }

    u64 Material::getShaderIDRevision() const noexcept
    {
        return m_shaderAuthoring ? m_shaderAuthoring->shaderIDRevision : 0;
    }

    bool Material::isDataDriven() const
    {
        return m_shaderWorkflow != ShaderWorkflow::Legacy;
    }

    void Material::setPass( SmartPtr<IMaterialPass> pass )
    {
        m_pass = pass;
        invalidateRenderState();
    }

    void Material::invalidateRenderState()
    {
        m_graphRevision.fetch_add( 1, std::memory_order_acq_rel );
        std::atomic_store( &m_primaryPassSelection, std::shared_ptr<const PrimaryPassSelection>{} );
    }

    MaterialRenderState::Ptr Material::getRenderState() const
    {
        const auto revision = m_graphRevision.load( std::memory_order_acquire );
        auto selection = std::atomic_load( &m_primaryPassSelection );
        if( !selection || selection->graphRevision != revision )
        {
            auto next = std::allocate_shared<PrimaryPassSelection>(
                SourceBudgetAllocator<PrimaryPassSelection>{} );
            next->graphRevision = revision;
            for( const auto &technique : getTechniques() )
            {
                if( !technique )
                {
                    continue;
                }
                for( const auto &pass : technique->getPasses() )
                {
                    auto context = pass ? pass->getStateContext() : nullptr;
                    auto source = context ? context->getStateById( pass->getId() ) : nullptr;
                    auto data = source ? source->getData() : nullptr;
                    if( data && data->isDerived<MaterialPassStateData>() )
                    {
                        next->state = source;
                        break;
                    }
                }
                if( next->state )
                {
                    break;
                }
            }
            selection = next;
            // An overlapping graph edit may render the retained old version once,
            // but must never publish it as the new graph's selection.
            if( m_graphRevision.load( std::memory_order_acquire ) == revision )
            {
                std::atomic_store( &m_primaryPassSelection, selection );
            }
        }
        auto data = selection->state ? selection->state->getData() : nullptr;
        return data && data->isDerived<MaterialPassStateData>()
                   ? static_pointer_cast<MaterialPassStateData>( data )->getRenderState()
                   : nullptr;
    }

    SmartPtr<IMaterialPass> Material::getPass() const
    {
        return m_pass;
    }

}  // namespace workphone::render
