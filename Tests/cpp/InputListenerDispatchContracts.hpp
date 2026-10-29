#pragma once

#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/System/WindowMessageData.hpp>
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
#ifdef WP_PLATFORM_WIN32
#    include <Windows.h>
#endif

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

    class SuspendedInput final : public InputDeviceManager
    {
    public:
        SuspendedInput()
        {
            // Exercise the real native window listener without opening a window
            // or polling any physical input device.
            window = make_ptr<render::GraphicsWindow>();
            setWindow( window );
        }

        size_t queuedEvents() const
        {
            size_t count = 0;
            for( const auto &queue : m_inputEventQueue ) count += queue.size();
            return count;
        }

        void injectStaleTaskEvents( SmartPtr<IInputEvent> event )
        {
            for( auto &queue : m_inputEventQueue ) queue.push( event );
        }

#ifdef WP_PLATFORM_WIN32
        void nativeMessage( u32 message, size_t wParam = 0, size_t lParam = 0 )
        {
            auto event = make_ptr<WindowMessageData>();
            event->setMessage( message );
            event->setWParam( wParam );
            event->setLParam( lParam );
            for( auto listener : window->getListeners() ) listener->handleEvent( event );
        }
#endif

    private:
        SmartPtr<render::GraphicsWindow> window;
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
        {
            auto input = make_ptr<SuspendedInput>();
            input->setCreateMouse( false );
            input->setCreateKeyboard( false );
            input->setCreateJoysticks( false );
            input->load( nullptr );
            auto observed = std::make_shared<Observation>();
            auto listener = make_ptr<Listener>( observed );
            input->addListener( listener );
            auto event = input->createInputEvent();
            event->setEventType( IInputEvent::EventType::User );
            require( input->postEvent( event ) && input->queuedEvents() == static_cast<size_t>( TaskId::Count ),
                     "pause fixture must start with one retained event in every task queue" );
            auto oldMouse = input->getCurrentMouseState();
            auto oldKeyboard = input->getCurrentKeyboardState();
            oldMouse->setDragging( true );
            oldMouse->setWheelDelta( Vector2<real_Num>( 1, 2 ) );
            oldKeyboard->setKeyCode( 65 );
            oldKeyboard->setPressedDown( true );
            input->setDebuggerSuspended( true );
            require( input->getDebuggerSuspended() && input->queuedEvents() == 0,
                     "entering a debugger hold must discard all task queues" );
            require( !input->getCurrentKeyboardState()->isPressedDown() &&
                     !input->getCurrentMouseState()->isDragging() &&
                     input->getCurrentMouseState()->getWheelDelta() == Vector2<real_Num>::zero(),
                     "a debugger boundary must discard transient key, drag and wheel snapshots" );
            require( oldKeyboard->isPressedDown() && oldMouse->isDragging(),
                     "resetting input must not mutate snapshots retained by a paused callback" );
            bool acceptedWhileHeld = false;
            for( unsigned i = 0; i < 1024; ++i )
            {
                acceptedWhileHeld |= input->postEvent( event );
                input->queueEvent( event );
                input->triggerEvent( event );
                input->update();
            }
            require( !acceptedWhileHeld && input->queuedEvents() == 0 && observed->calls == 0,
                     "paused input must neither grow queues nor run direct or queued callbacks" );
#ifdef WP_PLATFORM_WIN32
            input->setCreateMouse( true );
            input->setCreateKeyboard( true );
            const auto heldKeyboard = input->getCurrentKeyboardState();
            const auto heldMouse = input->getCurrentMouseState();
            for( unsigned i = 0; i < 128; ++i )
            {
                input->nativeMessage( WM_KEYDOWN, 'A' );
                input->nativeMessage( WM_MOUSEMOVE, 0, ( i << 16 ) | i );
                input->nativeMessage( WM_LBUTTONDOWN );
                input->nativeMessage( WM_MOUSEWHEEL, WHEEL_DELTA << 16 );
            }
            require( input->queuedEvents() == 0 && heldKeyboard == input->getCurrentKeyboardState() &&
                     heldMouse == input->getCurrentMouseState(),
                     "native debugger UI messages must not publish gameplay events or snapshots" );
#endif
            // A resume boundary must also retire stale task events rather than
            // relying only on the entry drain or on producer cooperation.
            input->injectStaleTaskEvents( event );
            input->setDebuggerSuspended( false );
            require( !input->getDebuggerSuspended() && input->queuedEvents() == 0,
                     "resuming must discard stale task events before re-enabling producers" );
            input->update();
            require( observed->calls == 0, "resuming must never replay debugger UI input" );
            require( input->postEvent( event ), "fresh input must be accepted after resume" );
            input->update();
            require( observed->calls == 1, "fresh input must dispatch normally after resume" );
            // The Editor saves this flag rather than assuming it owns an
            // existing hold. Reapplying that prior true state remains held.
            input->setDebuggerSuspended( true );
            const auto previous = input->getDebuggerSuspended();
            input->setDebuggerSuspended( true );
            input->setDebuggerSuspended( previous );
            require( input->getDebuggerSuspended() && !input->postEvent( event ),
                     "restoring a pre-existing debugger input hold must preserve it" );
            input->setDebuggerSuspended( false );
            input->unload( nullptr );
        }
        std::printf( "Input listener dispatch contracts: %u assertions, direct/queued mutation, in-flight retirement and debugger suspension passed.\n",
                     assertions );
        return assertions;
    }
}
