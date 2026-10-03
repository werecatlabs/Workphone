#ifndef TypeManager_h__
#define TypeManager_h__

#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/GenericPool.hpp>
#include <Workphone/Core/GenericPoolData.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/UUID.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Memory/SharedPtr.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /**
     * @class TypeManager
     * @brief Central runtime registry and metadata store for engine types.
     *
     * The TypeManager maintains per-type metadata and runtime state for all
     * registered types in the engine. It provides stable runtime ids, name /
     * label lookup, hash resolution, base/derived relationships, instance
     * counting, grouping and storage mappings used by factories, reflection
     * and data-oriented subsystems.
     *
     * Key responsibilities:
     * - Provide stable runtime type ids and bidirectional name/hash lookups.
     * - Track immediate base types and compute inheritance relationships.
     * - Store per-type persistent hash, localized label, compact runtime index
     *   and grouping used for efficient storage and lookup.
     * - Maintain atomic live-instance counters to support concurrent object
     *   creation/destruction.
     * - Expose pools and arrays used by low-level object allocation systems.
     *
     * Thread-safety:
     * - Many query operations are safe for concurrent readers. Internal
     *   containers use atomic types and read/write locks where appropriate.
     * - Mutating operations that change internal arrays (reserve, new id,
     *   setLabel, setDataType, registerType, load/unload) are protected by
     *   internal mutexes (see members: m_nameMutex, m_labelsMutex, m_mutex).
     *
     * Lifetime / ownership:
     * - The TypeManager is designed to be used as a singleton; setInstance()
     *   / instance() manage the global pointer but do not transfer ownership.
     *   Callers remain responsible for lifetime of any injected instance.
     */
    class WPCore_API TypeManager
    {
    public:
        /**
         * @brief Construct a TypeManager.
         *
         * Initializes internal containers and pools to an empty state. This
         * constructor does not register application types � call load() to
         * perform deferred registration or pre-allocation required by the
         * application.
         */
        TypeManager();

        /**
         * @brief Destroy the TypeManager.
         *
         * Releases any owned resources. If this object is set as the global
         * singleton, callers must ensure no other threads access the manager
         * during destruction.
         */
        ~TypeManager();

        /**
         * @brief Initialize or populate internal runtime metadata.
         *
         * Use this to perform deferred initialization such as registering
         * built-in types, pre-allocating arrays/pools or otherwise preparing
         * the manager for runtime usage. Safe to call once during startup.
         *
         * After load(), type lookups and other runtime operations are expected
         * to succeed for registered types.
         */
        void load();

        /**
         * @brief Tear down runtime metadata and clear internal containers.
         *
         * Unregisters or clears stored type metadata and resets internal
         * counters. After calling unload(), the manager must not be used until
         * load() is called again. This will release internal memory and pools
         * used to track types.
         */
        void unload();

        /**
         * @brief Get the internal (non-localized) name for a type id.
         * @param id Runtime type id to query.
         * @return Pointer to a null-terminated C-string name, or nullptr if the
         *         id is invalid or the name is not set.
         *
         * The returned pointer refers to internal storage and must not be
         * freed by the caller. Use the String / FixedString wrappers if you
         * must retain the name beyond immediate use.
         *
         * Thread-safety: read-only operation; internal locking used for safe
         * concurrent reads.
         */
        const c8 *getName( u32 id ) const;

        /**
         * @brief Get a human-friendly localized label for a type id.
         * @param id Runtime type id to query.
         * @return Pointer to a null-terminated C-string label, or nullptr if no
         *         label is set.
         *
         * Labels are intended for UI and presentation and are distinct from
         * the internal type name. Returned pointer references internal
         * storage � do not free.
         */
        const c8 *getLabel( u32 id ) const;

        /**
         * @brief Set or update a human-readable label for a type id.
         * @param id Runtime type id to modify.
         * @param label Localized or user-friendly label to assign.
         *
         * Thread-safety: mutates internal label storage and is protected by an
         * internal mutex (m_labelsMutex) to allow concurrent access safely.
         * Passing an empty String effectively clears the label.
         */
        void setLabel( u32 id, const String &label );

        /**
         * @brief Get the persistent 64-bit hash associated with a type id.
         * @param hash Runtime type hash to query.
         * @return 64-bit hash value, or 0 if unset or id invalid.
         *
         * Hash values are intended for compact serialization and fast lookup.
         * Stored atomically for safe concurrent reads/writes.
         */
        hash64 getHash( u32 hash ) const;

        /**
         * @brief Get the immediate base (parent) type id for a given type.
         * @param id Runtime type id to query.
         * @return Base type id, or 0 if the type has no base or the id is invalid.
         *
         * The returned id is the direct parent in the inheritance chain � use
         * getClassHierarchy / getBaseTypes to enumerate transitive bases.
         */
        u32 getBaseType( u32 id ) const;

        /**
         * @brief Test whether two runtime ids refer to the exact same type.
         * @param a First runtime type id.
         * @param b Second runtime type id.
         * @return true if the two ids are identical and represent the same type.
         *
         * This is a simple equality check on ids and does not examine type
         * metadata.
         */
        bool isExactly( u32 a, u32 b ) const;

        /**
         * @brief Test whether one type is derived from (or equal to) another.
         * @param a Candidate derived type id.
         * @param b Candidate base type id.
         * @return true if `a` == `b` or `a` inherits (directly or indirectly)
         *         from `b`.
         *
         * This walks the immediate-base chain using getBaseType() to determine
         * inheritance relationships. Thread-safe for concurrent readers.
         */
        bool isDerived( u32 a, u32 b ) const;

        /**
         * @brief Build a textual representation of the class hierarchy.
         * @param id Runtime type id to inspect.
         * @return Ordered Array of String values representing class names from
         *         the most-derived (id) to the most-base class.
         *
         * Useful for debugging, logging and UI display. Returns an empty array
         * if id is invalid.
         */
        Array<String> getClassHierarchy( u32 id ) const;

        /**
         * @brief Build a numeric (id-based) class hierarchy.
         * @param id Runtime type id to inspect.
         * @return Ordered Array of type ids from most-derived to base classes.
         *
         * Returns an empty array if id is invalid.
         */
        Array<u32> getClassHierarchyId( u32 id ) const;

        /**
         * @brief Get a compact runtime index assigned to a type id.
         * @param id Runtime type id to query.
         * @return Compact type index value used by data-oriented systems.
         *
         * The type index is stored atomically and is intended for tightly-packed
         * storage or indexing where the full runtime id is too large.
         */
        u32 getTypeIndex( u32 id ) const;

        /**
         * @brief Get the current number of live instances of a type.
         * @param id Runtime type id to query.
         * @return Number of live instances (may be 0).
         *
         * Instance counters are stored as atomic values and updated by object
         * allocation and destruction paths. This query is thread-safe.
         */
        u32 getNumInstances( u32 id ) const;

        /**
         * @brief Get the total number of registered types.
         * @return Number of registered types currently tracked by the manager.
         *
         * Note: type ids are allocated starting from 1; the returned count
         * reflects the highest allocated id (or running count), not necessarily
         * the number of active non-empty entries.
         */
        u32 getTotalNumTypes();

        /**
         * @brief Get the custom data type id assigned to a runtime type.
         * @param id Runtime type id to query.
         * @return Custom data type id (0 if unset).
         *
         * Data types are used by data-oriented storage/serialization systems to
         * map a type to a storage or format identifier.
         */
        u32 getDataType( u32 id ) const;

        /**
         * @brief Assign a custom data type id for a runtime type.
         * @param id Runtime type id to modify.
         * @param dataType Custom data type id to assign (non-zero typical).
         *
         * Thread-safety: mutates internal per-type data and is protected by
         * internal synchronization where required.
         */
        void setDataType( u32 id, u32 dataType );

        /**
         * @brief Convenience helper to set the data type using an enum value.
         * @tparam B Type that exposes a static B::typeInfo() returning runtime id.
         * @tparam T Enum type used for dataType.
         * @param dataType Enum value to assign.
         *
         * This calls B::typeInfo() to resolve the runtime id for the compile
         * time type and then invokes setDataType(id, value).
         */
        template <class B, class T>
        void setDataTypeEnum( T dataType );

        /**
         * @brief Convenience helper to read the stored data type as an enum.
         * @tparam T Enumeration type to cast the stored data type to.
         * @param id Runtime type id to query.
         * @return Stored dataType cast to T.
         *
         * The stored value is returned as the target enum type via static_cast.
         */
        template <class T>
        T getDataTypeEnum( u32 id ) const;

        /**
         * @brief Get the registered runtime id for a compile-time C++ type.
         * @tparam T C++ type to look up.
         * @return Runtime type id associated with T, or 0 if not registered.
         *
         * Default implementation uses typeid(T).name() to resolve the name and
         * forwards to getTypeByName(). Types that provide a custom static
         * typeInfo() should be resolved via that API instead.
         */
        template <class T>
        u32 getTypeId();

        /**
         * @brief Retrieve the UUID associated with a compile-time type T.
         * @tparam T C++ type exposing static T::typeInfo() that returns runtime id.
         * @return UUID for the type, or a default/empty UUID if unknown.
         *
         * This requires the supplied type to provide a static typeInfo() method.
         */
        template <class T>
        UUID getUUID();

        /**
         * @brief Retrieve the UUID associated with a runtime type id.
         * @param id Runtime type id to query.
         * @return UUID corresponding to the type, or a default/empty UUID if
         *         the type has no associated UUID or id is invalid.
         */
        UUID getUUID( u32 id ) const;

        /**
         * @brief Resolve a runtime type id by textual name (may mutate state).
         * @param name Name to resolve.
         * @return Runtime type id for the given name, or 0 if not found.
         *
         * This method may register a new id for the given name if the
         * implementation lazily creates ids during resolution.
         */
        u32 getTypeByName( const String &name );

        /**
         * @brief Allocate and reserve a new unique runtime type id.
         * @return Newly allocated runtime type id (non-zero).
         *
         * The returned id is reserved in internal arrays and may be used by
         * the caller to register additional metadata. This increments the
         * internal type counter.
         */
        u32 getNewTypeId();

        /**
         * @brief Allocate a new runtime type id and register basic metadata.
         * @param name Name to associate with the new type id.
         * @param baseType Id of the immediate base type (0 if none).
         * @return Newly allocated runtime type id.
         *
         * This will reserve internal slots for the id and set the provided
         * name and baseType entries.
         */
        u32 getNewTypeId( const String &name, u32 baseType );

        /**
         * @brief Allocate a new type id using the textual base type name.
         * @param name Name for the new type.
         * @param baseName Name of the base type to inherit from.
         * @return Newly allocated runtime type id.
         */
        u32 getNewTypeIdFromName( const String &name, const String &baseName );

        /**
         * @brief Get a list of all immediate and transitive base type ids.
         * @param type Runtime type id to examine.
         * @return Ordered Array of base type ids (immediate base first).
         *
         * Returns an empty array when the id is invalid or no bases exist.
         */
        Array<u32> getBaseTypes( u32 type ) const;

        /**
         * @brief Get a list of base type names for a runtime type id.
         * @param type Runtime type id to examine.
         * @return Ordered Array of base type names.
         */
        Array<String> getBaseTypeNames( u32 type ) const;

        /**
         * @brief Return all runtime type ids that derive (directly/indirectly) from `type`.
         * @param type Base runtime type id to query.
         * @return Array of derived runtime type ids.
         *
         * Useful for enumerating concrete subclasses for factory registration
         * or UI. Returns empty array if none or if input id is invalid.
         */
        Array<u32> getDerivedTypes( u32 type ) const;

        void getDerivedTypes( u32 type, u32 *buffer, u32 &bufferSize, u32 maxBufferSize ) const;

        /**
         * @brief Return display names for types that derive from `type`.
         * @param type Base runtime type id to query.
         * @return Array of derived type names suitable for UI display.
         */
        Array<String> getDerivedTypeNames( u32 type ) const;

        /**
         * @brief Get the group index assigned to a runtime type id.
         * @param id Runtime type id to query.
         * @return Group index value used to partition types, or 0 if unset.
         *
         * Groups provide a smaller index range suitable for grouped storage.
         * Group indices are computed lazily and cached in m_typeGroup.
         */
        u32 getTypeGroup( u32 id ) const;

        /**
         * @brief Resolve a runtime type id from a textual name without mutating state.
         * @param name Name to resolve.
         * @return Type id for name or 0 if not found.
         *
         * Unlike getTypeByName(), this function will not allocate a new id.
         */
        u32 getIdFromName( const String &name ) const;

        /**
         * @brief Resolve a runtime type id from a stored 64-bit hash.
         * @param hash 64-bit persistent hash to resolve.
         * @return Type id for hash or 0 if not found.
         *
         * Hash table lookups are atomic and thread-safe for concurrent reads.
         */
        u32 getIdFromHash( hash64 hash ) const;

        void initialiseMemoryPools();

        /**
         * @brief Set the global singleton instance pointer.
         * @param typeManager Pointer to the TypeManager to use as global.
         *
         * This does not transfer ownership: the caller remains responsible
         * for the lifetime of the provided pointer. Use only for injection
         * (tests, alternate implementations). Thread-safety: caller must
         * ensure proper synchronization when swapping the global instance.
         */
        static void setInstance( TypeManager *typeManager );

        /**
         * @brief Get the currently set global TypeManager pointer.
         * @return Pointer to the global TypeManager (may be nullptr).
         */
        static TypeManager *instance();

        /**
         * @brief Get the reserved runtime type id used to represent plain objects.
         * @return Object type id (non-zero).
         *
         * This id is used internally to represent the base `Object`/plain
         * object type within the engine type system.
         */
        u32 getObjectId();

        /**
         * @brief Grow internal object-related arrays/pools to accommodate objects.
         * @param growSize Number of additional slots to allocate.
         *
         * Expands internal pools and arrays used to track per-object runtime
         * data. This function is used by low-level allocation systems and
         * should be called with care from threads performing allocations.
         */
        void resizeObjectData( u32 growSize );

        /**
         * @brief Register reflection metadata for a runtime type id.
         * @param id Runtime type id to associate the metadata with.
         * @param type SharedPtr to reflection::Type containing metadata.
         *
         * The manager retains the provided SharedPtr in the `types` map.
         * Thread-safety: protected by the general mutex (m_mutex) where needed.
         */
        void registerType( u32 id, SharedPtr<reflection::Type> type );

        /**
         * @brief Retrieve reflection metadata for a runtime type id.
         * @param id Runtime type id to query.
         * @return SharedPtr to reflection::Type if found, otherwise an empty SharedPtr.
         */
        SharedPtr<reflection::Type> getType( u32 id );

        /**
         * @brief Public storage and pools used by object systems.
         *
         * These members are intentionally exposed to simplify low-level
         * allocation and to avoid repeated accessor overhead. Access is thread-
         * safe only when used according to the pool/atomic semantics
         * documented on each member.
         */
        atomic_u32 m_objectDataSize = 0;   ///< Current reserved size of object data (number of slots).
        atomic_u32 m_objectDataIndex = 0;  ///< Next object data index to allocate from (atomic).
        Array<u32> iTypeInfo;  ///< Maps runtime type id to internal type info / slot (iTypeInfo[id] =>
                               ///< typeInfo).
        Array<atomic_u8>
            objectFlags;  ///< Per-object flags stored as atomic bytes (one per object slot).
        GenericPool<BaseObjectData> baseObjectDataPool;  ///< Pool of base object data instances.
        GenericPool<SharedObjectData> sharedObjectDataPool;  ///< Pool of shared object data instances.
        Array<GenericPool<BaseObjectData>>
            baseObjectDataPools;  ///< Array of base object data pools for different groups.
        Array<GenericPool<SharedObjectData>>
            sharedObjectDataPools;  ///< Array of shared object data pools for different groups.
        std::unordered_map<u32, SharedPtr<reflection::Type>>
            types;  ///< Map of runtime id -> reflection metadata. Use registerType / getType to modify.

    private:
        /**
         * @brief Get the current reserved size of internal type arrays.
         * @return Current reserved size (number of type slots).
         *
         * This returns the internal capacity used for per-type arrays.
         */
        u32 getSize() const;

        /**
         * @brief Reserve capacity for type-related arrays and initialize new slots.
         * @param size Desired number of slots to reserve.
         *
         * Ensures internal arrays are large enough to hold `size` entries.
         * This may allocate or reallocate internal containers and initialize
         * atomic entries for newly added slots.
         */
        void reserve( u32 size );

        /**
         * @brief Compute a small group index for a given runtime type id.
         * @param typeInfo Runtime type id to compute the group for.
         * @return Calculated group index used for grouped storage arrays.
         *
         * The computed group index maps a type id into a denser index space
         * suitable for bucketed/grouped storage. This may be computed lazily
         * and cached in m_typeGroup.
         */
        u32 calculateGroupIndex( u32 typeInfo ) const;

        /// Stores the internal (non-localized) names of types indexed by id.
        ConcurrentArray<FixedString<128>> m_names;

        /// Stores human-friendly (possibly localized) labels for types indexed by id.
        ConcurrentArray<FixedString<128>> m_labels;

        /// Atomic storage of 64-bit persistent type hashes indexed by id.
        Array<atomic_u64> m_hashes;

        /// Atomic storage of compact runtime type indices indexed by id.
        Array<atomic_u32> m_typeIndex;

        /// Atomic storage of computed type group indices. Mutable because some
        /// const queries compute and cache group indices lazily.
        mutable Array<atomic_u32> m_typeGroup;

        /// Mapping from hash buckets to runtime type ids (for hash -> id lookup).
        Array<atomic_u32> m_hashTypes;

        /// Atomic storage of immediate base type id for each runtime type id.
        Array<atomic_u32> m_baseType;

        /// Custom data type ids used by data-oriented systems.
        Array<atomic_u32> m_dataTypes;

        /// Atomic live-instance counters per runtime type.
        Array<atomic_u32> m_numInstances;

        /// Next available runtime type id / running type count (starts at 1).
        atomic_u32 m_typeCount = 1;

        /// Current reserved size of internal arrays.
        atomic_u32 m_size = 0;

        /// General recursive mutex for operations that require broader protection.
        mutable RecursiveSpinMutex m_mutex;

        /// Global singleton pointer for the TypeManager. Not owned by this class.
        static TypeManager *instance_;
    };

    inline TypeManager *TypeManager::instance()
    {
        return instance_;
    }

    template <class T>
    T TypeManager::getDataTypeEnum( u32 id ) const
    {
        return static_cast<T>( getDataType( id ) );
    }

    template <class T>
    u32 TypeManager::getTypeId()
    {
        // Use typeid to get a unique type identifier for type T
        const auto &typeInfo = typeid( T );

        auto name = typeInfo.name();
        return getTypeByName( name );
    }

    template <class B, class T>
    void TypeManager::setDataTypeEnum( T dataType )
    {
        auto iTypeInfo = B::typeInfo();
        setDataType( iTypeInfo, static_cast<u32>( dataType ) );
    }

    template <class T>
    UUID TypeManager::getUUID()
    {
        auto typeInfo = T::typeInfo();
        return getUUID( typeInfo );
    }

}  // namespace workphone

#endif  // TypeManager_h__
