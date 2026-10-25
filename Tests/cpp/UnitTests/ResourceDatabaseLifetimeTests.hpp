#ifndef WP_RESOURCE_DATABASE_LIFETIME_TESTS_HPP
#define WP_RESOURCE_DATABASE_LIFETIME_TESTS_HPP

#include "MeshImportStreamingIntegrationTests.hpp"
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <mutex>
#include <new>
#if defined( WP_PLATFORM_WIN32 )
#include <Windows.h>
#endif

namespace resource_database_lifetime_tests
{
    using namespace workphone;

    // Retain only virtual address reservations after deletion on Windows. Any
    // stale intrusive weak-pointer access then fails deterministically, rather
    // than depending on whether the heap has reused the released object body.
    class HeapResource : public MeshResource
    {
    public:
        inline static std::array<void *, 16> reservations{};
        inline static size_t allocated = 0;
        inline static size_t destroyed = 0;
        inline static size_t freed = 0;

        ~HeapResource() override { ++destroyed; }

        static void *operator new( size_t size )
        {
            if( allocated == reservations.size() ) throw std::bad_alloc();
#if defined( WP_PLATFORM_WIN32 )
            auto memory = VirtualAlloc( nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            if( !memory ) throw std::bad_alloc();
#else
            auto memory = ::operator new( size );
#endif
            reservations[allocated++] = memory;
            return memory;
        }
        static void operator delete( void *memory ) noexcept
        {
#if defined( WP_PLATFORM_WIN32 )
            if( VirtualFree( memory, 0, MEM_DECOMMIT ) ) ++freed;
#else
            ::operator delete( memory );
            ++freed;
#endif
        }
    };

    struct AllocationGuard
    {
        AllocationGuard()
        {
            HeapResource::allocated = HeapResource::destroyed = HeapResource::freed = 0;
            HeapResource::reservations.fill( nullptr );
        }
        ~AllocationGuard()
        {
#if defined( WP_PLATFORM_WIN32 )
            for( auto memory : HeapResource::reservations )
                if( memory ) VirtualFree( memory, 0, MEM_RELEASE );
#endif
        }
    };

    // Exercise the public database routing against an independently owned
    // manager inventory. Retirement releases its physical heap allocation;
    // neither the catalog nor the database is allowed to keep it alive.
    class HeapMeshManager : public MeshManager
    {
    public:
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override
        {
            return createOrRetrieve( "", path, "" );
        }
        Pair<SmartPtr<IResource>, bool> createOrRetrieve(
            const String &uuid, const String &path, const String &type ) override
        {
            std::lock_guard<std::mutex> lock( mutex );
            if( owned && ( owned->getFilePath() == path ||
                ( !uuid.empty() && owned->getHandle()->getUUIDAsString() == uuid ) ) )
                return { owned, false };
            // make_ptr transfers the initial intrusive reference into the
            // manager owner; wrapping raw new would leave that reference live.
            owned = make_ptr<HeapResource>();
            owned->setFilePath( path );
            owned->getHandle()->setUUID( uuid.empty() ? StringUtil::getUUID() : uuid );
            return { owned, true };
        }
        void retire()
        {
            SmartPtr<IResource> retired;
            {
                std::lock_guard<std::mutex> lock( mutex );
                retired = owned;
                owned = nullptr;
            }
            retired = nullptr;
        }
        void unload( SmartPtr<ISharedObject> data ) override
        {
            retire();
            MeshManager::unload( data );
        }
    private:
        std::mutex mutex;
        SmartPtr<IResource> owned;
    };

    inline SmartPtr<HeapMeshManager> installManager(
        mesh_import_streaming_integration_tests::Fixture &fixture )
    {
        auto manager = make_ptr<HeapMeshManager>();
        auto app = fixture.guard.applicationManager;
        auto previous = app->getMeshManager();
        fixture.guard.addCleanup( [app, manager, previous]() mutable {
            manager->unload( nullptr );
            app->setMeshManager( previous );
        } );
        app->setMeshManager( manager );
        return manager;
    }

