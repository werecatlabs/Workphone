#ifndef WP_OverlayTextState_H
#define WP_OverlayTextState_H

#include <Workphone/State/States/OverlayElementState.hpp>

namespace workphone
{
    class WPCore_API OverlayTextState : public OverlayElementState
    {
    public:
        OverlayTextState();
        ~OverlayTextState() override;

        u32 alignment = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // WP_OVERLAYSTATE_H
