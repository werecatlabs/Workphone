#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{
    // Fixed-width types are instantiated in AtomicNumber.cpp. atomic_s8 uses plain char.
    template class AtomicNumber<c8>;
}  // namespace workphone
