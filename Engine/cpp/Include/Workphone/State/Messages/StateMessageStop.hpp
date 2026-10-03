#ifndef StateMessageStop_h__
#define StateMessageStop_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageStop : public StateMessage
    {
    public:
        StateMessageStop() = default;
        ~StateMessageStop() override = default;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // StateMessageStop_h__
