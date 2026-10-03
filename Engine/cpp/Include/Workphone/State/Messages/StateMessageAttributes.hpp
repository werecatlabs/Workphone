#ifndef StateMessageAttributes_h__
#define StateMessageAttributes_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    class WPCore_API StateMessageAttributes : public StateMessage
    {
    public:
    protected:
        int m_iValue;
        f32 m_fValue;
        Vector3<real_Num> m_vector3Value;
        Vector4F m_vector4Value;
        String m_sValue;
    };
}  // namespace workphone

#endif  // StateMessageAttributes_h__
