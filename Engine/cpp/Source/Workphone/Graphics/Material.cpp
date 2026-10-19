#include <Workphone/WorkphonePCH.hpp>
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
#include <cmath>

namespace workphone::render
{

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
        String( ".m12" ), String( ".m13" ), String( ".m14" ), String( ".m15" ) };
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
        struct MaterialParameterSyncScope
        {
            bool &flag;
            bool previous;
            explicit MaterialParameterSyncScope( bool &value ) : flag( value ), previous( value )
            { flag = true; }
            ~MaterialParameterSyncScope() { flag = previous; }
        };

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
        // Seed MaterialStateData so property accessors work for callers that construct
        // a stand-alone Material without explicit setup. createStateObject() already
        // registers the state context with MaterialStateData.
        if( !getStateContext() )
        {
            createStateObject();
        }

        auto filePath = getFilePath();
        if( !StringUtil::isNullOrEmpty( filePath ) )
        {
            loadFromFile( filePath );
        }
        shaderParametersChanged( 0 ); // Publish edits made before the first pass existed.
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

                for( auto technique : m_techniques )
                {
                    technique->unload( data );
                }

                m_techniques.clear();

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
                        return Array<SmartPtr<ITexture>>( state->textures.begin(),
                                                          state->textures.end() );
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
                        if( layerIdx < state->textures.size() )
                        {
                            return state->textures[layerIdx];
                        }
                        // Custom editor slots (AO, opacity, masks, etc.) are stored
                        // in texture units beyond Ogre's fixed PBS state array.
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
        m_techniques.erase( std::remove( m_techniques.begin(), m_techniques.end(), technique ),
                            m_techniques.end() );
        invalidateRenderState();
    }

    void Material::setTechniques( const Array<SmartPtr<IMaterialTechnique>> &techniques )
    {
        invalidateRenderState();
        m_techniques = { techniques.begin(), techniques.end() };
        for( auto technique : techniques ) if( technique ) technique->setMaterial( this );
        invalidateRenderState();
    }

    void Material::removeAllTechniques()
    {
        invalidateRenderState();
        m_techniques.clear();
        invalidateRenderState();
    }

    SmartPtr<IMaterialTechnique> Material::getTechnique( u32 index ) const
    {
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
        m_hasPendingMetalness = true;
        m_pendingMetalness = metalness;
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
        m_hasPendingRoughness = true;
        m_pendingRoughness = roughness;
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
                    if( !p->isLoaded() )
                    {
                        p->setMaterial( this );
                        p->load( nullptr );
                    }
                }
            }
            // Pass constructors may initialize state before the material owner is
            // attached. Apply pre-load values after the complete graph is ready.
            if( m_hasPendingRoughness ) setRoughness( m_pendingRoughness );
            if( m_hasPendingMetalness ) setMetalness( m_pendingMetalness );
            if( m_hasPendingOpacity ) setOpacity( m_pendingOpacity );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Material::addTechnique( SmartPtr<IMaterialTechnique> technique )
    {
        invalidateRenderState();
        if( technique ) technique->setMaterial( this );
        m_techniques.push_back( technique );
        invalidateRenderState();
        if( m_hasPendingRoughness )
        {
            setRoughness( m_pendingRoughness );
        }
        if( m_hasPendingMetalness )
        {
            setMetalness( m_pendingMetalness );
        }
        if( m_hasPendingOpacity )
        {
            setOpacity( m_pendingOpacity );
        }
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
                        return state->materialType;
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
                        state->materialType = materialType;
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
                        auto sUUID = textureChild->getProperty( uuidStr );
                        if( !StringUtil::isNullOrEmpty( sUUID ) )
                        {
                            auto uuid = StringUtil::parseUUID( sUUID );
                            auto tex = resourceDatabase->loadResourceById( uuid );
                            if( textureCount < textures.size() )
                            {
                                textures[textureCount] = tex;
                            }
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

            // Data-driven shader (Esoterica integration, step 5): persist the bound shader id
            // and each editable scalar/colour parameter so the choice survives save/reload.
            properties->setProperty( shaderIDStr, m_shaderID );
            if( isDataDriven() && m_shaderParameters.IsValid() )
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
                        properties->setProperty( key, m_shaderParameters.GetInt( handle ) );
                    else if( p.m_type == MaterialShaderParameterType::UInt )
                        properties->setProperty( key, m_shaderParameters.GetUInt( handle ) );
                    else if( p.m_type == MaterialShaderParameterType::Bool )
                        properties->setProperty( key, m_shaderParameters.GetBool( handle ) );
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

            // Data-driven shader (step 5): rebind the saved shader and restore scalar/colour params.
            String savedShaderID;
            if( properties->getPropertyValue( shaderIDStr, savedShaderID ) && !savedShaderID.empty() )
            {
                auto shader = MaterialShaderRegistry::instance().find( savedShaderID );
                if( shader )
                {
                    setShaderParameters( shader->getParameterInfo(), shader->getShaderID() );
                    if( shader->getShaderID() == defaultPBRShaderIDStr )
                    {
                        syncPBRToShaderParameters();
                    }
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
                                m_shaderParameters.SetScalar( handle, value );
                        }
                        else if( p.m_type == MaterialShaderParameterType::UInt )
                        {
                            u32 value = 0;
                            if( properties->getPropertyValue( key, value ) )
                                m_shaderParameters.SetScalar( handle, value );
                        }
                        else if( p.m_type == MaterialShaderParameterType::Bool )
                        {
                            bool value = false;
                            if( properties->getPropertyValue( key, value ) )
                                m_shaderParameters.SetBool( handle, value );
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
                            Vector4F v;
                            v.x = properties->getPropertyAsFloat( key + shaderParamVectorXStr, 0.0f );
                            v.y = properties->getPropertyAsFloat( key + shaderParamVectorYStr, 0.0f );
                            v.z = properties->getPropertyAsFloat( key + shaderParamVectorZStr, 0.0f );
                            v.w = properties->getPropertyAsFloat( key + shaderParamVectorWStr, 0.0f );
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
                            if( properties->getPropertyValue( shaderTexturePrefixStr + p.m_name,
                                                              texPath ) )
                            {
                                setShaderTexture( p.m_name, texPath );
                            }
                        }
                        else if( p.m_type == MaterialShaderParameterType::Matrix )
                        {
                            Matrix4<f32> m;
                            f32 *p = m.ptr();
                            for( u32 i = 0; i < 16; ++i )
                            {
                                p[i] = properties->getPropertyAsFloat( key + shaderParamMatrixSuffixStr[i],
                                                                       0.0f );
                            }
                            m_shaderParameters.SetMatrix( handle, m );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
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
                        handled = state->setFloatSetting( setting, value ) || handled;
                    }
                }
            }
        }

        if( !handled )
        {
            WP_LOG_WARNING( String( "Material::setEditorFloat ignored unknown setting: " ) + name );
        }
    }

    f32 Material::getEditorFloat( const String &name, f32 defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveFloatSetting( name );
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
                        if( state->getFloatSetting( setting, value ) )
                        {
                            return value;
                        }
                    }
                }
            }
        }

        return defaultValue;
    }

    void Material::setEditorUInt( const String &name, u32 value )
    {
        const auto setting = MaterialPassStateData::resolveUIntSetting( name );
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
                        handled = state->setUIntSetting( setting, value ) || handled;
                    }
                }
            }
        }

        if( !handled )
        {
            WP_LOG_WARNING( String( "Material::setEditorUInt ignored unknown or invalid setting: " ) +
                            name );
        }
    }

    u32 Material::getEditorUInt( const String &name, u32 defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveUIntSetting( name );
        // Authored techniques can exist before a render technique is selected,
        // and a newly allocated resource can have none. Match the other editor
        // setting readers rather than dereferencing the selected-technique cache.
        for( auto technique : getTechniques() )
        {
            for( auto pass : technique->getPasses() )
            {
                if( auto stateContext = pass->getStateContext() )
                {
                    if( auto state = stateContext->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                    {
                        auto value = defaultValue;
                        if( state->getUIntSetting( setting, value ) )
                        {
                            return value;
                        }
                    }
                }
            }
        }

        return defaultValue;
    }

    void Material::setEditorBool( const String &name, bool value )
    {
        const auto setting = MaterialPassStateData::resolveBoolSetting( name );
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
                        handled = state->setBoolSetting( setting, value ) || handled;
                    }
                }
            }
        }

        if( !handled )
        {
            WP_LOG_WARNING( String( "Material::setEditorBool ignored unknown setting: " ) + name );
        }

        if( name == doubleSidedStr )
        {
            constexpr auto cullNone = 1u;
            constexpr auto cullClockwise = 2u;
            const auto mode = value ? cullNone : getEditorUInt( cullModeStr, cullClockwise );
            applyCullModeToPasses( this, mode );
        }
    }

    bool Material::getEditorBool( const String &name, bool defaultValue ) const
    {
        const auto setting = MaterialPassStateData::resolveBoolSetting( name );
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
                        if( state->getBoolSetting( setting, value ) )
                        {
                            return value;
                        }
                    }
                }
            }
        }

        return defaultValue;
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

        setEditorUInt( renderModeStr, mode );

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
    }

    u32 Material::getRenderMode() const
    {
        auto defaultMode = isTransparent() ? 3u : ( isCutout() ? 1u : 0u );
        return getEditorUInt( renderModeStr, defaultMode );
    }

    void Material::setWorkflow( u32 workflow )
    {
        // Keep this in sync with the material editor's PBR Workflow dropdown.
        setEditorUInt( workflowStr, workflow <= 3u ? workflow : 0u );
    }

    u32 Material::getWorkflow() const
    {
        return getEditorUInt( workflowStr, 0u );
    }

    void Material::setSpecularAmount( f32 value )
    {
        setEditorFloat( specularStr, value );
        setSpecular( ColourF( value, value, value, 1.0f ) );
    }

    f32 Material::getSpecularAmount() const
    {
        auto specular = getSpecular();
        auto defaultValue = ( specular.r + specular.g + specular.b ) / 3.0f;
        return getEditorFloat( specularStr, defaultValue );
    }

    void Material::setNormalStrength( f32 value )
    {
        setEditorFloat( normalStrengthStr, value );
    }

    f32 Material::getNormalStrength() const
    {
        return getEditorFloat( normalStrengthStr, 1.0f );
    }

    void Material::setDetailNormalStrength( f32 value )
    {
        setEditorFloat( detailNormalStrengthStr, value );
    }

    f32 Material::getDetailNormalStrength() const
    {
        return getEditorFloat( detailNormalStrengthStr, 1.0f );
    }

    void Material::setAlphaClip( f32 value )
    {
        setEditorFloat( alphaClipStr, value );
    }

    f32 Material::getAlphaClip() const
    {
        return getEditorFloat( alphaClipStr, 0.5f );
    }

    void Material::setOpacity( f32 value )
    {
        value = Math<real_Num>::clamp( value, 0.0f, 1.0f );
        m_hasPendingOpacity = true;
        m_pendingOpacity = value;
        setEditorFloat( opacityStr, value );

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
    }

    f32 Material::getOpacity() const
    {
        return getEditorFloat( opacityStr, getDiffuse().a );
    }

    void Material::setBlendMode( u32 blendMode )
    {
        setEditorUInt( blendModeStr, blendMode );

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
        return getEditorUInt( blendModeStr, 0u );
    }

    void Material::setDepthWrite( bool enabled )
    {
        setEditorBool( depthWriteStr, enabled );

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
                return getEditorBool( depthWriteStr, pass->isDepthWriteEnabled() );
            }
        }

        return getEditorBool( depthWriteStr, true );
    }

    void Material::setDepthTest( u32 mode )
    {
        setEditorUInt( depthTestStr, mode );

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
        return getEditorUInt( depthTestStr, 2u );
    }

    void Material::setCullMode( u32 mode )
    {
        setEditorUInt( cullModeStr, mode );

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
                return getEditorUInt( cullModeStr, pass->getCullingMode() );
            }
        }

        return getEditorUInt( cullModeStr, 2u );
    }

    void Material::setUVTiling( const Vector2F &tiling )
    {
        setEditorFloat( uvTilingXStr, tiling.X() );
        setEditorFloat( uvTilingYStr, tiling.Y() );
        setScale( Vector3F( tiling.X(), tiling.Y(), 1.0f ), 0u );
    }

    Vector2F Material::getUVTiling() const
    {
        return Vector2F( getEditorFloat( uvTilingXStr, 1.0f ),
                         getEditorFloat( uvTilingYStr, 1.0f ) );
    }

    void Material::setUVOffset( const Vector2F &offset )
    {
        setEditorFloat( uvOffsetXStr, offset.X() );
        setEditorFloat( uvOffsetYStr, offset.Y() );
    }

    Vector2F Material::getUVOffset() const
    {
        return Vector2F( getEditorFloat( uvOffsetXStr, 0.0f ),
                         getEditorFloat( uvOffsetYStr, 0.0f ) );
    }

    void Material::setUVRotation( f32 rotation )
    {
        setEditorFloat( uvRotationStr, rotation );
    }

    f32 Material::getUVRotation() const
    {
        return getEditorFloat( uvRotationStr, 0.0f );
    }

    void Material::setUVProjection( u32 projection )
    {
        // Keep this in sync with the material editor's Projection dropdown.
        setEditorUInt( uvProjectionStr, projection <= 6u ? projection : 0u );
    }

    u32 Material::getUVProjection() const
    {
        const auto projection = getEditorUInt( uvProjectionStr, 0u );
        return projection <= 6u ? projection : 0u;
    }

    void Material::setUVSet( u32 uvSet )
    {
        setEditorUInt( uvSetStr, uvSet <= 3u ? uvSet : 0u );
    }

    u32 Material::getUVSet() const
    {
        const auto uvSet = getEditorUInt( uvSetStr, 0u );
        return uvSet <= 3u ? uvSet : 0u;
    }

    void Material::setTriplanarScale( f32 scale )
    {
        const auto validScale = std::isfinite( scale ) ? Math<real_Num>::max( scale, 0.0001f ) : 1.0f;
        setEditorFloat( uvTriplanarScaleStr, validScale );
    }

    f32 Material::getTriplanarScale() const
    {
        const auto scale = getEditorFloat( uvTriplanarScaleStr, 1.0f );
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
        return m_cubicTextures.snapshot();
    }

    void Material::setCubicTextures( const Array<SmartPtr<ITexture>> &cubicTextures )
    {
        m_cubicTextures = { cubicTextures.begin(), cubicTextures.end() };
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

        const auto state = getRenderState();
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

        trySetColour( MaterialBuiltinParameter::Albedo, state ? state->diffuseColour : getDiffuse() );
        trySetScalar( MaterialBuiltinParameter::Metalness, state ? state->metalness : getMetalness() );
        trySetScalar( MaterialBuiltinParameter::Roughness, state ? state->roughness : getRoughness() );
        trySetColour( MaterialBuiltinParameter::Emissive, state ? state->emissiveColour : getEmissive() );
        trySetScalar( MaterialBuiltinParameter::Opacity, state ? state->opacity : getOpacity() );
        trySetScalar( MaterialBuiltinParameter::AlphaClip, state ? state->alphaClip : getAlphaClip() );

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
        auto target = ( m_shaderWorkflow != ShaderWorkflow::Pbr )
                          ? MaterialType::UI
                          : MaterialType::Standard;
        if( getMaterialType() != target )
        {
            setMaterialType( target );
        }
    }

    void Material::shaderParametersChanged( u8 mask ) noexcept
    {
        if( m_synchronizingShaderParameters ) return;
        m_pendingShaderSemantics |= mask;
        if( !m_pendingShaderSemantics ) return;
        m_shaderParametersDirty = true;
        if( m_shaderParameterEditDepth ) return;
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
        MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );
        const auto wants = [&]( MaterialBuiltinParameter semantic, MaterialShaderParameterType type, u32 size ) {
            return mask == 0xffu || ( ( mask & ( 1u << static_cast<u32>( semantic ) ) ) &&
                m_shaderParameters.IsHandleValid( m_shaderParameters.GetBuiltin( semantic ), type, size ) );
        };
        const auto scalar = [&]( MaterialBuiltinParameter semantic ) {
            return wants( semantic, MaterialShaderParameterType::Scalar, sizeof( f32 ) );
        };
        const auto colour = [&]( MaterialBuiltinParameter semantic ) {
            return wants( semantic, MaterialShaderParameterType::Colour, sizeof( ColourF ) );
        };
        auto getScalar = [this]( MaterialBuiltinParameter semantic, f32 def ) -> f32 {
            auto h = m_shaderParameters.GetBuiltin( semantic );
            return m_shaderParameters.IsHandleValid( h, MaterialShaderParameterType::Scalar, sizeof( f32 ) ) ?
                   m_shaderParameters.GetScalar( h ) : def;
        };
        auto getColour = [this]( MaterialBuiltinParameter semantic, const ColourF &def ) -> ColourF {
            auto h = m_shaderParameters.GetBuiltin( semantic );
            return m_shaderParameters.IsHandleValid( h, MaterialShaderParameterType::Colour, sizeof( ColourF ) ) ?
                   m_shaderParameters.GetColour( h ) : def;
        };

        const bool pbr = m_shaderWorkflow == ShaderWorkflow::Pbr;
        const bool colorOnly = m_shaderWorkflow == ShaderWorkflow::ColorOnly;
        const bool diffuseChanged = colorOnly ? colour( MaterialBuiltinParameter::Color ) :
                                                pbr && colour( MaterialBuiltinParameter::Albedo );
        const bool emissiveChanged = colorOnly ? diffuseChanged : colour( MaterialBuiltinParameter::Emissive );
        const bool metalChanged = pbr && scalar( MaterialBuiltinParameter::Metalness );
        const bool roughChanged = pbr && scalar( MaterialBuiltinParameter::Roughness );
        const bool clipChanged = pbr && scalar( MaterialBuiltinParameter::AlphaClip );
        const bool opacityChanged = diffuseChanged || scalar( MaterialBuiltinParameter::Opacity );
        const auto currentDiffuse = state ? state->diffuseColour : getDiffuse();
        auto diffuse = diffuseChanged ? getColour( colorOnly ? MaterialBuiltinParameter::Color :
            MaterialBuiltinParameter::Albedo, colorOnly ? ColourF( 0.5f, 0.5f, 0.5f, 1 ) : currentDiffuse ) : currentDiffuse;
        const auto emissive = colorOnly ? getColour( MaterialBuiltinParameter::Color, ColourF( 0.5f, 0.5f, 0.5f, 1 ) ) :
            getColour( MaterialBuiltinParameter::Emissive, pbr ? ( state ? state->emissiveColour : getEmissive() ) :
                                                                     ColourF( 0.5f, 0.5f, 0.5f, 1 ) );
        const auto metal = getScalar( MaterialBuiltinParameter::Metalness, state ? state->metalness : getMetalness() );
        const auto rough = getScalar( MaterialBuiltinParameter::Roughness, state ? state->roughness : getRoughness() );
        const auto clip = getScalar( MaterialBuiltinParameter::AlphaClip, state ? state->alphaClip : getAlphaClip() );
        const auto opacity = Math<f32>::clamp( getScalar( MaterialBuiltinParameter::Opacity,
            pbr ? ( state ? state->opacity : getOpacity() ) : 1.0f ), 0.0f, 1.0f );
        const bool transparent = opacityChanged && opacity < 0.999f;
        const bool changeRenderMode = transparent && getRenderMode() < 2u;
        if( opacityChanged ) { diffuse.a = opacity; m_hasPendingOpacity = true; m_pendingOpacity = opacity; }
        if( metalChanged ) { m_hasPendingMetalness = true; m_pendingMetalness = metal; }
        if( roughChanged ) { m_hasPendingRoughness = true; m_pendingRoughness = rough; }

        bool applied = false;
        // One graph walk and one write scope per pass. Virtual surface setters keep
        // backend behavior; the outer state lock coalesces their publications.
        for( auto technique : getTechniques() )
        {
            if( !technique ) continue;
            for( auto pass : technique->getPasses() )
            {
                if( !pass ) continue;
                auto context = pass->getStateContext();
                auto data = context ? context->invalidateStateDataById<MaterialPassStateData>( pass->getId() ) :
                                      SafePtr<MaterialPassStateData>{};
                if( diffuseChanged || opacityChanged ) pass->setDiffuse( diffuse );
                if( emissiveChanged ) pass->setEmissive( emissive );
                if( metalChanged ) pass->setMetalness( metal );
                if( roughChanged ) pass->setRoughness( rough );
                if( data )
                {
                    if( clipChanged ) data->setFloatSetting( MaterialFloatSetting::alphaClip, clip );
                    if( opacityChanged ) data->setFloatSetting( MaterialFloatSetting::opacity, opacity );
                    if( changeRenderMode ) data->setUIntSetting( MaterialUIntSetting::renderMode, 3u );
                }
                if( transparent ) pass->setTransparent( true );
                if( changeRenderMode ) { pass->setCutout( false ); pass->setSceneBlending( 1u ); }
                applied = true;
            }
        }
        if( applied ) { m_shaderParametersDirty = false; m_pendingShaderSemantics = 0; }
        else { m_shaderParametersDirty = true; m_pendingShaderSemantics |= mask; }
    }

    void Material::applyDataDrivenShaderToRenderer()
    {
        routeDataDrivenMaterialType();
        applyShaderParametersToRenderer();
        applyDataDrivenTextures();
    }

    void Material::setShaderTexture( const String &paramName, const String &texturePath )
    {
        for( auto &ref : m_shaderTextureRefs )
        {
            if( ref.paramName == paramName )
            {
                ref.texturePath = texturePath;
                return;
            }
        }
        m_shaderTextureRefs.push_back( { paramName, texturePath } );
    }

    String Material::getShaderTexture( const String &paramName ) const
    {
        for( auto const &ref : m_shaderTextureRefs )
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
        if( !isDataDriven() || !m_shaderParameters.IsValid() )
        {
            return;
        }
        // Best-effort: bind each named texture slot to a texture unit by ordinal. Backend
        // overrides (step 3c) should map these to the correct Hlms Pbs/Unlit semantic slots.
        u32 layerIdx = 0;
        auto const &info = m_shaderParameters.GetParameterInfo();
        for( auto const &p : info )
        {
            if( p.m_type != MaterialShaderParameterType::Texture )
            {
                continue;
            }
            auto path = getShaderTexture( p.m_name );
            if( !path.empty() )
            {
                setTexture( path, layerIdx );
            }
            ++layerIdx;
        }
    }

    void Material::setShaderParameters( const Array<MaterialShaderParameterInfo> &parameterInfo,
                                        const String &shaderID /*= String() */ )
    {
        const auto candidateId = shaderID.empty() ? m_shaderID : shaderID;
        auto shader = MaterialShaderRegistry::instance().find( candidateId );
        auto layout = shader ? shader->getLayout() : MaterialShaderParameterLayout::Ptr{};
        bool sameSchema = layout && layout->GetParameterInfo().size() == parameterInfo.size();
        for( size_t i = 0; sameSchema && i < parameterInfo.size(); ++i )
        {
            const auto &registered = layout->GetParameterInfo()[i];
            sameSchema = registered.m_name == parameterInfo[i].m_name &&
                         registered.m_type == parameterInfo[i].m_type;
        }
        if( !sameSchema ) layout = MaterialShaderParameterLayout::Create( parameterInfo );
        if( !layout )
        {
            WP_LOG_WARNING( "Material rejected an invalid shader parameter schema." );
            return; // Keep the last valid shader/layout and values.
        }
        MaterialParameterSyncScope synchronization( m_synchronizingShaderParameters );
        auto edit = m_shaderParameters.Edit();
        auto previous = std::move( m_shaderParameters );
        m_shaderParameters.Reset( std::move( layout ) );
        if( !shaderID.empty() ) setShaderID( shaderID );
        // Layout replacement is an authoring/load operation. Resolve names here only;
        // compatible values survive reordered/extended schemas and partial updates.
        for( u32 i = 0; i < m_shaderParameters.GetParameterInfo().size(); ++i )
        {
            const auto &info = m_shaderParameters.GetParameterInfo()[i];
            const auto old = previous.FindParameter( info.m_name );
            if( !old.IsValid() || previous.GetParameterInfo()[old.m_parameterIndex].m_type != info.m_type )
                continue;
            const auto next = m_shaderParameters.GetParameter( i );
            switch( info.m_type )
            {
            case MaterialShaderParameterType::Scalar:
                m_shaderParameters.SetScalar( next, previous.GetScalar( old ) ); break;
            case MaterialShaderParameterType::Int:
                m_shaderParameters.SetScalar( next, previous.GetInt( old ) ); break;
            case MaterialShaderParameterType::UInt:
                m_shaderParameters.SetScalar( next, previous.GetUInt( old ) ); break;
            case MaterialShaderParameterType::Bool:
                m_shaderParameters.SetBool( next, previous.GetBool( old ) ); break;
            case MaterialShaderParameterType::Colour:
                m_shaderParameters.SetColour( next, previous.GetColour( old ) ); break;
            case MaterialShaderParameterType::Vector2:
                m_shaderParameters.SetVector2( next, previous.GetVector2f( old ) ); break;
            case MaterialShaderParameterType::Vector4:
                m_shaderParameters.SetVector4( next, previous.GetVector4f( old ) ); break;
            case MaterialShaderParameterType::IntVector2:
            {
                Vector2<s32> value;
                previous.Get( MaterialParameterHandle<Vector2<s32>>{ old }, value );
                m_shaderParameters.SetVector2( next, value ); break;
            }
            case MaterialShaderParameterType::IntVector4:
            {
                Vector4<s32> value;
                previous.Get( MaterialParameterHandle<Vector4<s32>>{ old }, value );
                m_shaderParameters.SetVector4( next, value ); break;
            }
            case MaterialShaderParameterType::Matrix:
                m_shaderParameters.SetMatrix( next, previous.GetMatrix( old ) ); break;
            case MaterialShaderParameterType::Texture:
                m_shaderParameters.SetTexture( next, previous.GetResource( old ) ); break;
            case MaterialShaderParameterType::Buffer:
                m_shaderParameters.SetBuffer( next, previous.GetResource( old ) ); break;
            case MaterialShaderParameterType::Sampler:
                m_shaderParameters.SetSampler( next, previous.GetResource( old ) ); break;
            }
        }
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
        m_shaderID = shaderID;
        m_shaderWorkflow = shaderID == unlitShaderIDStr ? ShaderWorkflow::Unlit :
                           shaderID == colorOnlyShaderIDStr ? ShaderWorkflow::ColorOnly :
                                                            ShaderWorkflow::Pbr;
        m_shaderParametersDirty = true;
    }

    String Material::getShaderID() const
    {
        return m_shaderID;
    }

    bool Material::isDataDriven() const
    {
        return !m_shaderID.empty();
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
            auto next = std::make_shared<PrimaryPassSelection>();
            next->graphRevision = revision;
            for( const auto &technique : getTechniques() )
            {
                if( !technique ) continue;
                for( const auto &pass : technique->getPasses() )
                {
                    auto context = pass ? pass->getStateContext() : nullptr;
                    auto source = context ? context->getStateById( pass->getId() ) : nullptr;
                    auto data = source ? source->getData() : nullptr;
                    if( data && data->isDerived<MaterialPassStateData>() )
                    { next->state = source; break; }
                }
                if( next->state ) break;
            }
            selection = next;
            // An overlapping graph edit may render the retained old version once,
            // but must never publish it as the new graph's selection.
            if( m_graphRevision.load( std::memory_order_acquire ) == revision )
                std::atomic_store( &m_primaryPassSelection, selection );
        }
        auto data = selection->state ? selection->state->getData() : nullptr;
        return data && data->isDerived<MaterialPassStateData>() ?
            static_pointer_cast<MaterialPassStateData>( data )->getRenderState() : nullptr;
    }

    SmartPtr<IMaterialPass> Material::getPass() const
    {
        return m_pass;
    }

}  // namespace workphone::render
