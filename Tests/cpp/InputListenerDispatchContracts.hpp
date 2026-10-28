#pragma once

#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>

namespace input_listener_dispatch_contracts
{
    using namespace workphone;

    struct Observation
    {
        std::atomic<unsigned> calls{ 0 }, destroyed{ 0 };
        std::atomic<bool> validEvents{ true };
    };

    class Listener final : public IEventListener
    {
    public:
        explicit Listener( std::shared_ptr<Observation> observation ) : observed( std::move( observation ) ) {}
        ~Listener() override { ++observed->destroyed; }

        Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                               SmartPtr<IEvent> event ) override
        {
            ++observed->calls;
            if( type != EventType::Input || value != IEvent::inputEvent || !sender || !object || !event )
                observed->validEvents = false;
            if( onInput ) onInput();
            return Parameter();
        }

        std::shared_ptr<Observation> observed;
        std::function<void()> onInput;
    };

    inline unsigned run()
    {
        unsigned assertions = 0;
        const auto require = [&]( bool condition, const char *message ) {
            ++assertions;
            if( !condition ) throw std::runtime_error( message );
        };
        for( const auto queued : { false, true } )
        {
            auto input = make_ptr<InputDeviceManager>();
            input->setCreateMouse( false );
            input->setCreateKeyboard( false );
            input->setCreateJoysticks( false );
            input->load( nullptr );
            // Tests never initialize or poll platform devices.
            input->setCreateJoysticks( false );
            auto event = input->createInputEvent();
            event->setEventType( IInputEvent::EventType::User );
            const auto dispatch = [&]( unsigned count = 1 ) {
                for( unsigned i = 0; i < count; ++i )
                {
                    if( queued )
                    {
                        if( !input->postEvent( event ) )
                            throw std::runtime_error( "listener fixture could not queue its event" );
                    }
                    else input->triggerEvent( event );
                }
                if( queued ) input->update();
            };

            auto firstObserved = std::make_shared<Observation>();
            auto nextObserved = std::make_shared<Observation>();
            auto tailObserved = std::make_shared<Observation>();
            auto first = make_ptr<Listener>( firstObserved );
            auto next = make_ptr<Listener>( nextObserved );
            auto tail = make_ptr<Listener>( tailObserved );
            WeakPtr<IEventListener> weakFirst( first ), weakNext( next );
            bool retainedInsideRemoval = false, retainedThroughEvent = false, absentNextEvent = false;
            first->onInput = [&] {
                input->removeListener( first );
                input->removeListener( next );
                first = nullptr;
                next = nullptr;
                retainedInsideRemoval = !weakFirst.expired() && !weakNext.expired();
            };
            tail->onInput = [&] {
                if( tailObserved->calls == 1 )
                    retainedThroughEvent = !weakFirst.expired() && !weakNext.expired();
                else absentNextEvent = weakFirst.expired() && weakNext.expired();
            };
            input->addListener( first );
            input->addListener( next );
            input->addListener( tail );
            // In queued mode both events are processed in the same update().
            dispatch( 2 );
            require( firstObserved->calls == 1 && nextObserved->calls == 1 && tailObserved->calls == 2,
                     "removing self and next must preserve this event and omit both from the next event" );
            require( retainedInsideRemoval && retainedThroughEvent,
                     "listener snapshot must retain removed callback owners through the complete event" );
            require( absentNextEvent && weakFirst.expired() && weakNext.expired() &&
                     firstObserved->destroyed == 1 && nextObserved->destroyed == 1,
                     "retired listeners must release at the event boundary, including between queued events" );
            require( firstObserved->validEvents && nextObserved->validEvents && tailObserved->validEvents,
                     "direct and queued callbacks must preserve the input event contract" );
            input->removeListeners();
            tail = nullptr;

            auto activeObserved = std::make_shared<Observation>();
            auto removedObserved = std::make_shared<Observation>();
            auto lateObserved = std::make_shared<Observation>();
            auto active = make_ptr<Listener>( activeObserved );
            auto removed = make_ptr<Listener>( removedObserved );
            auto late = make_ptr<Listener>( lateObserved );
            WeakPtr<IEventListener> weakActive( active ), weakRemoved( removed );
            std::promise<void> entered, resume;
            auto callbackEntered = entered.get_future();
            auto continueDispatch = resume.get_future().share();
            bool nextSawActiveRetained = false;
            active->onInput = [&] { entered.set_value(); continueDispatch.wait(); };
            removed->onInput = [&] { nextSawActiveRetained = !weakActive.expired(); };
            input->addListener( active );
            input->addListener( removed );
            std::exception_ptr dispatchFailure;
            std::thread worker( [&] {
                const auto previousTask = Thread::getCurrentTask();
                Thread::setCurrentTask( TaskId::Render );
                try { dispatch(); }
                catch( ... ) { dispatchFailure = std::current_exception(); }
                Thread::setCurrentTask( previousTask );
            } );
            const auto reached = callbackEntered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
            bool retainedWhileBlocked = false;
            if( reached )
            {
                // Another thread retires both the currently executing listener
                // and a later one while dispatch is suspended in user code.
                input->removeListener( active );
                input->removeListener( removed );
                active = nullptr;
                removed = nullptr;
                retainedWhileBlocked = !weakActive.expired() && !weakRemoved.expired() &&
                                       activeObserved->destroyed == 0 && removedObserved->destroyed == 0;
                input->addListener( late );
            }
            resume.set_value();
            worker.join();
            require( reached && !dispatchFailure,
                     "input dispatch must reach and leave the controlled cross-thread removal boundary" );
            require( retainedWhileBlocked && nextSawActiveRetained,
                     "concurrent removal must retain active and later callbacks until the whole snapshot finishes" );
            require( activeObserved->calls == 1 && removedObserved->calls == 1 && lateObserved->calls == 0,
                     "one in-flight event must use its original membership despite removal and late registration" );
            require( weakActive.expired() && weakRemoved.expired() &&
                     activeObserved->destroyed == 1 && removedObserved->destroyed == 1,
                     "concurrently retired listeners must release after in-flight dispatch returns" );
            dispatch();
            require( activeObserved->calls == 1 && removedObserved->calls == 1 && lateObserved->calls == 1,
                     "the next event must observe removal and late registration exactly once" );
            require( activeObserved->validEvents && removedObserved->validEvents && lateObserved->validEvents,
                     "cross-thread dispatch must preserve valid direct and queued input arguments" );
            input->removeListeners();
            late = nullptr;
            input->unload( nullptr );
        }
        std::printf( "Input listener dispatch contracts: %u assertions, direct/queued mutation and in-flight retirement passed.\n",
                     assertions );
        return assertions;
    }
}
