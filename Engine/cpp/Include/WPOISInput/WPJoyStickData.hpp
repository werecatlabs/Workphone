#ifndef FBJoyStickData_h__
#define FBJoyStickData_h__

#include <WPOISInput/WPOISInputPrerequisites.hpp>
#include "Workphone/WorkphoneTypes.hpp"

namespace workphone
{
    struct JoyStickData
    {
        JoyStickData();

        hash32 m_gameInputId = 0;
        u32 m_joyStickIdx = 0;
    };
}  // namespace workphone

#endif  // FBJoyStickData_h__
