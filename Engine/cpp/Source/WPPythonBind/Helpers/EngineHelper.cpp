#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/Helpers/EngineHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{
    SmartPtr<IApplicationManager> EngineHelper::instance()
    {
        auto applicationManager = IApplicationManager::instance();
        return applicationManager;
    }

    SmartPtr<ITimer> EngineHelper::getTimer( IApplicationManager *engine )
    {
        return engine->getTimer();
    }

    SmartPtr<IStateManager> EngineHelper::getStateManager( IApplicationManager *engine )
    {
        return engine->getStateManager();
    }

    SmartPtr<IFactory> EngineHelper::getFactory( IApplicationManager *engine )
    {
        return nullptr; //engine->getFactoryManager();
    }

    SmartPtr<render::IGraphicsSystem> EngineHelper::getGraphicsSystem( IApplicationManager *engine )
    {
        return engine->getGraphicsSystem();
    }

    //MapManagerPtr EngineHelper::getMapManager( IApplicationManager *engine )
    //{
    //    return engine->getMapManager();
    //}

    SmartPtr<scene::ISceneManager> EngineHelper::getEntityManager( IApplicationManager *engine )
    {
        return nullptr; // engine->getEntityManager();
    }

    SmartPtr<IFileSystem> EngineHelper::getFileSystem( IApplicationManager *engine )
    {
        return engine->getFileSystem();
    }

    SmartPtr<IInputDeviceManager> EngineHelper::getInputManager( IApplicationManager *engine )
    {
        return nullptr; // engine->getInputManager();
    }

    SmartPtr<physics::IPhysicsManager2D> EngineHelper::getPhysicsManager2( IApplicationManager *engine )
    {
        return nullptr; //engine->getPhysicsManager2();
    }

    SmartPtr<physics::IPhysicsManager> EngineHelper::getPhysicsManager3( IApplicationManager *engine )
    {
        return nullptr; //engine->getPhysicsManager3();
    }
} // end namespace fb
