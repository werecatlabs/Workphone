#ifndef StateMessagePair_h__
#define StateMessagePair_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    template <class T, class U>
    class StateMessagePair : public StateMessage
    {
    public:
        StateMessagePair() = default;
        ~StateMessagePair() override = default;

        T getFirst() const
        {
            return m_first;
        }

        void setFirst( const T &value )
        {
            m_first = value;
        }

        U getSecond() const
        {
            return m_second;
        }

        void setSecond( const U &value )
        {
            m_second = value;
        }

        WP_CLASS_REGISTER_TEMPLATE_PAIR_DECL( StateMessagePair, T, U );

    protected:
        T m_first;
        U m_second;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE_PAIR_TYPEID( workphone, StateMessagePair, T, U, StateMessage );

}  // namespace workphone

#endif  // StateMessagePair_h__
