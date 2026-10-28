#ifndef GraphicsSystemHelper_h__
#define GraphicsSystemHelper_h__

#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>

namespace workphone
{
    class GraphicsSystemHelper
    {
    public:
        /** Snapshot of live player graphics policy, including display capabilities. */
        static SmartPtr<Properties> getRuntimeSettings( render::IGraphicsSystem *system );
        /** Validate and apply a staged policy, retaining unrelated player settings. */
        static SmartPtr<Properties> applyRuntimeSettings( render::IGraphicsSystem *system,
                                                          SmartPtr<Properties> requested );
        /** Flat mode.N.width/height properties avoid unregistered array bindings. */
        static SmartPtr<Properties> getDisplayModes( render::IGraphicsSystem *system );
    };
}  // namespace workphone

#endif  // GraphicsSystemHelper_h__
