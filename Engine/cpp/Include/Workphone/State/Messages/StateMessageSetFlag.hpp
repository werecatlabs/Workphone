#ifndef StateMessageSetFlag_h__
#define StateMessageSetFlag_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    class WPCore_API StateMessageSetFlag : public StateMessage
    {
    public:
        StateMessageSetFlag();
        ~StateMessageSetFlag() override;

        u32 getFlags() const;
        void setFlags( u32 flags );

    protected:
        u32 m_flags = 0;
    };
}  // namespace workphone

#endif  // StateMessageSetFlag_h__
