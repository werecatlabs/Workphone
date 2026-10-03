#ifndef _StateChangedMessage_H_
#define _StateChangedMessage_H_

#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageStandard : public IStateMessage
    {
    public:
        StateMessageStandard();
        explicit StateMessageStandard( u32 subjectId );
        ~StateMessageStandard() override;

        void setSubjectId( u32 id );
        u32 getSubjectId() const;

    private:
        u32 m_subjectId = 0;
    };
}  // namespace workphone

#endif
