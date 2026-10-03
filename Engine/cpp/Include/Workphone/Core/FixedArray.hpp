#ifndef FBFixedSizeArray_h__
#define FBFixedSizeArray_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <array>

namespace workphone
{
    template <class T, std::size_t N>
    using FixedArray = std::array<T, N>;
}  // namespace workphone

#endif  // FBFixedSizeArray_h__
