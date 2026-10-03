#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleSystem.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleNode.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/WPOgreTypes.hpp>
#include <Workphone/Workphone.hpp>

#if WP_OGRE_USE_PARTICLE_UNIVERSE
#    include <ParticleAffectors/ParticleUniverseColourAffector.h>
#endif

namespace workphone
{
    namespace render
    {
#if WP_OGRE_USE_PARTICLE_UNIVERSE
        const hash_type PUParticleSystem::STATE_MESSAGE_RENDER_QUEUE_GROUP =
            StringUtil::getHash( "render_queue_group" );

        PUParticleSystem::PUParticleSystem()
        {
        }

        PUParticleSystem::PUParticleSystem( SmartPtr<IGraphicsScene> creator ) :
            m_particleSystem( nullptr ),
            m_creator( creator )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            //m_frameListener = SmartPtr<IFrameListener>( new ParticleSystemFrameListener( this ) );

            //SmartPtr<IStateContext> stateContext = engine->getPlatformManager()->createStateObject();
            //setStateContext( stateContext );

            //stateContext->addStateListener(
            //    SmartPtr<IStateListener>( new ParticleSystemStateListener( this ) ), true );
        }

        PUParticleSystem::~PUParticleSystem()
        {
            // RecursiveMutex::ScopedLock lock(OgreMutex);

            // auto engine = core::IApplicationManager::instance();
            // SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();

            // SmartPtr<CSceneManagerOgre> smgr = m_creator;

            // m_particleSystem->detachFromParent();

            // ParticleUniverse::ParticleSystemManager* particleSystemManager =
            // ParticleUniverse::ParticleSystemManager::getSingletonPtr();
            // particleSystemManager->destroyParticleSystem(m_particleSystem, smgr->getSceneManager());

            // smgr->_removeGraphicsObject(this);

            // if (graphicsSystem)
            //{
            //	graphicsSystem->removeFrameListener(m_frameListener);
            // }
        }

        void PUParticleSystem::initialise( ParticleUniverse::ParticleSystem *particleSystem )
        {
            // m_particleSystem = particleSystem;

            // m_templateName = m_particleSystem->getTemplateName().c_str();

            // u32 numTechniques = m_particleSystem->getNumTechniques();
            // for(u32 i=0; i<numTechniques; ++i)
            //{
            //	ParticleUniverse::ParticleTechnique* puTech = m_particleSystem->getTechnique(i);
            //	if(puTech)
            //	{
            //		SmartPtr<IParticleTechnique> techPtr(new PUParticleTechnique(puTech));
            //		m_techniques[StringUtil::getHash(puTech->getName().c_str())] = techPtr;
            //	}
            // }
        }

        void PUParticleSystem::update()
        {
            //if( m_particleSystem )
            //    m_particleSystem->_update( dt );
        }

        void PUParticleSystem::setName( const String &name )
        {
            m_name = name;
        }

        String PUParticleSystem::getName() const
        {
            return m_name;
        }

        u32 PUParticleSystem::getId() const
        {
            return 0;
        }

        void PUParticleSystem::setMaterialName( const String &materialName, s32 index )
        {
        }

        String PUParticleSystem::getMaterialName( s32 index ) const
        {
            return StringUtil::EmptyString;
        }

        void PUParticleSystem::setCastShadows( bool castShadows )
        {
        }

        bool PUParticleSystem::getCastShadows() const
        {
            return false;
        }

        void PUParticleSystem::setRecieveShadows( bool recieveShadows )
        {
        }

        bool PUParticleSystem::getRecieveShadows() const
        {
            return false;
        }

        void PUParticleSystem::setVisible( bool isVisible )
        {
            m_visible = isVisible ? TRUE : FALSE;

            //if( task != TaskId::Render )
            //{
            //    StateMessageVisiblePtr stateMessageVisible( new StateMessageVisible );
            //    stateMessageVisible->setVisible( isVisible );
            //    stateMessageVisible->setCascade( true );
            //    m_stateContext->addMessage( TaskId::Render, stateMessageVisible );
            //}
            //else
            //{
            //    if( m_particleSystem )
            //        m_particleSystem->setVisible( isVisible );
            //}
        }

