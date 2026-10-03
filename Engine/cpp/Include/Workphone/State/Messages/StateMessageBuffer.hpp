#ifndef StateMessageBuffer_h__
#define StateMessageBuffer_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    class WPCore_API StateMessageBuffer : public StateMessage
    {
    public:
        StateMessageBuffer();
        ~StateMessageBuffer() override;

        u8 *getBuffer() const;
        void setBuffer( u8 *value );

        WP_CLASS_REGISTER_DECL;

    protected:
        u8 *m_buffer = nullptr;
    };
}  // namespace workphone

#endif  // StateMessageBuffer_h__
