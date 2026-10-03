#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Helpers/EngineHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    core::ApplicationManager *EngineHelper::getSingleton()
    {
        auto applicationManager = core::ApplicationManager::instance();
        return static_cast<core::ApplicationManager *>( applicationManager.get() );
    }

    SmartPtr<ITimer> EngineHelper::getTimer( core::ApplicationManager *engine )
    {
        return engine->getTimer();
    }

    ITimer *EngineHelper::getTimerSingleton()
    {
        auto applicationManager = core::ApplicationManager::instance();
        auto timer = applicationManager->getTimer();
        return timer.get();
    }

    // SmartPtr<IStateManager> EngineHelper::getStateManager( core::ApplicationManager *engine )
    //{
    //     return engine->getStateManager();
    // }

    // fb::FactoryPtr EngineHelper::getFactory( Engine* engine )
    //{
    //	return engine->getFactory();
    // }

    // GraphicsSystemPtr EngineHelper::getGraphicsSystem( core::ApplicationManager *engine )
    //{
    //     return engine->getGraphicsSystem();
    // }

    // MapManagerPtr EngineHelper::getMapManager( core::ApplicationManager *engine )
    //{
    //     return engine->getMapManager();
    // }

    // EntityManagerPtr EngineHelper::getEntityManager( core::ApplicationManager *engine )
    //{
    //     return engine->getEntityManager();
    // }

    ////boost::shared_ptr<IFileSystem> EngineHelper::getFileSystem( Engine* engine )
    ////{
    ////	return engine->getFileSystem();
    ////}

    // fb::InputManagerPtr EngineHelper::getInputManager( core::ApplicationManager *engine )
    //{
    //     return engine->getInputManager();
    // }

    // fb::PhysicsManager2Ptr EngineHelper::getPhysicsManager2( core::ApplicationManager *engine )
    //{
    //     return engine->getPhysicsManager2();
    // }

    // fb::PhysicsManager3Ptr EngineHelper::getPhysicsManager3( core::ApplicationManager *engine )
    //{
    //     return engine->getPhysicsManager3();
    // }

    // fb::PoolPtr EngineHelper::getPool( core::ApplicationManager *engine )
    //{
    //     return engine->getPool();
    // }

    // fb::SmartPtr<ITimer> EngineHelper::getMainTimer( core::ApplicationManager *engine )
    //{
    //     return engine->getMainTimer();
    // }

    // fb::MessageManagerPtr EngineHelper::getMessageManager( core::ApplicationManager *engine )
    //{
    //     return engine->getMessageManager();
    // }

    // boost::shared_ptr<IComponent> EngineHelper::getComponentHash( core::ApplicationManager *system,
    // lua_Integer hash )
    //{
    //     hash32 id = *reinterpret_cast<hash32 *>( &hash );
    //     boost::shared_ptr<IComponentContainer> &components = system->getComponents();
    //     if( components )
    //     {
    //         boost::shared_ptr<IComponent> component = components->getComponentById( id );
    //         if( component )
    //         {
    //             return component;
    //         }
    //     }

    //    String msg = String( "Could find component: " ) + StringUtil::toString( id ) + String( " " );
    //    msg += Engine::getSingletonPtr()->getScriptManager()->getDebugInfo();
    //    WP_LOG_MESSAGE( "LuaScriptMgr", msg.c_str() );

    //    return nullptr;
    //}

    // boost::shared_ptr<IComponentContainer> EngineHelper::_getComponents( core::ApplicationManager
    // *system )
    //{
    //     boost::shared_ptr<IComponentContainer> &components = system->getComponents();
    //     return components;
    // }

    // boost::shared_ptr<IComponent> EngineHelper::getComponent( core::ApplicationManager *system, const
    // char *type )
    //{
    //     auto hash = StringUtil::getHash( type );
    //     lua_Integer id = *reinterpret_cast<lua_Integer *>( &hash );
    //     return getComponentHash( system, id );
    // }

    // IGraphicsCameraManager *EngineHelper::_getCameraControllerMgr( core::ApplicationManager *engine )
    //{
    //     return engine->getCameraManager().get();
    // }

    // void EngineHelper::printLuaMessage( const char *msg )
    //{
    //     printf( msg );
    // }

    // IStateManager *EngineHelper::_getStateManager( core::ApplicationManager *system )
    //{
    //     return system->getStateManager().get();
    // }

    // IGraphicsSystem *EngineHelper::_getGraphicsSystem( core::ApplicationManager *system )
    //{
    //     return system->getGraphicsSystem().get();
    // }

    // IInputManager *EngineHelper::_getInputManager( core::ApplicationManager *system )
    //{
    //     return system->getInputManager().get();
    // }

    // IEntityManager *EngineHelper::_getEntityManager( core::ApplicationManager *system )
    //{
    //     return system->getEntityManager().get();
    // }

    // IPhysicsManager2 *EngineHelper::_getPhysicsManager2( core::ApplicationManager *system )
    //{
    //     return system->getPhysicsManager2().get();
    // }

    // IPhysicsManager3 *EngineHelper::_getPhysicsManager3( core::ApplicationManager *system )
    //{
    //     return system->getPhysicsManager3().get();
    // }

    // ISoundManager *EngineHelper::_getSoundManager( core::ApplicationManager *system )
    //{
    //     return system->getSoundManager().get();
    // }

    // IFlashControlManager *EngineHelper::_getFlashControlManager( core::ApplicationManager *system )
    //{
    //     return system->getFlashControlManager().get();
    // }

    // IVideoManager *EngineHelper::_getVideoManager( core::ApplicationManager *system )
    //{
    //     return system->getVideoManager().get();
    // }

    // ICombatManager2 *EngineHelper::_getCombatManager2( core::ApplicationManager *system )
    //{
    //     return system->getCombatManager2().get();
    // }

    // IFileSystem *EngineHelper::_getFileSystem( core::ApplicationManager *system )
    //{
    //     return 0;  // system->getFileSystem().get();
    // }

    // IProjectileManager2 *EngineHelper::_getProjectileManager2( core::ApplicationManager *system )
    //{
    //     return system->getProjectileManager2().get();
    // }

    // IGameManager *EngineHelper::_getGameSceneManager( core::ApplicationManager *system )
    //{
    //     return system->getGameManager().get();
    // }

    // IGUIManager *EngineHelper::_getGUIManager( core::ApplicationManager *system )
    //{
    //     return system->getGUIManager().get();
    // }

    // IAiManager *EngineHelper::_getAiManager( core::ApplicationManager *system )
    //{
    //     return system->getAiManager().get();
    // }

    // ISpecialFxManager *EngineHelper::_getSpecialFxManager( core::ApplicationManager *system )
    //{
    //     return system->getSpecialFxManager().get();
    // }

    // IFactory* EngineHelper::_getFactory( Engine* engine )
    //{
    //	return engine->getFactory().get();
    // }
} // namespace workphone
