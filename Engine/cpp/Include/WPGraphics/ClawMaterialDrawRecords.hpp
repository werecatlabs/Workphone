#ifndef WP_CLAW_MATERIAL_DRAW_RECORDS_HPP
#define WP_CLAW_MATERIAL_DRAW_RECORDS_HPP

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawDrawSort.hpp>
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
            ClawPreparedMaterialDX11 prepared;
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
        // Declaration order keeps the resource alive until every container and
        // pool allocation has been released. Only small blocks use the pool;
        // large vector/bucket arrays retain their normal reusable capacities.
        StorageCounters m_storage;
        StorageResource m_poolStorage{ m_storage, true }, m_vectorStorage{ m_storage, false };
        std::pmr::unsynchronized_pool_resource m_pool{ std::pmr::pool_options{ 32, 512 }, &m_poolStorage };
        // Large arrays bypass chunk caching: geometric growth must not retain
        // every former vector allocation alongside the current capacity.
        std::pmr::vector<Record> m_records{ &m_vectorStorage };
        std::pmr::vector<Owner> m_owners{ &m_vectorStorage };
        std::pmr::vector<u32> m_bindings{ &m_vectorStorage };
        std::pmr::vector<ClawDrawSortKey> m_sortKeys{ &m_vectorStorage };
        std::pmr::vector<int32_t> m_sortOrder{ &m_vectorStorage };
        std::pmr::unordered_map<const IMaterial *, u32> m_materials{ &m_pool };
        std::pmr::unordered_map<const ClawMesh *, Binding> m_meshes{ &m_pool };
        u64 m_publication = 0, m_preparations = 0;
        bool m_sealed = false;
    };
}
#endif
