#ifndef _WPOgreTypes_H_
#define _WPOgreTypes_H_

#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    enum SceneManagerType
    {
        SMT_GENERIC,
        SMT_OCTTREE,
        SMT_TERRAIN,

        SMT_COUNT
    };

    enum CoordinateSystemType
    {
        CST_LEFT_HANDED,
        CST_RIGHT_HANDED,

        CST_COUNT
    };

}  // namespace workphone

#endif
