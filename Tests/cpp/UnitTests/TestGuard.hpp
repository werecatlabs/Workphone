#ifndef TestGuard_h__
#define TestGuard_h__

#include <Workphone/Workphone.hpp>
#include <functional>
#include <vector>

namespace workphone
{

    struct TestGuard
    {
        explicit TestGuard( bool requirePhysics = false );

        ~TestGuard();

        void addCleanup( std::function<void()> cleanup );

        void trackFilesystemPath( const String &path );

        void trackPhysicsActor( SmartPtr<physics::IPhysicsScene3> scene,
                                SmartPtr<physics::IPhysicsBody3> body );

        template <class T>
        void releaseInitialReference( T *ptr )
        {
            addCleanup( [ptr]() {
                if( ptr )
                {
                    ptr->removeReference();
                }
            } );
        }

        void setupThread();

        void resetTimer();

        void updateScene( u32 iterations = 10 );

        void updatePhysics( u32 iterations = 10 );

        void runUpdateCycle( int frameCount = 1 );

        SmartPtr<scene::IGameActor> createBasicActor( bool isStatic = false );

        void cleanup();

        bool isAvailable = false;
        bool requiresPhysics = false;
        SmartPtr<core::IApplicationManager> applicationManager;
        SmartPtr<IFactoryManager> factoryManager;
        SmartPtr<IFileSystem> fileSystem;
        SmartPtr<IStateManager> stateManager;
        SmartPtr<IResourceDatabase> resourceDatabase;
        TypeManager *typeManager = nullptr;
        SmartPtr<ITimer> timer;
        SmartPtr<physics::IPhysicsManager> physicsManager;
        SmartPtr<scene::IGameManager> sceneManager;
        SmartPtr<scene::IGameScene> scene;
        SmartPtr<ITaskManager> taskManager;
        SmartPtr<render::IGraphicsSystem> graphicsSystem;
        std::vector<std::function<void()>> cleanupActions;
    };

}  // namespace workphone

#endif  // TestGuard_h__
