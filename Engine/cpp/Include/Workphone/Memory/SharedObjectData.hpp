#ifndef SharedObjectData_h__
#define SharedObjectData_h__

#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <limits>
#include <memory>

namespace workphone
{

    struct SharedObjectData
    {
        ///< The event task flags.
        atomic_u32 m_eventTaskFlags = std::numeric_limits<u32>::max();

        ///< Pointer to the script data.
        AtomicWeakPtr<ISharedObject> m_scriptData;

        ///< Pointer to the shared object listener.
        AtomicRawPtr<ISharedObjectListener> m_sharedObjectListener;

        AtomicRawPtr<IEventListener> m_sharedEventListener;

        // Immutable dispatch generations. Registration admits object/control and
        // element capacity before publication; retaining a generation is free.
        std::shared_ptr<const Array<SmartPtr<IEventListener>>> m_sharedEventListeners;
        RecursiveSpinMutex m_eventListenerMutex;
    };

}  // namespace workphone

#endif  // SharedObjectData_h__
