#ifndef EngineHelper_h__
#define EngineHelper_h__

#include <Workphone/System/ApplicationManager.hpp>

namespace workphone
{
    class EngineHelper
    {
    public:
        static core::ApplicationManager *getSingleton();

        static SmartPtr<ITimer> getTimer( core::ApplicationManager *engine );

        static ITimer *getTimerSingleton();

        // static SmartPtr<IStateManager> getStateManager( core::ApplicationManager *engine );

        ////static FactoryPtr getFactory(Engine* engine);
        // static boost::shared_ptr<IFileSystem> getFileSystem( core::ApplicationManager *engine );

        // static GraphicsSystemPtr getGraphicsSystem( core::ApplicationManager *engine );
        // static InputManagerPtr getInputManager( core::ApplicationManager *engine );
        // static MapManagerPtr getMapManager( core::ApplicationManager *engine );

        // static EntityManagerPtr getEntityManager( core::ApplicationManager *engine );

        // static PhysicsManager2Ptr getPhysicsManager2( core::ApplicationManager *engine );

        // static PhysicsManager3Ptr getPhysicsManager3( core::ApplicationManager *engine );

        // static PoolPtr getPool( core::ApplicationManager *engine );

        // static SmartPtr<ITimer> getMainTimer( core::ApplicationManager *engine );

        // static MessageManagerPtr getMessageManager( core::ApplicationManager *engine );

        // static boost::shared_ptr<IComponent> getComponentHash( core::ApplicationManager *system,
        // lua_Integer hash );

        // static boost::shared_ptr<IComponentContainer> _getComponents( core::ApplicationManager *system
        // );

        // static boost::shared_ptr<IComponent> getComponent( core::ApplicationManager *system, const
        // char *type );

        // static IGraphicsCameraManager *_getCameraControllerMgr( core::ApplicationManager *engine );

        // static void printLuaMessage( const char *msg );

        // static IStateManager *_getStateManager( core::ApplicationManager *system );

        // static IGraphicsSystem *_getGraphicsSystem( core::ApplicationManager *system );

        // static IInputManager *_getInputManager( core::ApplicationManager *system );

        // static IEntityManager *_getEntityManager( core::ApplicationManager *system );

        // static IPhysicsManager2 *_getPhysicsManager2( core::ApplicationManager *system );

        // static IPhysicsManager3 *_getPhysicsManager3( core::ApplicationManager *system );

        // static ISoundManager *_getSoundManager( core::ApplicationManager *system );

        // static IFlashControlManager *_getFlashControlManager( core::ApplicationManager *system );

        // static IVideoManager *_getVideoManager( core::ApplicationManager *system );

        // static ICombatManager2 *_getCombatManager2( core::ApplicationManager *system );

        // static IFileSystem *_getFileSystem( core::ApplicationManager *system );

        // static IProjectileManager2 *_getProjectileManager2( core::ApplicationManager *system );

        // static IGameManager *_getGameSceneManager( core::ApplicationManager *system );

        // static IGUIManager *_getGUIManager( core::ApplicationManager *system );

        // static IAiManager *_getAiManager( core::ApplicationManager *system );

        // static ISpecialFxManager *_getSpecialFxManager( core::ApplicationManager *system );

        // static IFactory* _getFactory(Engine* engine);
    };
} // namespace workphone

#endif // EngineHelper_h__