        bool PUParticleSystem::isVisible() const
        {
            return m_visible ? true : false;
        }

        void PUParticleSystem::setRenderQueueGroup( u8 renderQueue )
        {
            m_renderQueueGroup = renderQueue;

            //if( task != TaskId::Render )
            //{
            //    SmartPtr<StateMessageUIntValue> stateMessage( new StateMessageUIntValue );
            //    stateMessage->setType( STATE_MESSAGE_RENDER_QUEUE_GROUP );
            //    stateMessage->setValue( renderQueue );
            //    m_stateContext->addMessage( TaskId::Render, stateMessage );
            //}
            //else
            //{
            //    if( m_particleSystem )
            //        m_particleSystem->setRenderQueueGroup( renderQueue );
            //}
        }

        void PUParticleSystem::setVisibilityFlags( u32 flags )
        {
            //if( task == TaskId::Render )
            //{
            //    if( m_particleSystem )
            //        m_particleSystem->setVisibilityFlags( flags );
            //}
            //else
            //{
            //    StateMessageTypePtr message( new StateMessageType );
            //    WP_ASSERT( message );

            //    static const hash32 VISIBILITY_FLAGS_HASH = StringUtil::getHash( "visibilityFlags" );

            //    message->setType( VISIBILITY_FLAGS_HASH );
            //    message->setTypeValue( flags );
            //    m_stateContext->addMessage( TaskId::Render, message );
            //}
        }

        u32 PUParticleSystem::getVisibilityFlags() const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            if( m_particleSystem )
            {
                return m_particleSystem->getVisibilityFlags();
            }