    inline void checkRetired( void *address, size_t count )
    {
        BOOST_REQUIRE_EQUAL( HeapResource::destroyed, count );
        BOOST_REQUIRE_EQUAL( HeapResource::freed, count );
#if defined( WP_PLATFORM_WIN32 )
        MEMORY_BASIC_INFORMATION information{};
        BOOST_REQUIRE( VirtualQuery( address, &information, sizeof( information ) ) );
        BOOST_CHECK_EQUAL( information.State, static_cast<DWORD>( MEM_RESERVE ) );
#endif
    }
}

BOOST_AUTO_TEST_SUITE( ResourceDatabaseLifetimeTests )

BOOST_AUTO_TEST_CASE( released_manager_resource_can_be_retrieved_and_database_unloaded )
{
    using namespace resource_database_lifetime_tests;
    AllocationGuard allocations;
    mesh_import_streaming_integration_tests::Fixture fixture;
    auto manager = installManager( fixture );
    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( fixture.database->getDatabaseManager() );
    BOOST_REQUIRE( catalog );
    const auto uuid = StringUtil::getUUID();
    auto seed = make_ptr<MeshResource>();
    seed->setFilePath( fixture.source ); seed->getHandle()->setUUID( uuid );
    catalog->addResourceEntry( seed ); seed = nullptr;
    auto director = dynamic_pointer_cast<scene::ResourceDirector>( catalog->getResourceEntryFromPath( fixture.source ) );
    BOOST_REQUIRE( director ); BOOST_REQUIRE_EQUAL( director->getResourceUUID(), uuid );

    auto first = fixture.database->createOrRetrieve( IMeshResource::typeInfo(), fixture.source );
    BOOST_REQUIRE( first.first ); BOOST_CHECK( first.second );
    BOOST_CHECK_EQUAL( first.first->getHandle()->getUUIDAsString(), uuid );
    BOOST_CHECK_EQUAL( first.first->getWeakReferences(), 0 );
    auto same = fixture.database->createOrRetrieve( fixture.source );
    BOOST_CHECK( same.first == first.first ); BOOST_CHECK( !same.second );
    auto retiredAddress = first.first.get();
    first.first = nullptr; same.first = nullptr; manager->retire();
    checkRetired( retiredAddress, 1 );

    auto reloaded = fixture.database->createOrRetrieve( fixture.source );
    BOOST_REQUIRE( reloaded.first ); BOOST_CHECK( reloaded.second );
    BOOST_CHECK( reloaded.first.get() != retiredAddress );
    BOOST_CHECK_EQUAL( reloaded.first->getHandle()->getUUIDAsString(), uuid );
    retiredAddress = reloaded.first.get(); reloaded.first = nullptr; manager->retire();
    checkRetired( retiredAddress, 2 );
    fixture.database->unload( nullptr );
    BOOST_CHECK( !fixture.database->isLoaded() );
}

BOOST_AUTO_TEST_CASE( uncataloged_identity_routes_without_catalog_or_graphics )
{
    using namespace resource_database_lifetime_tests;
    AllocationGuard allocations;
    mesh_import_streaming_integration_tests::Fixture fixture;
    auto manager = installManager( fixture );
    const String path = "uncataloged.unsupported-extension";
    auto first = fixture.database->createOrRetrieve( IMeshResource::typeInfo(), path );
    BOOST_REQUIRE( first.first ); BOOST_CHECK( first.second );
    const auto uuid = first.first->getHandle()->getUUIDAsString();
    BOOST_REQUIRE( !uuid.empty() );
    auto app = fixture.guard.applicationManager;
    auto catalog = fixture.database->getDatabaseManager(); auto graphics = app->getGraphicsSystem();
    fixture.guard.addCleanup( [app, database = fixture.database, catalog, graphics]() mutable {
        app->setGraphicsSystem( graphics ); database->setDatabaseManager( catalog );
    } );
    fixture.database->setDatabaseManager( nullptr ); app->setGraphicsSystem( nullptr );
    auto scheduler = WorldStreamingScheduler::shared();
    const auto stats = scheduler->stats();
    const auto previousLimit = stats.limits[size_t( StreamMemoryClass::SourceCpu )];
    struct RestoreLimit
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
        ~RestoreLimit() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
    } restore{ scheduler, previousLimit };
    String error; error.reserve( 1024 );
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu,
        stats.used[size_t( StreamMemoryClass::SourceCpu )], error ) );
    const auto denialsBefore = sourceBudgetDenialCount();
    auto same = fixture.database->createOrRetrieve( path );
    const auto denialsAfter = sourceBudgetDenialCount();
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, previousLimit, error ) );
    BOOST_CHECK( same.first == first.first ); BOOST_CHECK( !same.second );
    BOOST_CHECK_EQUAL( denialsAfter, denialsBefore );
    auto retiredAddress = first.first.get();
    first.first = nullptr; same.first = nullptr; manager->retire();
    checkRetired( retiredAddress, 1 );
    auto reloaded = fixture.database->createOrRetrieve( path );
    BOOST_REQUIRE( reloaded.first ); BOOST_CHECK( reloaded.second );
    BOOST_CHECK_EQUAL( reloaded.first->getHandle()->getUUIDAsString(), uuid );
    retiredAddress = reloaded.first.get(); reloaded.first = nullptr; manager->retire();
    checkRetired( retiredAddress, 2 );
    fixture.database->unload( nullptr );
    app->setGraphicsSystem( graphics ); fixture.database->setDatabaseManager( catalog );
}

BOOST_AUTO_TEST_SUITE_END()
#endif
