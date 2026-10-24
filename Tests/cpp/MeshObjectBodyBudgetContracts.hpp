#pragma once

#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/VertexElement.hpp>
#include <Workphone/Mesh/VertexBoneAssignment.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <thread>
#include <type_traits>
#include <stdexcept>

namespace mesh_object_body_budget_contracts
{
    using namespace workphone;
    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }
    template <class T> u64 directBytes()
    {
        return sourceBudgetStorageBytes( sizeof( T ), std::max( alignof( T ), alignof( std::max_align_t ) ) );
    }
    template <class T> u64 factoryBytes()
    {
        return sourceBudgetStorageBytes( sizeof( T ), alignof( T ) );
    }
    template <class T> u64 managerBytes()
    {
        auto manager = core::IApplicationManager::instance()->getFactoryManager();
        auto typeManager = TypeManager::instance();
        auto producer = manager->getFactoryByHash( typeManager->getHash( T::typeInfo() ) );
        return !producer ? directBytes<T>() : producer->hasPool() ? 0 : factoryBytes<T>();
    }
    template <class T> u64 constructorBytes()
    {
        if constexpr( std::is_base_of_v<VertexBuffer, T> )
            return sourceBudgetStorageBytes( sizeof( u8 * ), alignof( u8 * ) );
        else if constexpr( std::is_same_v<VertexDeclaration, T> )
            return sourceBudgetStorageBytes( 4 * sizeof( SmartPtr<IVertexElement> ), alignof( SmartPtr<IVertexElement> ) );
        else return 0;
    }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 bytes )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ), "mesh body capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template <class F> void denied( F &&operation, const char *message )
    {
        bool refused = false;
        try { operation(); } catch( const std::bad_alloc & ) { refused = true; }
        require( refused, message );
    }
    template <class T> void direct()
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, T>, "mesh bodies participate in shared Source admission" );
        Capacity capacity;
        const auto baseline = capacity.used();
        const auto body = directBytes<T>(), payload = constructorBytes<T>();
        capacity.set( baseline + body - 1 );
        denied( [] { auto object = make_ptr<T>(); }, "direct mesh body requires complete physical admission" );
        require( capacity.used() == baseline, "denied direct mesh body retains no reservation" );
        if( payload )
        {
            capacity.set( baseline + body + payload - 1 );
            denied( [] { auto object = make_ptr<T>(); }, "constructor metadata denial frees its already admitted body" );
            require( capacity.used() == baseline, "partial constructor failure retires body and owned metadata" );
        }
        capacity.set( baseline + body + payload );
        auto object = make_ptr<T>();
        require( capacity.used() == baseline + body + payload, "direct mesh body and constructor metadata charge exactly" );
        auto alias = object; object = nullptr;
        require( capacity.used() == baseline + body + payload, "retained mesh object alias keeps its physical body charged" );
        alias->unload( nullptr );
        require( capacity.used() == baseline + body, "unload retires payload while retained direct object body stays charged" );
        capacity.set( capacity.used() );
        alias = nullptr;
        require( capacity.used() == baseline, "final direct mesh owner retires physical body at the cap" );
    }
    template <class T> void factory()
    {
        Capacity capacity;
        FactoryTemplate<T> producer;
        // Loading installs the listener that returns final references through
        // freeObject. An uninitialized factory would leave dangling inventory
        // entries and use the direct-new deletion path for factory storage.
        producer.load( nullptr );
        producer.setPool( nullptr ); // Deliberately exercise its heap fallback.
        const auto baseline = capacity.used();
        const auto body = factoryBytes<T>(), payload = constructorBytes<T>();
        const auto inventories = sourceBudgetStorageBytes( sizeof( T * ), alignof( T * ) ) +
                                 sourceBudgetStorageBytes( sizeof( SmartPtr<ISharedObject> ), alignof( SmartPtr<ISharedObject> ) );
        capacity.set( baseline + body - 1 );
        denied( [&] { auto object = producer.template make_ptr<T>(); }, "factory heap fallback admits the body before publication" );
        require( capacity.used() == baseline, "denied factory heap body leaves no inventory publication" );
        capacity.set( baseline + body + payload + inventories - 1 );
        denied( [&] { auto object = producer.template make_ptr<T>(); }, "factory publication denial unwinds body and constructor metadata" );
        require( capacity.used() == baseline, "failed factory publication has no body or partial scratch leak" );
        capacity.set( baseline + body + payload + inventories );
        auto object = producer.template make_ptr<T>();
        require( !object->isPoolElement() && capacity.used() == baseline + body + payload + inventories,
                 "factory heap fallback counts actual body alignment and both inventories" );
        capacity.set( capacity.used() );
        object = nullptr;
        require( capacity.used() == baseline + inventories, "factory inventory outlives the final heap object body" );
        producer.unload( nullptr );
        require( capacity.used() == baseline, "factory teardown retires retained metadata without admission" );
    }
    template <class T> void pooled()
    {
        Capacity capacity;
        const auto baseline = capacity.used();
        auto pool = make_ptr<Pool<T>>();
        pool->setNextSize( 4 );
        auto memory = pool->allocate_object();
        require( bool( memory ), "mesh pool publishes a usable slot" );
        const auto retained = capacity.used();
        const auto slab = sourceBudgetStorageBytes( 4 * sizeof( T ), alignof( T ) );
        require( retained >= baseline + slab, "mesh pool admits the full slab including unused object slots" );
        capacity.set( retained + constructorBytes<T>() );
        auto object = new( memory.get() ) T;
        require( capacity.used() == retained + constructorBytes<T>(), "pooled placement owns metadata without duplicating slab body charge" );
        capacity.set( capacity.used() );
        object->~T();
        pool->free_object( object );
        require( capacity.used() == retained, "dead pooled object leaves the full physical slab and spare slots charged" );
        pool->clear();
        require( capacity.used() <= retained - slab, "pool clear physically retires its complete slab" );
        pool = nullptr;
        require( capacity.used() == baseline, "full-capacity pool destruction retires queue and block inventories too" );
    }
    template <class T> void coldPool()
    {
        Capacity capacity;
        const auto baseline = capacity.used();
        const auto slab = sourceBudgetStorageBytes( 4 * sizeof( T ), alignof( T ) );
        unsigned rejected = 0, accepted = 0;
        for( const auto headroom : { u64( 0 ), u64( 64 ), slab - 1, slab, slab + 128, slab + 4096 } )
        {
            auto pool = make_ptr<Pool<T>>();
            pool->setNextSize( 4 );
            const auto empty = capacity.used();
            capacity.set( empty + headroom );
            try
            {
                auto slot = pool->allocate_object();
                require( bool( slot ) && pool->getSize() == 1,
                         "admitted cold mesh pool publishes exactly one complete block" );
                pool->free_object( slot.get() );
                ++accepted;
            }
            catch( const std::bad_alloc & )
            {
                ++rejected;
                require( pool->getSize() == 0 && capacity.used() == empty,
                         "cold pool refusal leaves no published slab or partial queue/inventory capacity" );
            }
            capacity.set( capacity.used() );
            pool = nullptr;
            require( capacity.used() == baseline, "cold pool owner retirement releases every staged metadata allocation" );
            capacity.restore();
        }
        require( rejected >= 3 && accepted > 0, "cold mesh pool covers early/partial admission denial and a complete retry" );
    }
    struct LoadedApplication
    {
        SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
        LoadingState previous = application->getLoadingState();
        u8 flags = application->getObjectFlags();
        LoadedApplication()
        {
            application->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
            application->setLoadingState( LoadingState::Loaded );
            application->setObjectFlags( flags );
        }
        ~LoadedApplication()
        {
            application->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
            application->setLoadingState( previous );
            application->setObjectFlags( flags );
        }
    };
    inline void resourceNotificationDenial()
    {
        LoadedApplication loaded;
        Capacity capacity;
        const auto baseline = capacity.used();
        auto resource = make_ptr<MeshResource>();
        resource->setMesh( make_ptr<Mesh>() );
        resource->setLoadingState( LoadingState::Loaded );
        resource->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        resource->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
        require( capacity.used() == baseline + directBytes<MeshResource>() + directBytes<Mesh>(),
                 "resource shutdown fixture owns exactly its wrapper and empty mesh bodies" );
        // The detached empty Mesh stays in unload's local owner through its
        // final notification. Both notifications therefore have zero headroom;
        // no successful event job can obscure the body-retirement assertion.
        capacity.set( capacity.used() );
        const auto denials = sourceBudgetDenialCount();
        resource->unload( nullptr );
        require( !resource->getMesh() && resource->getLoadingState() == LoadingState::Unloaded &&
                     sourceBudgetDenialCount() > denials,
                 "denied loading notifications do not strand a loaded resource's mesh owner" );
        require( capacity.used() == baseline + directBytes<MeshResource>(),
                 "resource shutdown retires the mesh body while its retained wrapper remains charged" );
        resource = nullptr;
        require( capacity.used() == baseline, "last resource wrapper owner retires its physical body at capacity" );
    }
    template <class T> void check() { direct<T>(); factory<T>(); pooled<T>(); coldPool<T>(); }
    inline void run()
    {
        check<Mesh>(); check<SubMesh>(); check<VertexBuffer>(); check<IndexBuffer>();
        check<VertexDeclaration>(); check<VertexElement>(); check<VertexBoneAssignment>(); check<MeshResource>();
        resourceNotificationDenial();
        Capacity capacity;
        const auto baseline = capacity.used();
        auto object = make_ptr<VertexElement>();
        require( capacity.used() == baseline + directBytes<VertexElement>(), "cross-thread fixture charges one real body" );
        require( object->getReferences() == 1, "cross-thread fixture has one final object owner" );
        capacity.set( capacity.used() );
        // SmartPtr's const-rvalue constructor copies ownership. Mutate this
        // sole owner exclusively on the worker, then observe it only after join.
        std::thread release( [&object] { object = nullptr; } );
        release.join();
        require( !object && capacity.used() == baseline, "final direct mesh body release retires its lease on another thread" );
    }
}
