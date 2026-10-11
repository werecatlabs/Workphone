#pragma once
#include <Workphone/System/TaskManager.hpp>
#include <Workphone/Database/AssetImportQueue.hpp>
#include <Workphone/System/ThreadPool.hpp>
#include <Workphone/System/JobQueue.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>

namespace
{
    class ImportSchedulingTasks : public workphone::TaskManager
    {
    public:
        workphone::u32 getNumTasks() const override
        {
            return 1;
        }
    };

    void assetImportQueueContracts()
    {
        workphone::AssetImportQueue serial;
        std::promise<void> entered, release;
        auto resume = release.get_future().share();
        std::vector<int> order;
        require( serial.enqueue( [&] {
            order.push_back( 1 );
            entered.set_value();
            resume.wait();
        } ),
                 "First import must reserve the sole drainer" );
        auto drain = std::async( std::launch::async, [&] { serial.drain(); } );
        entered.get_future().wait();
        const bool second = serial.enqueue( [&] { order.push_back( 2 ); } );
        const bool third = serial.enqueue( [&] { order.push_back( 3 ); } );
        release.set_value();
        drain.get();
        require( !second && !third && order == std::vector<int>{ 1, 2, 3 },
                 "Concurrent reimport admission must return immediately and preserve FIFO batch order" );
        require( serial.enqueue( [&] {
            order.push_back( 4 );
            require( !serial.enqueue( [&] { order.push_back( 5 ); } ),
                     "Reentrant submission must not start another drainer" );
        } ),
                 "Completed queue must admit a new drainer" );
        serial.drain();
        require( order == std::vector<int>{ 1, 2, 3, 4, 5 },
                 "Reentrant requests must run after their current batch" );
    }

    class CleanupProbe : public workphone::Resource<workphone::IResource>
    {
    public:
        workphone::SmartPtr<workphone::ResourceDatabase> database;
        std::shared_ptr<std::atomic<int>> calls;
        std::shared_ptr<std::atomic<bool>> unlocked;
        void unload( workphone::SmartPtr<workphone::ISharedObject> ) override
        {
            auto signal = std::make_shared<std::promise<void>>();
            auto done = signal->get_future();
            std::thread reader( [owner = database, signal] {
                owner->getMediaCatalog();  // Acquires ResourceDatabase::m_mutex on another thread.
                signal->set_value();
            } );
            const bool ready = done.wait_for( std::chrono::seconds( 1 ) ) == std::future_status::ready;
            if( ready )
                reader.join();
            else
                reader.detach();
            *unlocked = ready;
            ++*calls;
        }
    };

