#ifndef GraphicsSystemHelper_h__
#define GraphicsSystemHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class GraphicsSystemHelper
    {
    public:
        static SmartPtr<render::IGraphicsSceneManager> addSceneManager( SmartPtr<render::IGraphicsSystem> graphicsSystem, const char *type,
                                                const char *name );
        static SmartPtr<render::IGraphicsSceneManager> getSceneManager( SmartPtr<render::IGraphicsSystem> gfxSystem,
                                                const char *sceneManagerName );

        static SmartPtr<render::IWindow> getRenderWindow( SmartPtr<render::IGraphicsSystem> sys );
        static SmartPtr<render::IWindow> getRenderWindowNamed( SmartPtr<render::IGraphicsSystem> sys, const char *name );

        static SmartPtr<render::IDeferredShadingSystem> getDeferredShadingSystem(
            SmartPtr<render::IGraphicsSystem> sys );

        static SmartPtr<render::IDebug> getDebug(
            SmartPtr<render::IGraphicsSystem> sys );
    };

}  // end namespace fb

#endif  // GraphicsSystemHelper_h__
