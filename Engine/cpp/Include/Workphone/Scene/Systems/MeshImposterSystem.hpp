#pragma once

#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <array>
#include <memory>
#include <mutex>

namespace workphone::scene
{
    class MeshImposter;

    /** LOD-task update owner and paged storage for MeshImposter component state.
     * Scene-owned instances track components through ComponentSystem. instance()
     * is the shared metadata store, including configuration of unloaded components.
     * Pages keep references stable during reentrant scene publication. Released
     * slots retain no material or frame data and are reused without allocation.
     */
    class WPCore_API MeshImposterSystem final : public ComponentSystem
    {
    public:
        MeshImposterSystem();
        ~MeshImposterSystem() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        TaskId getUpdateTask() const override;
        u32 addComponent( SmartPtr<IComponent> component ) override;
        void removeComponent( SmartPtr<IComponent> component ) override;
        void removeComponent( u32 id ) override;

        WP_CLASS_REGISTER_DECL;

        struct Handle
        {
            u32 index = static_cast<u32>( -1 );
            u64 generation = 0;
        };

        /** Process-lifetime metadata has no ISharedObject or TypeManager-pool
         * state. Only scene-owned update systems participate in that lifecycle. */
        class WPCore_API Storage final
        {
        public:
            Storage();
            ~Storage();
            Storage( const Storage & ) = delete;
            Storage &operator=( const Storage & ) = delete;

            Handle create();
            void destroy( Handle handle );
            bool contains( Handle handle ) const;
            size_t getActiveCount() const;
            size_t getSlotCount() const;

        private:
            friend class MeshImposter;
            static constexpr u32 pageSize = 32;
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

            mutable std::recursive_mutex m_storageMutex;
            Array<std::unique_ptr<Page>> m_pages{ Allocator<std::unique_ptr<Page>>( sourceBudget ) };
            u32 m_freeHead = invalidIndex;
            size_t m_activeCount = 0;
        };

        static Storage &instance();

    protected:
        void reserveData( size_t size ) override;

    private:
        static constexpr u32 invalidIndex = static_cast<u32>( -1 );

        Array<time_interval> m_nextUpdates{ Allocator<time_interval>( sourceBudget ) };
        time_interval m_lastUpdateTime = 0;
    };
}  // namespace workphone::scene
