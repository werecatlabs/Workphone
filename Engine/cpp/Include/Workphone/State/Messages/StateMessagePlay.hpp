#ifndef StateMessagePlay_h__
#define StateMessagePlay_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessagePlay : public StateMessage
    {
    public:
        StateMessagePlay();
        ~StateMessagePlay() override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // StateMessagePlay_h__
