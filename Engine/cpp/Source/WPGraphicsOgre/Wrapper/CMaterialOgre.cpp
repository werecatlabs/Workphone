#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialOgre, Material );

        u32 CMaterialOgre::m_nameExt = 0;

        CMaterialOgre::CMaterialOgre()
        {
            static const auto MaterialStr = String( "Material" );
            auto name = MaterialStr + StringUtil::toString( m_nameExt++ );
            setName( name );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            WP_ASSERT( stateContext );

            auto state = factoryManager->make_ptr<State>();
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<MaterialStateData>();
            state->setData( stateData );
        }

        CMaterialOgre::~CMaterialOgre()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }
        }

        void CMaterialOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                WP_ASSERT( resourceGroupManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                const auto stateTask = graphicsSystem->getStateTask();

                setLoadingState( LoadingState::Loading );

                WP_ASSERT( getMaterialType() < MaterialType::Count );

                if( !m_material )
                {
                    createMaterialByType();
                }

                WP_ASSERT( m_material );

                WP_ASSERT( m_material->getNumTechniques() == 1 );

                if( m_material )
                {
                    m_material->load();
                }

                WP_ASSERT( m_material->getNumTechniques() == 1 );

                auto techniques = m_techniques.snapshot();
                for( auto technique : techniques )
                {
                    technique->load( nullptr );
                }

                setLoadingState( LoadingState::Loaded );

                auto message = factoryManager->make_ptr<StateMessageLoad>();
                message->setType( StateMessageLoad::LOADED_HASH );
                message->setObject( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialOgre::reload( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( isThreadSafe() );

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
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &state = getLoadingState();
                if( state != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto techniques = m_techniques.snapshot();
                    for( auto technique : techniques )
                    {
                        technique->unload( data );
                    }

                    m_techniques.clear();

                    m_material = nullptr;

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CMaterialOgre::setCubicTexture( const String &fileName, bool uvw, u32 layerIdx )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            if( isThreadSafe() )
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
            else
            {
                auto message = factoryManager->make_ptr<StateMessageSetTexture>();
                message->setType( SET_TEXTURE_HASH );
                message->setTextureName( fileName );
                message->setTextureIndex( layerIdx );
                addMessage( message );
            }
        }

        void CMaterialOgre::setCubicTexture( const Array<SmartPtr<render::ITexture>> &textures,
                                             u32 layerIdx /*= 0 */ )
        {
            if( textures.size() < (u32)SkyboxTextureTypes::Count )
            {
                WP_LOG_ERROR( "Invalid texture array size" );
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            if( isThreadSafe() )
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
                            std::vector<Ogre::String> textureNames;
                            textureNames.resize( 6 );

                            Array<u32> map;
                            map.reserve( 6 );
                            map.push_back( (u32)SkyboxTextureTypes::Right );
                            map.push_back( (u32)SkyboxTextureTypes::Left );
                            map.push_back( (u32)SkyboxTextureTypes::Up );
                            map.push_back( (u32)SkyboxTextureTypes::Down );
                            map.push_back( (u32)SkyboxTextureTypes::Front );
                            map.push_back( (u32)SkyboxTextureTypes::Back );

                            for( size_t i = 0; i < 6; ++i )
                            {
                                auto index = map[i];
                                auto pTexture = textures[index];
                                if( pTexture )
                                {
                                    auto textureFilePath = pTexture->getFilePath();
                                    textureNames[i] = Ogre::String( textureFilePath.c_str(),
                                                                    textureFilePath.length() );
                                }
                                else
                                {
                                    textureNames[i] = "checker.png";
                                }
                            }

                            auto materialName = getName();
                            auto grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                            auto textureManager = graphicsSystem->getTextureManager();
                            auto textureResult =
                                textureManager->createOrRetrieve( materialName + "_Cubemap" );
                            auto texture =
                                workphone::static_pointer_cast<CTextureOgre>( textureResult.first );
                            texture->setTextureType( TextureType::TEX_TYPE_CUBE_MAP );
                            texture->load( nullptr );

                            auto tex = texture->getTexture();
                            if( tex )
                            {
                                tex->setLayerNames( textureNames );
                                tex->reload();

                                textureUnitState->setTexture( tex );
                            }
                        }
                    }
                }

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObjectsArray>();
                message->setType( StateMessage::SET_CUBEMAP );

                auto objects = Array<SmartPtr<ISharedObject>>( textures.begin(), textures.end() );
                message->setObjects( objects );

                addMessage( message );
            }
        }

        void CMaterialOgre::setFragmentParam( const String &name, f32 value )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
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
            else
            {
                auto message = factoryManager->make_ptr<StateMessageFragmentParam>();
                message->setType( FRAGMENT_FLOAT_HASH );
                message->setName( name );
                message->setFloat( value );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
                }
            }
        }

        void CMaterialOgre::setFragmentParam( const String &name, const Vector2F &value )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
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
            else
            {
                SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
                message->setType( FRAGMENT_VECTOR2F_HASH );
                message->setName( name );
                message->setVector2f( value );

                if( auto stateConext = getStateContext() )
                {
                    stateConext->addMessage( renderTask, message );
                }
            }
        }

        void CMaterialOgre::setFragmentParam( const String &name, const Vector3F &value )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
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
            else
            {
                SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
                message->setType( FRAGMENT_VECTOR3F_HASH );
                message->setName( name );
                message->setVector3f( value );

                if( auto stateConext = getStateContext() )
                {
                    stateConext->addMessage( renderTask, message );
                }
            }
        }

        void CMaterialOgre::setFragmentParam( const String &name, const Vector4F &value )
        {
            auto applicationManager = core::IApplicationManager::instance();

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
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
                                name.c_str(),
                                Ogre::Vector4( value.X(), value.Y(), value.Z(), value.W() ) );
                        }
                    }
                }
            }
            else
            {
                SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
                message->setType( FRAGMENT_VECTOR4F_HASH );
                message->setName( name );
                message->setVector4f( value );

                if( auto stateConext = getStateContext() )
                {
                    stateConext->addMessage( renderTask, message );
                }
            }
        }

        void CMaterialOgre::setFragmentParam( const String &name, const ColourF &value )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
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
            else
            {
                SmartPtr<StateMessageFragmentParam> message( new StateMessageFragmentParam );
                message->setType( FRAGMENT_COLOUR_HASH );
                message->setName( name );
                message->setColourf( value );

                if( auto stateConext = getStateContext() )
                {
                    stateConext->addMessage( renderTask, message );
                }
            }
        }

        void CMaterialOgre::setScale( const Vector3F &scale, u32 textureIndex /*= 0*/,
                                      u32 passIndex /*= 0*/, u32 techniqueIndex /*= 0 */ )
        {
            if( auto material = getMaterial() )
            {
                auto mat = Ogre::Matrix4::IDENTITY;
                mat.setScale( Ogre::Vector3( scale.X(), scale.Y(), scale.Z() ) );
                material->getTechnique( techniqueIndex )
                    ->getPass( passIndex )
                    ->getTextureUnitState( textureIndex )
                    ->setTextureTransform( mat );
            }
        }

        SmartPtr<IMaterialNode> CMaterialOgre::getRoot() const
        {
            return m_root;
        }

        void CMaterialOgre::setRoot( SmartPtr<IMaterialNode> root )
        {
            m_root = root;
        }

        void CMaterialOgre::createMaterialByType()
        {
            try
            {
                auto uuid = String();

                if( auto handle = getHandle() )
                {
                    uuid = handle->getUUIDAsString();
                }

                if( StringUtil::isNullOrEmpty( uuid ) )
                {
                    uuid = StringUtil::getUUID();
                }

                auto materialManager = Ogre::MaterialManager::getSingletonPtr();

                auto resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                if( m_material == nullptr )
                {
                    //if ( getNumTechniques() != 0 )
                    //{
                    //    removeAllTechniques();
                    //}

                    switch( auto materialType = getMaterialType() )
                    {
                    case MaterialType::Standard:
                    {
                        //auto result = materialManager->load( "Standard", resourceGroup );
                        //auto pbrBase = Ogre::dynamic_pointer_cast<Ogre::Material>( result );
                        //if( pbrBase )
                        //{
                        //    m_material = pbrBase->clone( uuid );
                        //}
                        //else
                        {
                            auto result =
                                materialManager->createOrRetrieve( uuid.c_str(), resourceGroup );
                            m_material = Ogre::static_pointer_cast<Ogre::Material>( result.first );
                        }
                    }
                    break;
                    default:
                    {
                        auto result = materialManager->createOrRetrieve( uuid.c_str(), resourceGroup );
                        m_material = Ogre::static_pointer_cast<Ogre::Material>( result.first );
                    }
                    };
                }

                if( m_material )
                {
                    m_material->load();
                }

                //if( auto p = getTechniquesPtr() )
                //{
                //    auto &techniques = *p;
                //    for( auto t : techniques )
                //    {
                //        t->unload( nullptr );
                //    }

                //    techniques.clear();
                //}

                //m_material->removeAllTechniques();

                //WP_ASSERT( isValid() );

                auto ogreTechniques = m_material->getTechniques();

                auto count = 0;

                if( ogreTechniques.size() > getNumTechniques() )
                {
                    while( ogreTechniques.size() != getNumTechniques() )
                    {
                        auto technique = workphone::make_ptr<CMaterialTechniqueOgre>();
                        addTechnique( technique );
                    }
                }

                auto techniques = m_techniques.snapshot();
                for( size_t i = 0; i < techniques.size(); ++i )
                {
                    auto technique =
                        workphone::static_pointer_cast<CMaterialTechniqueOgre>( techniques[i] );

                    if( count < ogreTechniques.size() )
                    {
                        technique->setTechnique( ogreTechniques[count] );
                    }

                    technique->setMaterial( this );

                    count++;
                }

                //if( auto p = getTechniquesPtr() )
                //{
                //    auto &techniques = *p;
                //    techniques.push_back( technique );
                //}

                WP_ASSERT( isValid() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<SmartPtr<ISharedObject>> CMaterialOgre::getChildObjects() const
        {
            auto objects = Material::getChildObjects();

            return objects;
        }

        Ogre::MaterialPtr CMaterialOgre::getMaterial() const
        {
            return m_material;
        }

        void CMaterialOgre::setMaterial( Ogre::MaterialPtr material )
        {
            m_material = material;
        }

        bool CMaterialOgre::isValid() const
        {
            if( isLoaded() )
            {
                if( auto material = getMaterial() )
                {
                    auto techniques = getTechniques();
                    return material->getNumTechniques() == techniques.size();
                }
            }

            return true;
        }

        bool CMaterialOgre::MaterialStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( !m_owner->isLoaded() )
            {
                m_owner->load( nullptr );
            }

            auto messageType = message->getType();

            if( message->isExactly<StateMessagePair<bool, int>>() )
            {
                auto pairMessage =
                    workphone::static_pointer_cast<StateMessagePair<bool, int>>( message );

                auto first = pairMessage->getFirst();
                auto second = pairMessage->getSecond();

                if( messageType == IMaterial::LIGHTING_ENABLED_HASH )
                {
                    m_owner->setLightingEnabled( first, second );
                }
            }
            else if( message->isExactly<StateMessageUIntValue>() )
            {
                auto intMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );

                if( messageType == StringUtil::getHash( "materialType" ) )
                {
                    m_owner->setMaterialType( (MaterialType)intMessage->getValue() );
                }
            }
            else if( message->isExactly<StateMessageLoad>() )
            {
                auto loadMessage = workphone::static_pointer_cast<StateMessageLoad>( message );
                if( messageType == StateMessageLoad::LOAD_HASH )
                {
                    m_owner->load( nullptr );
                }
                else
                {
                    m_owner->reload( nullptr );
                }
            }
            else if( message->isExactly<StateMessageFragmentParam>() )
            {
                auto fragmentMessage =
                    workphone::static_pointer_cast<StateMessageFragmentParam>( message );

                if( messageType == FRAGMENT_FLOAT_HASH )
                {
                    m_owner->setFragmentParam( fragmentMessage->getName(), fragmentMessage->getFloat() );
                }
                else if( messageType == FRAGMENT_VECTOR2F_HASH )
                {
                    m_owner->setFragmentParam( fragmentMessage->getName(),
                                               fragmentMessage->getVector2f() );
                }
                else if( messageType == FRAGMENT_VECTOR3F_HASH )
                {
                    m_owner->setFragmentParam( fragmentMessage->getName(),
                                               fragmentMessage->getVector3f() );
                }
                else if( messageType == FRAGMENT_VECTOR4F_HASH )
                {
                    m_owner->setFragmentParam( fragmentMessage->getName(),
                                               fragmentMessage->getVector4f() );
                }
                else if( messageType == FRAGMENT_COLOUR_HASH )
                {
                    m_owner->setFragmentParam( fragmentMessage->getName(),
                                               fragmentMessage->getColourf() );
                }
            }
            else if( message->isExactly<StateMessageSetTexture>() )
            {
                auto textureMessage = workphone::static_pointer_cast<StateMessageSetTexture>( message );
                if( auto texture = textureMessage->getTexture() )
                {
                    const auto textureIndex = textureMessage->getTextureIndex();
                    m_owner->setTexture( texture, textureIndex );
                }
                else
                {
                    const auto textureName = textureMessage->getTextureName();
                    const auto textureIndex = textureMessage->getTextureIndex();
                    m_owner->setTexture( textureName, textureIndex );
                }
            }
            else if( message->isExactly<StateMessageObjectsArray>() )
            {
                auto arrayMessage = workphone::static_pointer_cast<StateMessageObjectsArray>( message );
                auto value = arrayMessage->getObjects();

                if( messageType == StateMessage::SET_CUBEMAP )
                {
                    m_owner->setCubicTexture( Array<SmartPtr<ITexture>>( value.begin(), value.end() ) );
                }
            }

            return false;
        }

        bool CMaterialOgre::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            const auto &loadingState = m_owner->getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                state->setDirty( false );
            }

            return false;
        }

        CMaterialOgre *CMaterialOgre::MaterialStateListener::getOwner() const
        {
            return m_owner;
        }

        void CMaterialOgre::MaterialStateListener::setOwner( CMaterialOgre *owner )
        {
            m_owner = owner;
        }

        CMaterialOgre::MaterialStateListener::~MaterialStateListener()
        {
        }

        CMaterialOgre::MaterialStateListener::MaterialStateListener()
        {
        }

        CMaterialOgre::MaterialStateListener::MaterialStateListener( CMaterialOgre *material ) :
            m_owner( material )
        {
        }

        void CMaterialOgre::MaterialEvents::preRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
        }

        void CMaterialOgre::MaterialEvents::postRenderTargetUpdate( const Ogre::RenderTargetEvent &evt )
        {
        }

        void CMaterialOgre::MaterialEvents::preViewportUpdate(
            const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CMaterialOgre::MaterialEvents::postViewportUpdate(
            const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CMaterialOgre::MaterialEvents::viewportAdded( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        void CMaterialOgre::MaterialEvents::viewportRemoved( const Ogre::RenderTargetViewportEvent &evt )
        {
        }

        CMaterialOgre::MaterialEvents::~MaterialEvents()
        {
        }

        CMaterialOgre::MaterialEvents::MaterialEvents( CMaterialOgre *material ) : m_material( material )
        {
        }

        CMaterialOgre::MaterialOgreListener::MaterialOgreListener()
        {
        }

        CMaterialOgre::MaterialOgreListener::~MaterialOgreListener()
        {
        }

    }  // end namespace render
}  // namespace workphone
