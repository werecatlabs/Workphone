#ifndef UIDragState_h__
#define UIDragState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>

namespace workphone
{

    class UIDragStateData : public StateData
    {
    public:
        UIDragStateData();
        ~UIDragStateData() override;

        SmartPtr<ui::IUIDragSource> dragSource;
        SmartPtr<ui::IUIDropTarget> dropTarget;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // UIDragState_h__
