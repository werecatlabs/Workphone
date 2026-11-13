#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class RegistryCallbackComponent : public Component
    {
    public:
        std::function<void()> callback;
        void preUpdate() override { if( callback ) callback(); }
        void update() override { if( callback ) callback(); }
        void postUpdate() override { if( callback ) callback(); }
    };

    // Reusable rendezvous: registration and removal have a known completed
    // boundary, without depending on sleeps or the scheduler's thread order.
    class RegistryBarrier
    {
    public:
        explicit RegistryBarrier( unsigned participants ) : m_participants( participants ) {}

        void wait()
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            const auto generation = m_generation;
            if( ++m_arrived == m_participants )
            {
                m_arrived = 0;
                ++m_generation;
                m_ready.notify_all();
            }
            else
            {
                m_ready.wait( lock, [&]() { return generation != m_generation; } );
            }
        }

    private:
        const unsigned m_participants;
        unsigned m_arrived = 0;
        unsigned m_generation = 0;
        std::mutex m_mutex;
        std::condition_variable m_ready;
    };

    struct RegistryFixture : TestGuard
    {
        std::vector<SmartPtr<IComponent>> owned;

        SmartPtr<RegistryCallbackComponent> component()
        {
            auto result = make_ptr<RegistryCallbackComponent>();
            owned.push_back( result );
            return result;
        }

        ~RegistryFixture()
        {
            for( auto &component : owned ) sceneManager->unregisterAllComponent( component );
            Thread::setCurrentTask( TaskId::Primary );
        }

        size_t count( Thread::UpdateState phase, TaskId task, const SmartPtr<IComponent> &component )
        {
            const auto snapshot = sceneManager->getRegisteredComponents( phase, task ).snapshot();
            return std::count( snapshot.begin(), snapshot.end(), component );
        }
    };
}  // namespace

BOOST_FIXTURE_TEST_SUITE( GameManagerRegistrationTests, RegistryFixture )

BOOST_AUTO_TEST_CASE( repeated_registration_is_unique_per_phase_and_task )
{
    auto value = component();
    const Thread::UpdateState phases[] = { Thread::UpdateState::PreUpdate,
        Thread::UpdateState::Transform, Thread::UpdateState::Update, Thread::UpdateState::PostUpdate };
    for( const auto phase : phases )
    {
        for( const auto task : { TaskId::Application, TaskId::Render } )
        {
            for( unsigned attempt = 0; attempt < 10; ++attempt )
                sceneManager->registerComponentUpdate( task, phase, value );
            BOOST_CHECK_EQUAL( count( phase, task, value ), 1 );
        }
    }
    sceneManager->unregisterComponentUpdate( TaskId::Application, Thread::UpdateState::Update, value );
    sceneManager->unregisterComponentUpdate( TaskId::Application, Thread::UpdateState::Update, value );
    BOOST_CHECK_EQUAL( count( Thread::UpdateState::Update, TaskId::Application, value ), 0 );
    BOOST_CHECK_EQUAL( count( Thread::UpdateState::Update, TaskId::Render, value ), 1 );
    BOOST_CHECK_EQUAL( count( Thread::UpdateState::PreUpdate, TaskId::Application, value ), 1 );

    sceneManager->unregisterAllComponent( value );
    for( const auto phase : phases )
        for( const auto task : { TaskId::Application, TaskId::Render } )
            BOOST_CHECK_EQUAL( count( phase, task, value ), 0 );
}