            return 0;
        }

        SmartPtr<IGraphicsObject> PUParticleSystem::clone( const String &name ) const
        {
            return nullptr;
        }

        void PUParticleSystem::_getObject( void **ppObject ) const
        {
            *ppObject = m_particleSystem;
        }

        void PUParticleSystem::setPropertyValue( const String &name, const String &value )
        {
        }

        bool PUParticleSystem::getPropertyValue( const String &name, String &value )
        {
            return false;
        }

        void PUParticleSystem::setProperties( const Properties &propertyGroup )
        {
            /*String value;
            propertyGroup.GetPropertyValue("BaseType", value);
            if(value==("Affector"))
            {
            propertyGroup.GetPropertyValue("TechinqueName", value);
            ParticleUniverse::ParticleTechnique* techinque = m_particleSystem->getTechnique(value.c_str());

            propertyGroup.GetPropertyValue("AffectorName", value);
            ParticleUniverse::ParticleAffector* affector = techinque->getAffector(value.c_str());

            propertyGroup.GetPropertyValue("SubType", value);
            if(value==("Colour") || value==("Color"))
            {
            ParticleUniverse::ColourAffector* colourAffector =
            static_cast<ParticleUniverse::ColourAffector*>(affector); colourAffector->clearColourMap();
            }
            }*/
        }

        bool PUParticleSystem::getProperties( Properties &propertyGroup, u32 flags ) const
        {
            return false;
        }

        void PUParticleSystem::setFastForward( f32 time, f32 interval )
        {
        }

        f32 PUParticleSystem::getFastForwardTime() const
        {
            return 0.0f;
        }

        f32 PUParticleSystem::getFastForwardInterval() const
        {
            return 0.0f;
        }

        void PUParticleSystem::setTemplateName( const String &templateName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            // Ogre::String templateNameStr = templateName.c_str();
            // m_particleSystem->setTemplateName(templateNameStr);
        }

        String PUParticleSystem::getTemplateName( void ) const
        {
            return StringUtil::EmptyString;
        }

        void PUParticleSystem::setScale( const Vector3F &scale )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            // if(m_particleSystem)
            //	m_particleSystem->setScale(Ogre::Vector3(scale));
        }

        Vector3F PUParticleSystem::getScale() const
        {
            return Vector3F::zero();
        }

        SmartPtr<IParticleTechnique> PUParticleSystem::getTechnique( const String &name ) const
        {
            ParticleUniverse::ParticleTechnique *puTech = m_particleSystem->getTechnique( name.c_str() );
            return nullptr;  // SmartPtr<IParticleTechnique>(new PUParticleTechnique(puTech), true);
        }

        ParticleUniverse::ParticleSystem *PUParticleSystem::getSystem() const
        {
            return m_particleSystem;
        }

        void PUParticleSystem::setParticleSystem( ParticleUniverse::ParticleSystem *particleSystem )
        {
            m_particleSystem = particleSystem;
        }

        const SmartPtr<IScriptInvoker> &PUParticleSystem::getInvoker() const
        {
            return m_scriptInvoker;
        }

        SmartPtr<IScriptInvoker> &PUParticleSystem::getInvoker()
        {
            return m_scriptInvoker;
        }

        void PUParticleSystem::setInvoker( SmartPtr<IScriptInvoker> invoker )
        {
            m_scriptInvoker = invoker;
        }

        const SmartPtr<IScriptReceiver> &PUParticleSystem::getReceiver() const
        {
            return m_scriptReceiver;
        }

        SmartPtr<IScriptReceiver> &PUParticleSystem::getReceiver()
        {
            return m_scriptReceiver;
        }

        void PUParticleSystem::setReceiver( SmartPtr<IScriptReceiver> receiver )
        {
            m_scriptReceiver = receiver;
        }

        s32 PUParticleSystem::getObject( u32 hash, SmartPtr<IObject> &object ) const
        {
            auto it = m_techniques.find( hash );
            if( it != m_techniques.end() )
            {
                // object = it->second;
                return 0;
            }

            return 0;
        }

        //IGraphicsSceneNode *PUParticleSystem::getOwner() const
        //{
        //    return nullptr;
        //}

        //void PUParticleSystem::setOwner( IGraphicsSceneNode *sceneNode )
        //{
        //    // SmartPtr<CSceneManagerOgre> sceneManager = m_creator;
        //    // sceneManager->registerForUpdates(this);
        //}

        void PUParticleSystem::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( !m_particleSystem )
                {
                    auto particleSystemManager =
                        ParticleUniverse::ParticleSystemManager::getSingletonPtr();
                    if( particleSystemManager )
                    {
                        Ogre::SceneManager *sceneManager = nullptr;
                        m_creator->_getObject( (void **)&sceneManager );

                        m_particleSystem = particleSystemManager->createParticleSystem(
                            m_name.c_str(), m_templateName.c_str(), sceneManager );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void PUParticleSystem::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        PUParticleSystem::ParticleSystemStateListener::ParticleSystemStateListener(
            PUParticleSystem *puSystem ) :
            m_puSystem( puSystem )
        {
        }

        bool PUParticleSystem::ParticleSystemStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // if(task == TaskId::Render)
            //{
            //	if(message->isExactly(StateMessageType::TYPE_INFO))
            //	{
            //		StateMessageTypePtr stateMessageType = message;
            //		if(stateMessageType->getType() == StringUtil::getHash("start"))
            //		{
            //			m_puSystem->start();
            //		}
            //		else if(stateMessageType->getType() == StringUtil::getHash("stop"))
            //		{
            //			m_puSystem->stop();
            //		}
            //		else if(stateMessageType->getType() == StringUtil::getHash("visibilityFlags"))
            //		{
            //			m_puSystem->setVisibilityFlags(stateMessageType->getTypeValue());
            //		}
            //	}
            //	else if(message->isExactly(StateMessageUIntValue::TYPE_INFO))
            //	{
            //		SmartPtr<StateMessageUIntValue> stateMessageValue = message;
            //		if(stateMessageValue->getType() == STATE_MESSAGE_RENDER_QUEUE_GROUP)
            //		{
            //			m_puSystem->setRenderQueueGroup(stateMessageValue->getValue());
            //		}
            //	}
            //	else if(message->isExactly(StateMessageVisible::TYPE_INFO))
            //	{
            //		StateMessageVisiblePtr visibleMessage = message;
            //		m_puSystem->setVisible(visibleMessage->isVisible());
            //	}
            // }

            return false;
        }

        bool PUParticleSystem::ParticleSystemStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

#endif
    }  // namespace render
}  // namespace workphone
