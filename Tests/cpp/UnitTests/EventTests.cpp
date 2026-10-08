#include <random>
#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <stdexcept>

using namespace workphone;
using namespace std;

enum LoadEvent
{
    Loaded,
    Unloaded,
};

class DefaultEventListener : public IEventListener
{
public:
    Parameter handleEvent( EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
                           SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                           SmartPtr<IEvent> event )
    {
        ++eventCount;
        lastEventValue = eventValue;
        if( throwOnEvent )
        {
            throw std::runtime_error( "Intentional event-listener test failure" );
        }

        return Parameter();
    }

    u32 eventCount = 0;
    hash_type lastEventValue = 0;
    bool throwOnEvent = false;
};

namespace workphone
{
    class TargetedEventComponent : public scene::Component
    {
    public:
        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override
        {
            ++m_eventCount;
            return scene::Component::handleEvent( eventType, eventValue, arguments, sender, object,
                                                  event );
        }

        u32 getEventCount() const
        {
            return m_eventCount;
        }

        void resetEventCount()
        {
            m_eventCount = 0;
        }

        WP_CLASS_REGISTER_DECL;

    private:
        u32 m_eventCount = 0;
    };

    WP_CLASS_REGISTER_DERIVED( workphone, TargetedEventComponent, scene::Component );
}  // namespace workphone

BOOST_AUTO_TEST_CASE( event_listener_registrations_are_independent_between_contexts )
{
    auto manager = core::IApplicationManager::instance()->getStateManager();
    auto first = manager->addStateContext();
    auto second = manager->addStateContext();
    auto shared = make_ptr<DefaultEventListener>();
    auto other = make_ptr<DefaultEventListener>();
    const auto initialReferences = shared->getReferences();
    first->addEventListener( shared );
    first->addEventListener( other );
    second->addEventListener( shared );
    first->addEventListener( shared );
    BOOST_REQUIRE_EQUAL( first->getEventListeners().size(), 2u );
    BOOST_CHECK( first->getEventListeners()[1] == other );
    BOOST_CHECK_EQUAL( shared->getReferences(), initialReferences + 2 );
    BOOST_CHECK( first->removeEventListener( shared ) );
    BOOST_CHECK( !first->removeEventListener( shared ) );
    BOOST_REQUIRE_EQUAL( first->getEventListeners().size(), 1u );
    BOOST_CHECK( first->getEventListeners()[0] == other );
    BOOST_REQUIRE_EQUAL( second->getEventListeners().size(), 1u );
    BOOST_CHECK( second->getEventListeners()[0] == shared );
    manager->removeStateContext( first );
    BOOST_REQUIRE_EQUAL( second->getEventListeners().size(), 1u );
    BOOST_CHECK_EQUAL( shared->getReferences(), initialReferences + 1 );
    manager->removeStateContext( second );
    BOOST_CHECK_EQUAL( shared->getReferences(), initialReferences );
}

BOOST_AUTO_TEST_CASE( event_listener_add )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto stateContext = stateManager->addStateContext();
        BOOST_CHECK( stateContext );

        if( stateContext )
        {
            auto listener = workphone::make_ptr<DefaultEventListener>();
            stateContext->addEventListener( listener );

            auto eventListeners = stateContext->getEventListeners();

            BOOST_CHECK( eventListeners.size() == 1 );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( event_global_trigger )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto stateContext = stateManager->addStateContext();
        BOOST_CHECK( stateContext );

        if( stateContext )
        {
            auto listener = workphone::make_ptr<DefaultEventListener>();
            stateContext->addEventListener( listener );

            auto eventListeners = stateContext->getEventListeners();

            BOOST_CHECK( eventListeners.size() == 1 );

            auto testEventHash = StringUtil::getHash( "TestEvent" );
            applicationManager->triggerEvent( EventType::Object, testEventHash, Array<Parameter>(),
                                              nullptr, nullptr, nullptr );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( event_anonymous_broadcast_delivery )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto listener = workphone::make_ptr<DefaultEventListener>();
    applicationManager->addObjectListener( listener );
    guard.addCleanup( [applicationManager, listener]() mutable {
        applicationManager->removeObjectListener( listener );
    } );

    const auto eventHash = StringUtil::getHash( "anonymousBroadcastEvent" );
    applicationManager->triggerEvent( EventType::Object, eventHash, {}, nullptr, nullptr, nullptr, true,
                                      Thread::Application_Flag );

    BOOST_CHECK_EQUAL( listener->eventCount, 1 );
    BOOST_CHECK_EQUAL( listener->lastEventValue, eventHash );
}

BOOST_AUTO_TEST_CASE( event_listener_failure_isolated_and_delivery_deduplicated )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto throwingListener = workphone::make_ptr<DefaultEventListener>();
    throwingListener->throwOnEvent = true;
    auto receivingListener = workphone::make_ptr<DefaultEventListener>();

    applicationManager->addObjectListener( throwingListener );
    applicationManager->addObjectListener( receivingListener );
    guard.addCleanup( [applicationManager, throwingListener, receivingListener]() mutable {
        applicationManager->removeObjectListener( throwingListener );
        applicationManager->removeObjectListener( receivingListener );
    } );

    const auto eventHash = StringUtil::getHash( "isolatedListenerFailureEvent" );
    applicationManager->triggerEvent( EventType::Object, eventHash, {}, applicationManager,
                                      applicationManager, nullptr, true, Thread::Application_Flag );

    // ApplicationManager is the global sender and the receiving object. A listener registered on
    // it must still be invoked once, and a preceding exception must not abort dispatch.
    BOOST_CHECK_EQUAL( throwingListener->eventCount, 1 );
    BOOST_CHECK_EQUAL( receivingListener->eventCount, 1 );
    BOOST_CHECK_EQUAL( receivingListener->lastEventValue, eventHash );
}