BOOST_AUTO_TEST_CASE( concurrent_registration_and_removal_publish_unique_snapshots )
{
    constexpr unsigned workerCount = 8;
    constexpr unsigned rounds = 64;
    const auto phase = Thread::UpdateState::Transform;
    const auto task = TaskId::Render;
    auto common = component();
    std::vector<SmartPtr<IComponent>> distinct;
    for( unsigned index = 0; index < workerCount; ++index ) distinct.push_back( component() );
    RegistryBarrier barrier( workerCount );
    std::atomic<unsigned> invalidSnapshots{ 0 };
    std::vector<std::thread> workers;

    for( unsigned index = 0; index < workerCount; ++index )
    {
        workers.emplace_back( [&, index]() {
            for( unsigned round = 0; round < rounds; ++round )
            {
                barrier.wait();
                sceneManager->registerComponentUpdate( task, phase, common );
                sceneManager->registerComponentUpdate( task, phase, distinct[index] );
                sceneManager->registerComponentUpdate( task, phase, common );
                barrier.wait();
                if( index == 0 )
                {
                    const auto snapshot = sceneManager->getRegisteredComponents( phase, task ).snapshot();
                    if( std::count( snapshot.begin(), snapshot.end(), SmartPtr<IComponent>( common ) ) != 1 )
                        ++invalidSnapshots;
                    for( const auto &item : distinct )
                        if( std::count( snapshot.begin(), snapshot.end(), item ) != 1 ) ++invalidSnapshots;
                }
                barrier.wait();
                // These operations overlap on the same list, including the
                // same component, as load completion and scene teardown do.
                for( unsigned attempt = 0; attempt < 16; ++attempt )
                {
                    if( ( index + attempt ) % 2 == 0 )
                        sceneManager->unregisterComponentUpdate( task, phase, common );
                    else
                        sceneManager->registerComponentUpdate( task, phase, common );
                    const auto snapshot = sceneManager->getRegisteredComponents( phase, task ).snapshot();
                    std::unordered_set<IComponent *> unique;
                    for( const auto &item : snapshot )
                        if( !unique.insert( item.get() ).second ) ++invalidSnapshots;
                }
                barrier.wait();
                sceneManager->unregisterComponentUpdate( task, phase, common );
                sceneManager->unregisterComponentUpdate( task, phase, distinct[index] );
                barrier.wait();
                if( index == 0 )
                {
                    const auto snapshot = sceneManager->getRegisteredComponents( phase, task ).snapshot();
                    if( std::count( snapshot.begin(), snapshot.end(), SmartPtr<IComponent>( common ) ) != 0 )
                        ++invalidSnapshots;
                    for( const auto &item : distinct )
                        if( std::count( snapshot.begin(), snapshot.end(), item ) != 0 ) ++invalidSnapshots;
                }
                barrier.wait();
            }
        } );
    }
    for( auto &worker : workers ) worker.join();
    BOOST_CHECK_EQUAL( invalidSnapshots.load(), 0 );
}

BOOST_AUTO_TEST_CASE( callbacks_can_replace_registration_without_changing_the_current_snapshot )
{
    const auto task = sceneManager->getStateTask();
    Thread::setCurrentTask( task );
    scene->setSceneLoadingState( IGameScene::SceneLoadingState::Loaded );
    for( const auto phase : { Thread::UpdateState::PreUpdate, Thread::UpdateState::Update,
                             Thread::UpdateState::PostUpdate } )
    {
        auto first = component();
        auto second = component();
        auto replacement = component();
        unsigned firstCalls = 0, secondCalls = 0, replacementCalls = 0;
        first->callback = [&]() {
            ++firstCalls;
            sceneManager->unregisterComponentUpdate( task, phase, first );
            sceneManager->registerComponentUpdate( task, phase, replacement );
        };
        second->callback = [&]() { ++secondCalls; };
        replacement->callback = [&]() { ++replacementCalls; };
        sceneManager->registerComponentUpdate( task, phase, first );
        sceneManager->registerComponentUpdate( task, phase, second );

        const auto update = [&]() {
            if( phase == Thread::UpdateState::PreUpdate ) sceneManager->preUpdate();
            else if( phase == Thread::UpdateState::PostUpdate ) sceneManager->postUpdate();
            else sceneManager->update();
        };
        update();
        BOOST_CHECK_EQUAL( firstCalls, 1 );
        BOOST_CHECK_EQUAL( secondCalls, 1 );
        BOOST_CHECK_EQUAL( replacementCalls, 0 );
        BOOST_CHECK_EQUAL( count( phase, task, first ), 0 );
        BOOST_CHECK_EQUAL( count( phase, task, replacement ), 1 );
        update();
        BOOST_CHECK_EQUAL( firstCalls, 1 );
        BOOST_CHECK_EQUAL( secondCalls, 2 );
        BOOST_CHECK_EQUAL( replacementCalls, 1 );
        first->callback = second->callback = replacement->callback = nullptr;
        sceneManager->unregisterAllComponent( first );
        sceneManager->unregisterAllComponent( second );
        sceneManager->unregisterAllComponent( replacement );
    }
}

BOOST_AUTO_TEST_SUITE_END()
