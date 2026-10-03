#ifndef WP_OVERLAYSTATE_H
#define WP_OVERLAYSTATE_H

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class WPCore_API OverlayState : public StateData
    {
    public:
        OverlayState();
        ~OverlayState() override;

        bool isVisible() const;

        void setVisible( bool visible );

        WP_CLASS_REGISTER_DECL;

        atomic_bool m_visible = true;
    };
}  // namespace workphone

#endif  // WP_OVERLAYSTATE_H
