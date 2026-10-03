#ifndef StateMessageDirty_h__
#define StateMessageDirty_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageDirty : public StateMessage
    {
    public:
        StateMessageDirty();
        ~StateMessageDirty() override;

        bool isDirty() const;
        void setDirty( bool dirty );

        WP_CLASS_REGISTER_DECL;

    protected:
        bool m_isDirty = true;
    };
}  // namespace workphone

#endif  // StateMessageDirty_h__
