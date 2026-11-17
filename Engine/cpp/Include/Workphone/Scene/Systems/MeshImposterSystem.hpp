#pragma once

#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <array>
#include <memory>
#include <mutex>

namespace workphone::scene
{
    class MeshImposter;

    /** Paged structure-of-arrays storage for MeshImposter component state.
     * Pages keep references stable during reentrant scene publication. Released
     * slots retain no material or frame data and are reused without allocation.
     */
    class WPCore_API MeshImposterSystem final
    {
    public:
        struct Handle
        {
            u32 index = static_cast<u32>( -1 );
            u64 generation = 0;
        };

        static MeshImposterSystem &instance();
        Handle create();
        void destroy( Handle handle );
        bool contains( Handle handle ) const;
        size_t getActiveCount() const;
        size_t getSlotCount() const;

    private:
        friend class MeshImposter;
        static constexpr u32 pageSize = 32;
        static constexpr u32 invalidIndex = static_cast<u32>( -1 );
        using Views = Array<MeshImposterView>;
        struct Page;
        struct Access
        {
            std::unique_lock<std::recursive_mutex> lock;
            Views &views;
            SmartPtr<render::IMaterial> &material;
            Vector3F &centre;
            s32 &selectedView;
            u64 &selectionChanges;
        };
        Access access( Handle handle );
        bool containsUnlocked( Handle handle ) const;

        mutable std::recursive_mutex m_mutex;
        Array<std::unique_ptr<Page>> m_pages{ Allocator<std::unique_ptr<Page>>( sourceBudget ) };
        u32 m_freeHead = invalidIndex;
        size_t m_activeCount = 0;
        MeshImposterSystem();
        ~MeshImposterSystem();
    };
}
