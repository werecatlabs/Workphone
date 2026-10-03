#ifndef RenderWindowState_h__
#define RenderWindowState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    class WPCore_API WindowStateData : public StateData
    {
    public:
        WindowStateData();
        ~WindowStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector2I position = Vector2I::zero();

        u32 flags = 0;

        String title;
        String windowHandle;
    };
}  // namespace workphone

#endif  // RenderWindowState_h__
