#ifndef _WP_Function_h__
#define _WP_Function_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <functional>

namespace workphone
{
    template <class T>
    using Function = std::function<T>;
}  // namespace workphone

#endif  // _WP_Function_h__
