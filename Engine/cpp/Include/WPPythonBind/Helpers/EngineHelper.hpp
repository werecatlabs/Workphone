#ifndef EngineHelper_h__
#define EngineHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>

namespace fb
{
    using namespace core;

    class EngineHelper
    {
    public:
        static SmartPtr<core::IApplicationManager> instance();

        static SmartPtr<ITimer> getTimer( IApplicationManager *engine );

        static SmartPtr<IStateManager> getStateManager( IApplicationManager *engine );

        static SmartPtr<IFactory> getFactory( IApplicationManager *engine );
        static SmartPtr<IFileSystem> getFileSystem( IApplicationManager *engine );

        static SmartPtr<render::IGraphicsSystem> getGraphicsSystem( IApplicationManager *engine );
        static SmartPtr<IInputDeviceManager> getInputManager( IApplicationManager *engine );
        //static MapManagerPtr getMapManager( IApplicationManager *engine );

        static SmartPtr<scene::ISceneManager> getEntityManager( IApplicationManager *engine );

        static SmartPtr<physics::IPhysicsManager2D> getPhysicsManager2( IApplicationManager *engine );

        static SmartPtr<physics::IPhysicsManager> getPhysicsManager3( IApplicationManager *engine );
    };

}  // end namespace fb

#endif  // EngineHelper_h__