    void assetImportSchedulingContracts( const std::filesystem::path &folder )
    {
        using namespace workphone;
        assetImportQueueContracts();
        auto app = core::IApplicationManager::instance();
        const auto root = folder / "import-scheduling";
        std::filesystem::create_directories( root );
        struct Restore
        {
            core::IApplicationManager *app;
            String project, cache, settings;
            bool running, quit;
            SmartPtr<IResourceDatabase> resources;
            SmartPtr<IThreadPool> pool;
            SmartPtr<IJobQueue> queue;
            SmartPtr<ITaskManager> tasks;
            SmartPtr<scene::IGamePrefabManager> prefabs;
            ~Restore()
            {
                app->setResourceDatabase( resources );
                app->setThreadPool( pool );
                app->setJobQueue( queue );
                app->setTaskManager( tasks );
                app->setPrefabManager( prefabs );
                app->setProjectPath( project );
                app->setCachePath( cache );
                app->setSettingsPath( settings );
                app->setRunning( running );
                app->setQuit( quit );
            }
        } restore{ app.get(),
                   app->getProjectPath(),
                   app->getCachePath(),
                   app->getSettingsPath(),
                   app->isRunning(),
                   app->getQuit(),
                   app->getResourceDatabase(),
                   app->getThreadPool(),
                   app->getJobQueue(),
                   app->getTaskManager(),
                   app->getPrefabManager() };
        const String sourceRoot = root.generic_u8string().c_str();
        app->setProjectPath( sourceRoot );
        app->setCachePath( sourceRoot );
        app->setSettingsPath( sourceRoot + "/settings/" );
        app->setRunning( true );
        app->setQuit( false );
        app->setPrefabManager( make_ptr<scene::GamePrefabManager>() );
        app->setTaskManager( make_ptr<ImportSchedulingTasks>() );
        auto pool = make_ptr<ThreadPool>();
        pool->setNumThreads( 2 );  // Advertised workers deliberately cannot drain this producer's queue.
        app->setThreadPool( pool );
        auto queue = make_ptr<JobQueue>();
        queue->setRunning( true );
        app->setJobQueue( queue );
        auto resources = make_ptr<ResourceDatabase>();
        resources->load( nullptr );
        app->setResourceDatabase( resources );
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() );
        require( catalog->setProjectRoot( sourceRoot ), "Import fixture root must bind" );
        catalog->loadFromFile( sourceRoot + "/asset.db" );
        auto import = [&]( const char *name ) {
            std::ofstream( root / name ) << "return {}";
            resources->importFile( sourceRoot + "/" + name, true );
        };
        // Two workers have a four-job budget. Retaining four jobs reproduces
        // the old indefinite producer wait without starting real worker threads.
        Array<SmartPtr<ResourceDatabase::ImportFileJob>> retained;
        for( int i = 0; i < 4; ++i )
            retained.push_back( make_ptr<ResourceDatabase::ImportFileJob>() );
        import( "saturated.lua" );
        require( queue->hasJobs(), "Import admission must return without waiting for retained jobs" );
        pool->setNumThreads( 0 );
        queue->update();
        require( !queue->hasJobs() && catalog->getResourceEntryFromPath( "saturated.lua" ),
                 "Retained jobs must not prevent the serial importer from making progress" );
        retained.clear();
        pool->setNumThreads( 1 );
        import( "single-worker.lua" );
        require( !queue->hasJobs() && catalog->getResourceEntryFromPath( "single-worker.lua" ),
                 "The only worker must not queue imports behind itself" );
        pool->setNumThreads( 0 );
        import( "primary.lua" );
        require( !queue->hasJobs() && catalog->getResourceEntryFromPath( "primary.lua" ),
                 "Primary-only imports must not accumulate a queue requiring that same thread" );
        pool->setNumThreads( 2 );
        import( "queued.lua" );
        require( queue->hasJobs() && !catalog->getResourceEntryFromPath( "queued.lua" ),
                 "Spare worker capacity must retain asynchronous imports" );
        pool->setNumThreads( 0 );  // Drain with the primary-thread queue consumer.
        queue->update();
        require( !queue->hasJobs() && catalog->getResourceEntryFromPath( "queued.lua" ),
                 "Queued imports must complete after the producer releases the consumer" );
        const auto identity = entryId( catalog->getResourceEntryFromPath( "saturated.lua" ) );
        import( "saturated.lua" );
        require( entryId( catalog->getResourceEntryFromPath( "saturated.lua" ) ) == identity,
                 "Inline reimport must retain the authored UUID" );
        // Requests accepted before a project switch must not write into the next session.
        pool->setNumThreads( 2 );
        import( "stale.lua" );
        app->setProjectPath( sourceRoot + "/next-project" );
        pool->setNumThreads( 0 );
        queue->update();
        app->setProjectPath( sourceRoot );
        require( !catalog->getResourceEntryFromPath( "stale.lua" ),
                 "Stale project import must be rejected" );
        pool->setNumThreads( 2 );
        import( "cancelled.lua" );
        queue->shutdown();
        queue->setRunning( true );
        pool->setNumThreads( 0 );
        import( "after-cancel.lua" );
        require( catalog->getResourceEntryFromPath( "after-cancel.lua" ) != nullptr,
                 "Cancelling a scheduled pump must release import admission" );

