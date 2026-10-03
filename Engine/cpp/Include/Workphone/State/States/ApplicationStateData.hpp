#ifndef ApplicationState_h__
#define ApplicationState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class WPCore_API ApplicationStateData : public StateData
    {
    public:
        ApplicationStateData();
        ~ApplicationStateData() override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // ApplicationState_h__
