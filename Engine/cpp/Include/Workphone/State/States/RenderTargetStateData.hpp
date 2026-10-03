#ifndef RenderTargetStateData_h__
#define RenderTargetStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{

    class WPCore_API RenderTargetStateData : public StateData
    {
    public:
        RenderTargetStateData();
        ~RenderTargetStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector2I size = Vector2I( 128, 128 );

        u32 colourDepth = 32;
        u16 priority = 0;
        u8 flags = 0;

        Array<SmartPtr<render::IViewport>> viewports;
    };

}  // namespace workphone

#endif  // RenderTargetStateData_h__