BOOST_AUTO_TEST_CASE( event_filesystem_refresh_sender_is_globally_deliverable )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto fileSystem = applicationManager->getFileSystem();
    BOOST_REQUIRE( fileSystem );
    BOOST_CHECK( fileSystem->getObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS ) );
    BOOST_CHECK_EQUAL( fileSystem->getEventTaskFlags(), Thread::Application_Flag );

    auto listener = workphone::make_ptr<DefaultEventListener>();
    applicationManager->addObjectListener( listener );
    guard.addCleanup( [applicationManager, listener]() mutable {
        applicationManager->removeObjectListener( listener );
    } );

    Array<Parameter> arguments = { Parameter( "Assets" ) };
    applicationManager->triggerEvent( EventType::IO, IEvent::refreshPath, arguments, fileSystem, nullptr,
                                      nullptr, true, Thread::Application_Flag );

    BOOST_CHECK_EQUAL( listener->eventCount, 1 );
    BOOST_CHECK_EQUAL( listener->lastEventValue, IEvent::refreshPath );
}

BOOST_AUTO_TEST_CASE( event_component_target_dispatch )
{
    TestGuard guard;

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto gameManager = applicationManager->getGameManager();
    BOOST_REQUIRE( gameManager );

    auto scene = gameManager->getCurrentScene();
    BOOST_REQUIRE( scene );

    auto actor = gameManager->createActor();
    BOOST_REQUIRE( actor );

    const auto applicationListenerCount = applicationManager->getObjectListeners().size();

    auto target = workphone::make_ptr<TargetedEventComponent>();
    auto payload = workphone::make_ptr<TargetedEventComponent>();
    actor->addComponentInstance( target );
    actor->addComponentInstance( payload );
    target->load( nullptr );
    payload->load( nullptr );

    target->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
    payload->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, false );
    target->setLoadingState( LoadingState::Loaded );
    payload->setLoadingState( LoadingState::Loaded );
    target->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
    payload->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );

    BOOST_CHECK_EQUAL( applicationManager->getObjectListeners().size(), applicationListenerCount );

    scene->setActors( { actor } );
    target->resetEventCount();
    payload->resetEventCount();

    auto event = workphone::make_ptr<IEvent>();
    event->setTarget( target );

    BOOST_CHECK( event->isTarget( target ) );
    BOOST_CHECK( !event->isTarget( payload ) );

    applicationManager->triggerEvent( EventType::Scene, StringUtil::getHash( "targetedComponentEvent" ),
                                      {}, gameManager, payload, event, true, Thread::Application_Flag );

    BOOST_CHECK_EQUAL( target->getEventCount(), 1 );
    BOOST_CHECK_EQUAL( payload->getEventCount(), 0 );
}

BOOST_AUTO_TEST_CASE( event_listener_trigger )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        BOOST_CHECK( stateManager );

        auto stateContext = stateManager->addStateContext();
        BOOST_CHECK( stateContext );

        if( stateContext )
        {
            auto listener = workphone::make_ptr<DefaultEventListener>();
            stateContext->addEventListener( listener );

            auto eventListeners = stateContext->getEventListeners();

            BOOST_CHECK( eventListeners.size() == 1 );

            stateContext->triggerEvent( EventType::Loading, LoadEvent::Loaded, Array<Parameter>(),
                                        nullptr, nullptr, nullptr );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