        // Destructive reimport must preserve the live catalog and unrelated cache files.
        std::filesystem::create_directories( root / "Assets" );
        std::ofstream( root / "Assets" / "retained.lua" ) << "return {}";
        resources->importFile( sourceRoot + "/Assets/retained.lua", true );
        const auto retainedId = entryId( catalog->getResourceEntryFromPath( "Assets/retained.lua" ) );
        for( auto name : { "asset.db-wal", "asset.db-shm", "unknown.bin", "source.png",
                           "generated.fbmeshbin", "generated.pxtrianglemesh" } )
            if( std::string( name ).find( "asset.db" ) != 0 )
                std::ofstream( root / name ) << "payload";
        pool->setNumThreads( 2 );
        resources->importFile( sourceRoot + "/Assets/retained.lua", true );
        resources->reimportAssets();
        require( std::filesystem::exists( root / "generated.fbmeshbin" ),
                 "Queued reimport must not clean before draining" );
        pool->setNumThreads( 0 );
        queue->update();
        require( std::filesystem::exists( root / "asset.db" ) &&
                     std::filesystem::exists( root / "unknown.bin" ) &&
                     std::filesystem::exists( root / "source.png" ) &&
                     !std::filesystem::exists( root / "generated.fbmeshbin" ) &&
                     !std::filesystem::exists( root / "generated.pxtrianglemesh" ) &&
                     entryId( catalog->getResourceEntryFromPath( "Assets/retained.lua" ) ) == retainedId,
                 "Reimport must preserve catalog/source bytes and identities, removing only known "
                 "generated payloads" );
        auto bound = dynamic_cast<IParameterizedDatabase *>( catalog->getDatabase().get() );
        auto writer = app->getFactoryManager()->make_object<IDatabase>();
        writer->loadFromFile( sourceRoot + "/asset.db" );
        auto writerSql = dynamic_cast<IParameterizedDatabase *>( writer.get() );
        require( writerSql->queryBound( "BEGIN IMMEDIATE", {} ) != nullptr,
                 "Competing writer fixture must acquire its transaction" );
        const auto started = std::chrono::steady_clock::now();
        const bool busyRejected = bound->queryBound( "BEGIN IMMEDIATE", {} ) == nullptr;
        const auto elapsed = std::chrono::steady_clock::now() - started;
        writerSql->queryBound( "ROLLBACK", {} );
        require( busyRejected && elapsed < std::chrono::seconds( 5 ),
                 "Writer contention must fail promptly rather than stall for a minute" );
        require( bound->queryBound( "BEGIN IMMEDIATE", {} ) && bound->queryBound( "COMMIT", {} ),
                 "Timed-out writes must not poison the catalog connection" );
        writer->unload( nullptr );

        const auto originalThread = Thread::getCurrentThreadId();
        Thread::setCurrentThreadId( Thread::ThreadId::Primary );
        auto calls = std::make_shared<std::atomic<int>>( 0 );
        auto unlocked = std::make_shared<std::atomic<bool>>( false );
        auto instances = workphone::make_shared<Array<SmartPtr<IResource>>>();
        auto probe = make_ptr<CleanupProbe>();
        probe->database = resources;
        probe->calls = calls;
        probe->unlocked = unlocked;
        instances->push_back( probe );
        resources->setInstancesPtr( instances );
        probe = nullptr;
        resources->unloadUnusedResources();
        require( *calls == 1 && *unlocked && instances->size() == 1,
                 "Unload callbacks must run without holding the database mutex" );
        resources->removeUnusedResources();
        require( *calls == 2 && *unlocked && instances->empty(),
                 "Eviction must detach under lock and run callbacks after releasing it" );
        resources->setInstancesPtr( nullptr );
        Thread::setCurrentThreadId( originalThread );
        resources->unload( nullptr );
        catalog->unload( nullptr );
        queue->setRunning( false );
        std::cout << "PASS: saturated, single-worker and primary import progress; queued completion and "
                     "reimport identity\n";
    }
}  // namespace
