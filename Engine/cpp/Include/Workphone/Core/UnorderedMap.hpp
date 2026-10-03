#ifndef UnorderedMap_h__
#define UnorderedMap_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <unordered_map>

namespace workphone
{
    template <class T, class B>
    using UnorderedMap = std::unordered_map<T, B>;
}  // namespace workphone

#endif  // UnorderedMap_h__
