#ifndef StateMessageBlendMapValue_h__
#define StateMessageBlendMapValue_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{

    class WPCore_API StateMessageBlendMapValue : public StateMessage
    {
    public:
        StateMessageBlendMapValue();
        ~StateMessageBlendMapValue() override;

        Vector2I getCoordinates() const;
        void setCoordinates( const Vector2I &value );

        f32 getBlendValue() const;
        void setBlendValue( f32 value );

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector2I m_coordinates;
        f32 m_blendValue;
    };
}  // namespace workphone

#endif  // StateMessageBlendMapValue_h__
