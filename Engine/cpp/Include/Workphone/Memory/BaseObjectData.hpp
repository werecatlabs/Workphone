#ifndef BaseObjectData_h__
#define BaseObjectData_h__

#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/UnorderedMap.hpp>

namespace workphone
{

    struct BaseObjectData
    {
        /// Hash value representing the object type (used for runtime type identification).
        Handle m_handle;

        /// Numeric id for the object instance (used by higher-level registries).
        atomic_u32 m_objectId = 0;

        /// Optional user data pointer for the creator of the object (can be used for tracking or
        /// callbacks).
        void *m_pUserData = nullptr;

        /// Optional pointer supplied by the creator of the object (e.g. a factory or owning subsystem).
        void *m_creatorData = nullptr;

        /// Hash identifying the factory that produced this object.
        hash_type m_factoryData = 0;

        ///< The object name.
        FixedString<256> m_objectName;

        ///< Keyed user data storage (lazily allocated).
        UnorderedMap<hash_type, void *> *m_userData = nullptr;

#if !WP_FINAL
        /// Debug string for the object (used in non-final builds for diagnostics).
        String m_debugStr;

        /// Optional debug name for the object (used in non-final builds for diagnostics).
        c8 *m_debugName = nullptr;
#endif
    };

}  // namespace workphone

#endif  // BaseObjectData_h__
