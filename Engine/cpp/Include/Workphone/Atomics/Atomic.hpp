#ifndef Atomic_h__
#define Atomic_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>

namespace workphone
{

    template <typename T>
    class Atomic : public std::atomic<T>
    {
    };

}  // namespace workphone

#endif  // Atomic_h__
