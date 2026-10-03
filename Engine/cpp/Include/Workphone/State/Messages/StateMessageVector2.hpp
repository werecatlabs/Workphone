#ifndef StateMessageVector2_h__
#define StateMessageVector2_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    template <class T>
    class StateMessageVector2 : public StateMessage
    {
    public:
        StateMessageVector2();

        ~StateMessageVector2() override;

        Vector2<T> getValue() const;

        void setValue( const Vector2<T> &value );

        WP_CLASS_REGISTER_TEMPLATE_DECL( StateMessageVector2, StateMessage );

    protected:
        Vector2<T> m_value = Vector2<T>::zero();
    };

    template <class T>
    StateMessageVector2<T>::StateMessageVector2() = default;

    template <class T>
    StateMessageVector2<T>::~StateMessageVector2() = default;

    template <class T>
    Vector2<T> StateMessageVector2<T>::getValue() const
    {
        return m_value;
    }

    template <class T>
    void StateMessageVector2<T>::setValue( const Vector2<T> &value )
    {
        m_value = value;
    }

    WP_CLASS_REGISTER_DERIVED_TEMPLATE_TYPEID( workphone, StateMessageVector2, StateMessage,
                                               StateMessage );

    using StateMessageVector2I = StateMessageVector2<s32>;
    using StateMessageVector2F = StateMessageVector2<f32>;
}  // namespace workphone

#endif  // StateMessageVector2_h__
