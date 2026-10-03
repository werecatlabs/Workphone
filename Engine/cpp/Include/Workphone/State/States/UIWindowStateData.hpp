#ifndef UIWindowStateData_h__
#define UIWindowStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>

namespace workphone
{
    class WPCore_API UIWindowStateData : public StateData
    {
    public:
        UIWindowStateData();
        ~UIWindowStateData() override;

        WP_CLASS_REGISTER_DECL;

        SmartPtr<ui::IUIMenu> menu;
        FixedString<64> label;
        bool border = false;
        bool docked = false;
    };

}  // namespace workphone

#endif  // UIWindowStateData_h__
