#ifndef StateMessageProperties_h__
#define StateMessageProperties_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageProperties : public StateMessage
    {
    public:
        StateMessageProperties();
        ~StateMessageProperties() override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<Properties> m_properties;
    };
}  // namespace workphone

#endif  // StateMessageProperties_h__
