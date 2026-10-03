#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Atomics/AtomicNumber.hpp>

namespace workphone
{

    // Explicit instantiations (bool omitted – unsupported)
    template class AtomicNumber<s8>;
    template class AtomicNumber<u8>;
    template class AtomicNumber<s16>;
    template class AtomicNumber<u16>;
    template class AtomicNumber<s32>;
    template class AtomicNumber<u32>;
    template class AtomicNumber<s64>;
    template class AtomicNumber<u64>;

}  // namespace workphone
