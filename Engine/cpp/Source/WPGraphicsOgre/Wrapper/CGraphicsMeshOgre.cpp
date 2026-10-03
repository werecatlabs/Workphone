#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsMeshOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CAnimationControllerOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Core/MeshLoader.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreSceneManager.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgre, IGraphicsMesh );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgre::ScriptReceiver,
                                   IScriptReceiver );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgre::MeshStateListener,
                                   GraphicsObjectOgreStateListener );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgre::MaterialStateListener,
                                   IStateListener );

        CGraphicsMeshOgre::CGraphicsMeshOgre()
        {
            setupStateObject();
        }

        CGraphicsMeshOgre::~CGraphicsMeshOgre()
        {
            unload( nullptr );
        }

        void CGraphicsMeshOgre::update()
        {
            if( m_animationController )
            {
                m_animationController->update();
            }
        }

        void CGraphicsMeshOgre::registerForUpdates( bool registerObject )
        {
            // if(registerObject)
            //{
            //	SmartPtr<CSceneManagerOgre> smgr = m_creator;
            //	smgr->registerForUpdates(SmartPtr<IGraphicsObject>(this));
            // }
            // else
            //{
            //	SmartPtr<CSceneManagerOgre> smgr = m_creator;
            //	smgr->unregisteredForUpdates(this);
            // }
        }

        bool CGraphicsMeshOgre::isRegisteredForUpdates()
        {
            return false;
        }

        void CGraphicsMeshOgre::setTestOcclusion( bool testOcclusion )
        {
            // m_entity->setTestOcclusion(testOcclusion);
        }

        bool CGraphicsMeshOgre::getTestOcclusion() const
        {
            return false;
        }

        void CGraphicsMeshOgre::setIsOccluder( bool isOccluder )
        {
            // m_entity->setIsOccluder(isOccluder);
        }

        bool CGraphicsMeshOgre::isOccluder() const
        {
            return false;
        }

        void CGraphicsMeshOgre::detachFromParent()
        {
            // if(m_parent)
            //{
            //	m_parent->detachObject(this);
            //	m_parent = nullptr;
            // }

            ScopedLock lock( this );

            if( m_entity->isAttached() )
            {
                m_entity->detachFromParent();
            }
        }

        void CGraphicsMeshOgre::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
            m_owner = parent;
        }

        void CGraphicsMeshOgre::setMaterialName( const String &materialName, s32 index )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto materialManager = graphicsSystem->getMaterialManager();

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                if( index == -1 )
                {
                    auto material = materialManager->getByName( materialName );
                    if( material )
                    {
                        if( m_entity )
                        {
                            m_entity->setMaterialName( materialName.c_str() );
                        }

                        auto materialStateObject = material->getStateContext();
                        if( materialStateObject )
                        {
                            materialStateObject->addStateListener( m_materialStateListener );
                        }
                    }
                }
                else
                {
                    auto material = materialManager->getByName( materialName );
                    if( material )
                    {
                        if( m_entity )
                        {
                            auto subEntity = m_entity->getSubEntity( index );
                            subEntity->setMaterialName( materialName.c_str() );
                        }

                        auto materialStateObject = material->getStateContext();
                        if( materialStateObject )
                        {
                            materialStateObject->addStateListener( m_materialStateListener );
                        }
                    }
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageMaterialName>();
                message->setMaterialName( materialName );
                message->setIndex( index );

                auto stateContext = getStateContext();
                if( stateContext )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }

        String CGraphicsMeshOgre::getMaterialName( s32 index ) const
        {
            return StringUtil::EmptyString;
        }

        void CGraphicsMeshOgre::setMaterial( SmartPtr<IMaterial> material, s32 index /*= -1*/ )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto materialManager = graphicsSystem->getMaterialManager();

                auto factoryManager = applicationManager->getFactoryManager();
                auto renderTask = graphicsSystem->getRenderTask();
                auto stateTask = graphicsSystem->getStateTask();
                auto task = Thread::getCurrentTask();

                const auto &loadingState = getLoadingState();
                if( loadingState == LoadingState::Loaded && task == renderTask )
                {
                    if( material )
                    {
                        if( !material->isLoaded() )
                        {
                            material->load( nullptr );
                        }
                    }

                    if( index == -1 )
                    {
                        if( m_entity )
                        {
                            // m_entity->setMaterialName(materialName.c_str());
                        }

                        auto entity = getEntity();
                        if( entity )
                        {
                            if( m_material )
                            {
                                auto materialStateObject = m_material->getStateContext();
                                if( materialStateObject )
                                {
                                    materialStateObject->removeStateListener( m_materialStateListener );
                                }
                            }

                            auto pMaterial = workphone::static_pointer_cast<CMaterialOgre>( material );
                            if( pMaterial )
                            {
                                auto datablock = pMaterial->getMaterial();
                                if( datablock )
                                {
                                    entity->setMaterial( datablock );
                                }
                            }

                            m_material = material;

                            if( m_material )
                            {
                                auto materialStateObject = m_material->getStateContext();
                                if( materialStateObject )
                                {
                                    materialStateObject->addStateListener( m_materialStateListener );
                                }
                            }
                        }
                    }
                    else
                    {
                        if( auto entity = getEntity() )
                        {
                            if( auto subEntity = entity->getSubEntity( index ) )
                            {
                                auto currentMaterial = m_materials[index];
                                if( currentMaterial )
                                {
                                    auto materialStateObject = currentMaterial->getStateContext();
                                    if( materialStateObject )
                                    {
                                        materialStateObject->removeStateListener(
                                            m_materialStateListener );
                                    }
                                }

                                auto pMaterial =
                                    workphone::static_pointer_cast<CMaterialOgre>( material );
                                if( pMaterial )
                                {
                                    auto mat = pMaterial->getMaterial();
                                    if( mat )
                                    {
                                        subEntity->setMaterial( mat );
                                    }
                                }

                                m_materials[index] = material;

                                auto materialStateObject = material->getStateContext();
                                if( materialStateObject )
                                {
                                    materialStateObject->addStateListener( m_materialStateListener );
                                }
                            }
                        }
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageMaterial>();
                    message->setMaterial( material );
                    message->setIndex( index );

                    auto stateContext = getStateContext();
                    if( stateContext )
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

        SmartPtr<IMaterial> CGraphicsMeshOgre::getMaterial( s32 index /*= -1*/ ) const
        {
            return m_material;
        }

        SmartPtr<IGraphicsObject> CGraphicsMeshOgre::clone( const String &name ) const
        {
            auto m = workphone::make_ptr<CGraphicsMeshOgre>();
            return m;
        }

        void CGraphicsMeshOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_entity;
        }

        void CGraphicsMeshOgre::checkVertexProcessing()
        {
            m_checkVertProcessing = true;
        }

        SmartPtr<IAnimationController> CGraphicsMeshOgre::getAnimationController()
        {
            if( !m_animationController )
            {
                SmartPtr<CAnimationController> animationController( new CAnimationController );
                animationController->Initialize( m_entity );
                m_animationController = animationController;
                // m_animationController->getStateContext()->add();
            }

            return m_animationController;
        }

        Ogre::Entity *CGraphicsMeshOgre::getEntity() const
        {
            return m_entity;
        }

        void CGraphicsMeshOgre::setHardwareAnimationEnabled( bool enabled )
        {
            m_hardwareAnimationEnabled = enabled;
        }

        s32 CGraphicsMeshOgre::ScriptReceiver::setProperty( hash_type hash, const String &value )
        {
            // RecursiveMutex::ScopedLock lock(OgreMutex);

            // if(hash == StringUtil::MATERIAL_HASH)
            //{
            //	m_meshObject->setMaterialName(value);
            // }

            return 0;
        }

        s32 CGraphicsMeshOgre::ScriptReceiver::setProperty( hash_type hash, const Parameter &param )
        {
            // RecursiveMutex::ScopedLock lock(OgreMutex);

            // if(hash == StringUtil::VISIBLE_HASH)
            //{
            //	m_meshObject->setVisible((bool)param.data.bData);
            // }
            // else if( hash == StringUtil::VISIBILITY_MASK_HASH )
            //{
            //	m_meshObject->setVisibilityFlags(param.data.iData);
            // }

            return 0;
        }

        s32 CGraphicsMeshOgre::ScriptReceiver::getProperty( hash_type id, Parameter &param ) const
        {
            // if( id == StringUtil::VISIBILITY_MASK_HASH )
            //{
            //	param.data.iData = m_meshObject->getVisibilityFlags();
            //	return 0;
            // }

            return 0;
        }

        CGraphicsMeshOgre::ScriptReceiver::ScriptReceiver( CGraphicsMeshOgre *meshObject ) :
            m_meshObject( meshObject )
        {
        }

        void CGraphicsMeshOgre::handleEvent( SmartPtr<IEvent> event )
        {
        }

        void CGraphicsMeshOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto handle = getHandle();
                WP_ASSERT( handle );

                auto name = getName();
                //WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

                auto meshPath = getMeshName();
                WP_ASSERT( !StringUtil::isNullOrEmpty( meshPath ) );

                auto ext = Path::getFileExtension( meshPath );
                if( StringUtil::isNullOrEmpty( ext ) )
                {
                    meshPath += ".mesh";
                }

                Ogre::SceneManager *smgr = nullptr;
                if( auto creator = getCreator() )
                {
                    creator->_getObject( (void **)&smgr );
                }

                WP_ASSERT( smgr );

                if( smgr )
                {
                    auto meshName = getMeshName();
                    if( !StringUtil::isNullOrEmpty( meshName ) )
                    {
                        auto mesh = createMesh( meshName );
                        if( !mesh )
                        {
                            WP_LOG_ERROR( "Unable to create mesh: " + meshName );
                        }

                        if( mesh )
                        {
                            auto entity = smgr->createEntity( name.c_str(), mesh );
                            WP_ASSERT( entity );
                            m_entity = entity;
                        }
                    }
                }

                if( auto owner = getOwner() )
                {
                    Ogre::SceneNode *node = nullptr;
                    owner->_getObject( (void **)&node );

                    if( node )
                    {
                        if( m_entity )
                        {
                            node->attachObject( m_entity );
                        }
                    }
                }

                if( m_entity )
                {
                    auto numSubItems = m_entity->getNumSubEntities();
                    m_materials.resize( numSubItems );
                }

                auto mask = static_cast<u32>( 0 );
                mask = BitUtil::setFlagValue( mask, SceneFlag, true );
                setVisibilityFlags( mask );

                setLoadingState( LoadingState::Loaded );
            }
            catch( Ogre::Exception &e )
            {
                auto message = e.getFullDescription();
                WP_LOG_ERROR( message.c_str() );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CGraphicsMeshOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    //WP_ASSERT( !getOwner() );

                    SmartPtr<CGraphicsSceneOgre> smgr = getCreator();

                    if( m_entity )
                    {
                        m_entity->detachFromParent();

                        Ogre::SceneManager *ogreSmgr = smgr->getSceneManager();
                        ogreSmgr->destroyEntity( m_entity );
                        m_entity = nullptr;
                    }

                    registerForUpdates( false );

                    if( m_materialStateListener )
                    {
                        if( auto stateContext = getStateContext() )
                        {
                            stateContext->removeStateListener( m_materialStateListener );
                            m_materialStateListener = nullptr;
                        }
                    }

                    CGraphicsObjectOgre<IGraphicsMesh>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        String CGraphicsMeshOgre::getMeshName() const
        {
            return m_meshName;
        }

        void CGraphicsMeshOgre::setMeshName( const String &meshName )
        {
            m_meshName = meshName;
        }

        ProgressiveMeshOptions CGraphicsMeshOgre::getProgressiveMeshOptions() const
        {
            return m_progressiveMeshOptions.load();
        }

        void CGraphicsMeshOgre::setProgressiveMeshOptions( const ProgressiveMeshOptions &options )
        {
            m_progressiveMeshOptions.store( options );
        }

        SmartPtr<Properties> CGraphicsMeshOgre::getProperties() const
        {
            auto properties = CGraphicsObjectOgre<IGraphicsMesh>::getProperties();

            properties->setProperty( "MeshName", m_meshName );

            return properties;
        }

        void CGraphicsMeshOgre::setProperties( SmartPtr<Properties> properties )
        {
            properties->getPropertyValue( "MeshName", m_meshName );
        }

        Array<SmartPtr<ISharedObject>> CGraphicsMeshOgre::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> childObjects;
            childObjects.push_back( getStateContext() );
            childObjects.push_back( getStateListener() );

            if( auto owner = getOwner() )
            {
                childObjects.push_back( owner );
            }

            return childObjects;
        }

        SmartPtr<IGraphicsSkeleton> CGraphicsMeshOgre::getSkeleton() const
        {
            return m_skeleton;
        }

        void CGraphicsMeshOgre::setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton )
        {
            m_skeleton = skeleton;
        }

        void CGraphicsMeshOgre::materialLoaded( SmartPtr<IMaterial> material )
        {
            try
            {
                if( material )
                {
                    auto pMaterial = workphone::static_pointer_cast<CMaterialOgre>( material );
                    auto m = pMaterial->getMaterial();

                    auto it = std::find( m_materials.begin(), m_materials.end(), material );
                    auto index = static_cast<size_t>( std::distance( m_materials.begin(), it ) );
                    if( index < m_materials.size() )
                    {
                        if( auto entity = getEntity() )
                        {
                            auto subEntity = entity->getSubEntity( index );
                            subEntity->setMaterial( m );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Ogre::MeshPtr CGraphicsMeshOgre::createMesh( const String &meshName )
        {
            try
            {
                Ogre::SceneManager *smgr = nullptr;

                if( auto creator = getCreator() )
                {
                    creator->_getObject( (void **)&smgr );
                }

                auto applicationManager = core::IApplicationManager::instance();
                auto factoryManager = applicationManager->getFactoryManager();
                auto fileSystem = applicationManager->getFileSystem();

                auto meshManager = Ogre::MeshManager::getSingletonPtr();
                auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                auto meshExtention = Path::getFileExtension( meshName );
                static const auto engineMeshExtention = String( ".fbmeshbin" );
                if( meshExtention == engineMeshExtention )
                {
                    auto newMeshName = StringUtil::replaceAll( meshName, ".fbmeshbin", ".mesh" );

                    auto meshResult =
                        meshManager->createOrRetrieve( newMeshName.c_str(), resourceGroupName, true );
                    if( meshResult.second )
                    {
                        auto fbMeshStream = fileSystem->open( meshName, true, true, false, false );
                        if( !fbMeshStream )
                        {
                            fbMeshStream = fileSystem->open( meshName, true, true, false, true );
                        }

                        if( fbMeshStream )
                        {
                            MeshSerializer meshSerializer;
                            auto fbMesh = meshSerializer.loadMesh( fbMeshStream );

                            fbMesh->setName( newMeshName );

                            auto newMesh = meshResult.first.dynamicCast<Ogre::Mesh>();
                            MeshLoader::loadFBMesh( newMesh, fbMesh );
                            return newMesh;
                        }
                    }
                    else
                    {
                        auto newMesh = meshResult.first.dynamicCast<Ogre::Mesh>();
                        return newMesh;
                    }
                }
                else
                {
                    return meshManager->load( meshName.c_str(), resourceGroupName );
                }
            }
            catch( Ogre::Exception &e )
            {
                auto error = e.getFullDescription();
                WP_LOG_ERROR( error.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return Ogre::MeshPtr();
        }

        void CGraphicsMeshOgre::setupStateObject()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManager();
                WP_ASSERT( stateManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                CGraphicsObjectOgre<IGraphicsMesh>::setupStateObject();

                auto stateContext = getStateContext();

                auto state = factoryManager->make_ptr<State>();
                state->setOwner( this );
                stateContext->addState( state );

                auto stateData = factoryManager->make_ptr<GraphicsMeshState>();
                state->setData( stateData );

                auto graphicsState = factoryManager->make_ptr<State>();
                graphicsState->setOwner( this );
                stateContext->addState( graphicsState );

                auto graphicsStateData = factoryManager->make_ptr<GraphicsObjectData>();
                graphicsState->setData( graphicsStateData );

                auto stateTask = graphicsSystem->getStateTask();
                stateContext->setTaskId( stateTask );

                auto stateListener = workphone::make_ptr<MeshStateListener>();
                stateListener->setOwner( this );

                auto materialStateListener = factoryManager->make_ptr<MaterialStateListener>();
                materialStateListener->setOwner( this );
                m_materialStateListener = materialStateListener;

                m_stateListener = stateListener;
                stateContext->addStateListener( stateListener );
            }
            catch( Ogre::Exception &e )
            {
                auto error = e.getFullDescription();
                WP_LOG_ERROR( error.c_str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        CGraphicsMeshOgre::MeshStateListener::MeshStateListener() = default;

        CGraphicsMeshOgre::MeshStateListener::~MeshStateListener() = default;

        bool CGraphicsMeshOgre::MeshStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            GraphicsObjectOgreStateListener::handleStateMessage( message );

            WP_ASSERT( getOwner() );

            if( auto owner = workphone::static_pointer_cast<CGraphicsMeshOgre>( getOwner() ) )
            {
                if( message->isExactly<StateMessageMaterialName>() )
                {
                    SmartPtr<StateMessageMaterialName> materialMessage = message;
                    owner->setMaterialName( materialMessage->getMaterialName(),
                                            materialMessage->getIndex() );
                }
                else if( message->isExactly<StateMessageMaterial>() )
                {
                    auto materialMessage =
                        workphone::static_pointer_cast<StateMessageMaterial>( message );
                    auto material = materialMessage->getMaterial();
                    auto materialIndex = materialMessage->getIndex();
                    owner->setMaterial( material, materialIndex );
                }
                else if( message->isExactly<StateMessageVisible>() )
                {
                    SmartPtr<StateMessageVisible> visibleMessage = message;
                    owner->setVisible( visibleMessage->isVisible() );
                }
                else if( message->isExactly<StateMessageUIntValue>() )
                {
                    auto intValueMessage =
                        workphone::static_pointer_cast<StateMessageUIntValue>( message );
                    auto type = intValueMessage->getType();
                    auto value = intValueMessage->getValue();

                    if( type == RENDER_QUEUE_HASH )
                    {
                        owner->setRenderQueueGroup( static_cast<u8>( value ) );
                    }
                    else if( type == VISIBILITY_FLAGS_HASH )
                    {
                        owner->setVisibilityFlags( value );
                    }
                }
                else if( message->isExactly<StateMessageLoad>() )
                {
                    auto stateMessageLoad = workphone::static_pointer_cast<StateMessageLoad>( message );
                    auto eventType = stateMessageLoad->getType();

                    if( eventType == StateMessageLoad::LOADED_HASH )
                    {
                        auto materialObject = stateMessageLoad->getObject();
                        owner->materialLoaded( materialObject );
                    }
                }
            }

            return false;
        }

        bool CGraphicsMeshOgre::MeshStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            GraphicsObjectOgreStateListener::handleStateChanged( state );

            if( auto owner = workphone::static_pointer_cast<CGraphicsMeshOgre>( getOwner() ) )
            {
                const auto &loadingState = owner->getLoadingState();
                if( loadingState == LoadingState::Loaded )
                {
                    if( state->isDerived<GraphicsObjectData>() )
                    {
                        auto graphicsMeshState = SafeReadPtr<GraphicsObjectData>( state->getData() );
                        if( auto entity = owner->getEntity() )
                        {
                            entity->setVisibilityFlags( graphicsMeshState->visibilityMask );
                            entity->setRenderQueueGroup( graphicsMeshState->renderQueueGroup );
                        }
                    }
                }
            }

            return false;
        }

        CGraphicsMeshOgre::MaterialStateListener::MaterialStateListener( CGraphicsMeshOgre *owner )
        {
        }

        CGraphicsMeshOgre::MaterialStateListener::MaterialStateListener() = default;

        CGraphicsMeshOgre::MaterialStateListener::~MaterialStateListener()
        {
            m_owner = nullptr;
        }

        void CGraphicsMeshOgre::MaterialStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        bool CGraphicsMeshOgre::MaterialStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                auto materialObject = owner->getMaterial();
                owner->materialLoaded( materialObject );
            }

            return false;
        }

        bool CGraphicsMeshOgre::MaterialStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isExactly<StateMessageLoad>() )
                {
                    auto stateMessageLoad = workphone::static_pointer_cast<StateMessageLoad>( message );
                    auto eventType = stateMessageLoad->getType();

                    if( eventType == StateMessageLoad::LOADED_HASH )
                    {
                        auto materialObject = stateMessageLoad->getObject();
                        owner->materialLoaded( materialObject );
                    }
                }
            }

            return false;
        }

        SmartPtr<CGraphicsMeshOgre> CGraphicsMeshOgre::MaterialStateListener::getOwner() const
        {
            return m_owner;
        }

        void CGraphicsMeshOgre::MaterialStateListener::setOwner( SmartPtr<CGraphicsMeshOgre> owner )
        {
            m_owner = owner;
        }
    }  // end namespace render
}  // namespace workphone
