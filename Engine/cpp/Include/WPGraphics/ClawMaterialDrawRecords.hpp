#ifndef WP_CLAW_MATERIAL_DRAW_RECORDS_HPP
#define WP_CLAW_MATERIAL_DRAW_RECORDS_HPP

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawDrawSort.hpp>
#include <WPGraphics/ClawMaterialStorage.hpp>
#include <unordered_map>
#include <vector>
#include <memory_resource>
struct wp_graphics_cull_result;

namespace workphone::render
{
    class ClawMesh;

    /** Owner-thread builder, then immutable scene-job publication. Draws borrow
     * dense records; ownership is acquired once per distinct material. No native
     * views are retained. The publication must outlive every borrowed draw.
     */
    class WPGraphics_API ClawMaterialDrawRecords
    {
    public:
        struct Binding
        {
            u64 publication = 0;
            const ClawMesh *mesh = nullptr;
            u32 first = 0, count = 0;
        };
        struct Record
        {
            IMaterial *material = nullptr;
            const MaterialRenderState *state = nullptr;
            ClawMaterialRenderCache::Handle handle;
            const ClawPreparedMaterialDX11 *prepared = nullptr;
        };

        ClawMaterialDrawRecords();
        ~ClawMaterialDrawRecords();
        ClawMaterialDrawRecords( const ClawMaterialDrawRecords & ) = delete;
        ClawMaterialDrawRecords &operator=( const ClawMaterialDrawRecords & ) = delete;
        void capture( ClawMesh *mesh );
        void seal( ClawMaterialRenderCache *cache = nullptr );
        /** Reorder a private view before publication. Unknown/mixed materials,
         * refraction, UI and nonstandard depth tests are barriers. Missing cameras
         * retain order. Invalid policies/failed view allocation return false;
         * key-storage allocation may throw. Failure preserves the input view. */
        bool sortView( wp_graphics_cull_result *&view, ClawDrawSortStrategy, double bucketSize = 10.0 );
        // Retire owners and invalidate old bindings, retaining reusable capacity.
        void clear();
        const Binding *find( const ClawMesh *mesh ) const;
        bool valid( const Binding &, const ClawMesh *, u32 sections ) const;
        const Record *get( const Binding &, u32 section ) const;

        size_t getRecordCount() const { return m_records.size(); }
        size_t getBindingCount() const { return m_bindings.size(); }
        size_t getMeshCount() const { return m_meshes.size(); }
        u64 getPreparationCount() const { return m_preparations; }
        /** Actual vector capacity bytes for records/owners/section indices.
         * Excludes lookup maps, snapshots/resources and allocator bookkeeping. */
        u64 getDenseCapacityBytes() const;
        /** Table storage only, including pool metadata and lookup maps. Counts
         * actual upstream requests; excludes resource objects and allocator overhead. */
        u64 getStorageAllocationCount() const { return m_storage.allocations; }
        u64 getRetainedStorageBytes() const { return m_storage.bytes; }
        u64 getPeakStorageBytes() const { return m_storage.peak; }
        // Shared table bytes: deduplicate by table identity when summing views.
        u64 getPersistentStorageBytes() const { return m_preparedRecords.getStorageBytes(); }
        u64 getPersistentTableIdentity() const { return m_preparedRecords.getIdentity(); }

    private:
        struct Owner
        {
            SmartPtr<IMaterial> material;
            MaterialRenderState::Ptr state;
        };
        u32 captureMaterial( SmartPtr<IMaterial> material );
        struct StorageCounters { u64 allocations = 0, bytes = 0, peak = 0; };
        class StorageResource final : public std::pmr::memory_resource
        {
        public:
            StorageResource( StorageCounters &counters, bool cache ) : m_counters( counters ), m_cache( cache ) {}
            ~StorageResource() override;
        private:
            struct Block
            {
                Block *allNext, *freeNext;
                size_t requested, alignment, total, allocationAlignment;
            };
            Block *m_blocks = nullptr, *m_free = nullptr;
            StorageCounters &m_counters;
            bool m_cache;
            static size_t payloadOffset( size_t alignment );
            void *do_allocate( size_t bytes, size_t alignment ) override;
            void do_deallocate( void *pointer, size_t bytes, size_t alignment ) override;
            bool do_is_equal( const std::pmr::memory_resource &other ) const noexcept override
            { return this == &other; }
        };
        // Resources outlive their containers and the lazy node pool. Typed
        // allocators keep Debug iterator proxies out of the budgeted PMR path;
        // actual nodes, slabs, vectors and buckets remain admitted upstream.
        StorageCounters m_storage;
        StorageResource m_poolStorage{ m_storage, true }, m_vectorStorage{ m_storage, false };
        detail::MaterialStoragePool m_pool{ m_poolStorage };
        template<class T> using Storage = std::vector<T, detail::MaterialStorageAllocator<T>>;
        template<class K, class V> using Lookup = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>,
            detail::MaterialStorageAllocator<std::pair<const K, V>>>;
        // Large arrays bypass chunk caching: geometric growth must not retain
        // every former vector allocation alongside the current capacity.
        Storage<Record> m_records{ &m_vectorStorage };
        Storage<Owner> m_owners{ &m_vectorStorage };
        Storage<u32> m_bindings{ &m_vectorStorage };
        Storage<ClawDrawSortKey> m_sortKeys{ &m_vectorStorage };
        Storage<int32_t> m_sortOrder{ &m_vectorStorage };
        Lookup<const IMaterial *, u32> m_materials{ &m_pool };
        Lookup<const ClawMesh *, Binding> m_meshes{ &m_pool };
        u64 m_publication = 0, m_preparations = 0;
        bool m_sealed = false;
        ClawMaterialRenderCache::Lease m_preparedRecords;
    };
}
#endif
