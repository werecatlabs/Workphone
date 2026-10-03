#ifndef UILayoutState_h__
#define UILayoutState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    class WPCore_API UILayoutStateData : public StateData
    {
    public:
        static const u32 dirtyFlag;

        UILayoutStateData();
        ~UILayoutStateData() override;

        ISharedObject *uiComponent = nullptr;
        ISharedObject *owner = nullptr;

        atomic_u8 flags = 0;

        atomic_u8 verticalAlignment = 0;
        atomic_u8 horizontalAlignment = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // UILayoutState_h__
