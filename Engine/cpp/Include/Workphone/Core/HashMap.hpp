#ifndef HashMap_h__
#define HashMap_h__

#include <unordered_map>

namespace workphone
{
    template <class T, class B>
    using HashMap = std::unordered_map<T, B>;

    template <class T, class B>
    using UnorderedMap = std::unordered_map<T, B>;
}  // namespace workphone

#endif  // HashMap_h__
