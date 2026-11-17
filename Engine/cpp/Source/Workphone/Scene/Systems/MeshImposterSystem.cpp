#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Systems/MeshImposterSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <stdexcept>

namespace workphone::scene
{
    struct MeshImposterSystem::Page : SourceBudgetObject
    {
        std::array<Views, pageSize> views;
        std::array<SmartPtr<render::IMaterial>, pageSize> materials;
        std::array<Vector3F, pageSize> centres{};
        std::array<s32, pageSize> selectedViews;
        std::array<u64, pageSize> selectionChanges{};
        std::array<u64, pageSize> generations{};
        std::array<u8, pageSize> active{};
        std::array<u32, pageSize> nextFree{};

        Page()
        {
            selectedViews.fill( -1 );
            for( auto &frames : views ) frames = Views( Allocator<MeshImposterView>( sourceBudget ) );
        }
    };

    MeshImposterSystem::MeshImposterSystem() = default;
    MeshImposterSystem::~MeshImposterSystem() = default;

    MeshImposterSystem &MeshImposterSystem::instance()
    {
        static MeshImposterSystem system;
        return system;
    }

    MeshImposterSystem::Handle MeshImposterSystem::create()
    {
        std::lock_guard<std::recursive_mutex> lock( m_mutex );
        if( m_freeHead == invalidIndex )
        {
            if( m_pages.size() >= invalidIndex / pageSize ) throw std::length_error( "Mesh imposter slots exhausted" );
            const auto base = static_cast<u32>( m_pages.size() ) * pageSize;
            auto page = std::make_unique<Page>();
            for( u32 i = 0; i < pageSize; ++i )
                page->nextFree[i] = i + 1 < pageSize ? base + i + 1 : invalidIndex;
            m_pages.push_back( std::move( page ) );
            m_freeHead = base;
        }
        const auto index = m_freeHead;
        auto &page = *m_pages[index / pageSize];
        const auto slot = index % pageSize;
        m_freeHead = page.nextFree[slot];
        page.active[slot] = 1;
        ++m_activeCount;
        return { index, page.generations[slot] };
    }

    bool MeshImposterSystem::containsUnlocked( Handle handle ) const
    {
        if( handle.index / pageSize >= m_pages.size() ) return false;
        const auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        return page.active[slot] && page.generations[slot] == handle.generation;
    }

    void MeshImposterSystem::destroy( Handle handle )
    {
        // Material teardown can enter graphics. Retire resource owners after
        // releasing storage, preserving the publication lock order.
        Views retiredViews{ Allocator<MeshImposterView>( sourceBudget ) };
        SmartPtr<render::IMaterial> retiredMaterial;
        std::lock_guard<std::recursive_mutex> lock( m_mutex );
        if( !containsUnlocked( handle ) ) return;
        auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        page.views[slot].swap( retiredViews );
        retiredMaterial = page.materials[slot];
        page.materials[slot] = nullptr;
        page.centres[slot] = Vector3F::zero();
        page.selectedViews[slot] = -1;
        page.selectionChanges[slot] = 0;
        page.active[slot] = 0;
        ++page.generations[slot];
        page.nextFree[slot] = m_freeHead;
        m_freeHead = handle.index;
        --m_activeCount;
    }

    MeshImposterSystem::Access MeshImposterSystem::access( Handle handle )
    {
        std::unique_lock<std::recursive_mutex> lock( m_mutex );
        if( !containsUnlocked( handle ) ) throw std::logic_error( "Stale mesh imposter storage handle" );
        auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        return { std::move( lock ), page.views[slot], page.materials[slot], page.centres[slot],
            page.selectedViews[slot], page.selectionChanges[slot] };
    }

    bool MeshImposterSystem::contains( Handle handle ) const
    {
        std::lock_guard<std::recursive_mutex> lock( m_mutex );
        return containsUnlocked( handle );
    }

    size_t MeshImposterSystem::getActiveCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_mutex );
        return m_activeCount;
    }

    size_t MeshImposterSystem::getSlotCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_mutex );
        return m_pages.size() * pageSize;
    }
}
