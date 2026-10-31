#include <Workphone/Animation/PositionAnimator2.hpp>
#include <Workphone/Animation/PositionAnimator3.hpp>
#include <Workphone/Animation/ScaleAnimator2.hpp>
#include <Workphone/Animation/ScaleAnimator3.hpp>
#include <Workphone/Atomics/AtomicString.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace cpp_sdk_animator_contracts
{
    using namespace workphone;

    template <class V>
    struct Target
    {
        using Value = V;
        V position = V::zero(), scale = V::unit();
        unsigned positionWrites = 0, scaleWrites = 0;
        void setPosition( const V &value ) { position = value; ++positionWrites; }
        void setScale( const V &value ) { scale = value; ++scaleWrites; }
    };
    using Target2 = Target<Vector2<real_Num>>;
    using Target3 = Target<Vector3<real_Num>>;

    // Animator reads the real timer interface. Control only its delta query so
    // playback checks do not depend on host speed, sleeps or wall-clock jitter.
    class FrameTimer : public TimerMT
    {
    public:
        f64 delta = 0;
        bool fail = false;
        f64 getDeltaTime() const override
        {
            if( fail ) throw std::runtime_error( "injected timer failure" );
            return delta;
        }
    };

    void require( bool condition, const std::string &message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    template <template <class> class A, class T>
    void completionLifetime( FrameTimer &clock, const typename T::Value &start,
                             const typename T::Value &end, bool scaling, const char *family )
    {
        const auto verify = [&]( bool condition, const char *message ) {
            require( condition, std::string( family ) + ": " + message );
        };
        const auto value = [&]( const T &target ) { return scaling ? target.scale : target.position; };
        const auto writes = [&]( const T &target ) { return scaling ? target.scaleWrites : target.positionWrites; };
        for( const bool reverse : { false, true } )
        {
            auto target = std::make_unique<T>();
            A<T> instant( target.get(), 0, start, end );
            instant.setReverse( reverse );
            bool sawEndpoint = false;
            instant.setOnCompleteCallback( [&] {
                sawEndpoint = value( *target ) == ( reverse ? start : end );
                target.reset();
            } );
            instant.start();
            clock.delta = 0;
            instant.update();
            verify( sawEndpoint && !target && instant.isFinished(),
                    "zero-duration callback must observe its endpoint before releasing the target" );
            instant.update();
        }
        {
            T target;
            A<T> restart( &target, 1, start, end );
            unsigned completed = 0;
            bool sawEndpoint = false;
            restart.setOnCompleteCallback( [&] {
                sawEndpoint = value( target ) == end;
                if( ++completed == 1 ) restart.start();
            } );
            restart.start();
            clock.delta = 1;
            restart.update();
            verify( sawEndpoint && completed == 1 && restart.isPlaying() &&
                        restart.getAnimationTime() == 0 && value( target ) == end && writes( target ) == 1,
                    "callback restart must not overwrite the completed endpoint in the old update" );
            clock.delta = .25;
            restart.update();
            verify( value( target ) == start + ( end - start ) * real_Num( .25f ),
                    "restarted playback must advance on the next update" );
        }
        {
            T oldTarget, nextTarget;
            std::unique_ptr<A<T>> active = std::make_unique<A<T>>( &oldTarget, 1, start, end );
            bool sawEndpoint = false;
            active->setOnCompleteCallback( [&] {
                sawEndpoint = value( oldTarget ) == end;
                auto replacement = std::make_unique<A<T>>( &nextTarget, 2, start, end );
                replacement->start();
                // This deletes the animator and its callback while that callback
                // returns. The old update may inspect only its stack marker.
                active = std::move( replacement );
            } );
            active->start();
            auto *original = active.get();
            clock.delta = 1;
            original->update();
            verify( sawEndpoint && active.get() != original && writes( oldTarget ) == 1 &&
                        writes( nextTarget ) == 0 && value( oldTarget ) == end,
                    "completion may replace the animator without stale old or new target writes" );
            clock.delta = .5;
            active->update();
            verify( value( nextTarget ) == start + ( end - start ) * real_Num( .25f ) &&
                        writes( oldTarget ) == 1,
                    "replacement playback must belong entirely to the new target" );
        }
        {
            auto target = std::make_unique<T>();
            A<T> throwing( target.get(), 1, start, end );
            bool sawEndpoint = false, caught = false;
            throwing.setOnCompleteCallback( [&] {
                sawEndpoint = value( *target ) == end;
                target.reset();
                throw std::runtime_error( "injected completion failure" );
            } );
            throwing.start();
            clock.delta = 1;
            try { throwing.update(); }
            catch( const std::runtime_error & ) { caught = true; }
            verify( caught && sawEndpoint && !target && throwing.isFinished(),
                    "throwing completion must not leave a stack marker or touch its released target" );
            throwing.update();
        }
        {
            T target;
            A<T> retry( &target, 1, start, end );
            retry.start();
            clock.fail = true;
            bool caught = false;
            try { retry.update(); }
            catch( const std::runtime_error & ) { caught = true; }
            clock.fail = false;
            verify( caught && writes( target ) == 0, "a failed clock update must not touch the target" );
            clock.delta = 1;
            retry.update();
            verify( retry.isFinished() && value( target ) == end,
                    "a pre-completion exception must clear its marker and permit retry" );
        }
    }

    struct Runtime
    {
        TypeManager types;
        SmartPtr<core::IApplicationManager> previousApplication;
        SmartPtr<core::ApplicationManager> application;
        SmartPtr<FrameTimer> timer;

        Runtime()
        {
            types.load();
            TypeManager::setInstance( &types );
            previousApplication = core::IApplicationManager::instance();
            application = make_ptr<core::ApplicationManager>();
            timer = make_ptr<FrameTimer>();
            application->setTimer( timer );
            core::IApplicationManager::setInstance( application );
        }
        ~Runtime()
        {
            application->setTimer( nullptr );
            timer = nullptr;
            core::IApplicationManager::setInstance( previousApplication );
            application = nullptr;
            types.unload();
            TypeManager::setInstance( nullptr );
        }
    };

    template <template <class> class A, class T>
    void playback( FrameTimer &clock, const typename T::Value &start,
                   const typename T::Value &end, bool scaling, const char *family )
    {
        using V = typename T::Value;
        const auto verify = [&]( bool condition, const char *message ) {
            require( condition, std::string( family ) + ": " + message );
        };
        T target;
        const auto value = [&]() -> V { return scaling ? target.scale : target.position; };
        const auto writes = [&]() { return scaling ? target.scaleWrites : target.positionWrites; };
        const auto at = [&]( real_Num fraction ) { return start + ( end - start ) * fraction; };
        {
            A<T> defaults( &target, 2.0f );
            defaults.start();
            clock.delta = .5;
            defaults.update();
            verify( value() == V::unit(), "default endpoint arguments must instantiate and apply unit values" );
        }
        target.positionWrites = target.scaleWrites = 0;
        A<T> animation( &target, 2.0f, start, end );
        unsigned completed = 0;
        animation.setOnCompleteCallback( [&] { ++completed; } );
        clock.delta = .5;
        animation.update();
        verify( writes() == 0, "stopped animation must not write its target" );
        animation.start();
        animation.update();
        verify( animation.getAnimationTime() == .5f && value() == at( .25f ),
                "time is seconds and interpolation uses the requested duration" );
        animation.pause();
        const auto pausedWrites = writes();
        clock.delta = 1;
        animation.update();
        verify( animation.isPaused() && animation.getAnimationTime() == .5f &&
                    writes() == pausedWrites && value() == at( .25f ),
                "pause must freeze both clock and target" );
        animation.resume();
        animation.setAnimationSpeed( 2 );
        clock.delta = .25;
        animation.update();
        verify( animation.getAnimationTime() == 1 && value() == at( .5f ),
                "resume must preserve elapsed time and apply playback speed" );
        animation.start();
        verify( animation.getAnimationTime() == 1, "start while playing must preserve the base clock contract" );
        clock.delta = 1;
        animation.update();
        verify( animation.isFinished() && !animation.isPlaying() && value() == end && completed == 1,
                "completion must apply the endpoint and fire once" );
        const auto finishedWrites = writes();
        animation.update();
        verify( writes() == finishedWrites && completed == 1, "completed animation must remain stopped" );

        animation.setReverse( true );
        animation.setAnimationSpeed( 1 );
        animation.start();
        verify( animation.getAnimationTime() == 2 && !animation.isFinished(),
                "reverse playback must start at the duration" );
        clock.delta = .5;
        animation.update();
        verify( animation.getAnimationTime() == 1.5f && value() == at( .75f ),
                "reverse playback must traverse the same position curve backwards" );
        clock.delta = 2;
        animation.update();
        verify( animation.isFinished() && !animation.isPlaying() && value() == start && completed == 2,
                "reverse completion must finish at the start endpoint" );

        animation.setLoop( true );
        animation.setReverse( false );
        animation.start();
        clock.delta = .75;
        animation.update();
        verify( value() == at( .375f ), "forward loop must use seconds before its boundary" );
        clock.delta = 1.5;
        animation.update();
        verify( animation.isPlaying() && !animation.isFinished() && value() == start && completed == 2,
                "forward loop must wrap using the base clock without completion callbacks" );
        animation.stop();
        animation.setReverse( true );
        animation.start();
        clock.delta = 2.5;
        animation.update();
        verify( animation.isPlaying() && !animation.isFinished() && value() == end && completed == 2,
                "reverse loop must wrap to the duration" );
        animation.stop();
        const auto stoppedWrites = writes();
        animation.update();
        verify( writes() == stoppedWrites, "stop must prevent target writes" );

        {
            A<T> zero( &target, 0, start, end );
            unsigned zeroCompleted = 0;
            zero.setOnCompleteCallback( [&] { ++zeroCompleted; } );
            zero.start();
            clock.delta = 0;
            zero.update();
            zero.update();
            verify( zero.isFinished() && !zero.isPlaying() && value() == end && zeroCompleted == 1,
                    "zero duration must complete forward once without dividing by zero" );
            zero.setReverse( true );
            zero.start();
            zero.update();
            verify( zero.isFinished() && value() == start && zeroCompleted == 2,
                    "zero duration reverse must choose the start endpoint" );
        }
        {
            A<T> unbound;
            unbound.setAnimationLength( 1 );
            unbound.start();
            clock.delta = .25;
            unbound.update();
            verify( unbound.getAnimationTime() == .25f, "an unbound default target must advance safely" );
        }
        T nextTarget;
        const auto previousTargetValue = value();
        A<T> replacement( &nextTarget, 2, start, end );
        replacement.start();
        clock.delta = .5;
        replacement.update();
        verify( ( scaling ? nextTarget.scale : nextTarget.position ) == at( .25f ) &&
                    value() == previousTargetValue,
                "a replacement animator must own independent time and target writes" );
    }

    void atomicString()
    {
        using Text = BaseString<c8>;
        AtomicString<c8> empty;
        require( empty.load().empty(), "AtomicString default construction must expose an empty value" );
        AtomicString<c8> text( Text( "installed atomic string" ) );
        const Text converted = text;
        require( text.load() == converted && text.str() == "installed atomic string",
                 "AtomicString load/str/conversion must agree" );
        require( text.try_lock(), "AtomicString first try_lock must acquire" );
        require( !text.try_lock(), "AtomicString second try_lock must report contention" );
        text.unlock();
        require( text.try_lock(), "AtomicString unlock must permit reacquisition" );
        text.unlock();
        struct MutableText : AtomicString<c8>
        {
            void assign( const Text &value )
            {
                std::lock_guard<MutableText> guard( *this );
                m_string = value;
            }
        } shared;
        const Text first( 1024, 'a' ), second( 1024, 'b' );
        shared.assign( first );
        std::atomic<bool> valid{ true };
        std::thread writer( [&] {
            for( unsigned index = 0; index < 1000; ++index )
                shared.assign( index % 2 ? first : second );
        } );
        const auto read = [&] {
            for( unsigned index = 0; index < 1000; ++index )
            {
                const auto value = shared.load();
                if( value != first && value != second ) valid = false;
            }
        };
        std::thread one( read ), two( read );
        writer.join(); one.join(); two.join();
        require( valid, "AtomicString concurrent snapshots must not tear guarded mutations" );
    }
}

// Compile every member body rather than only parsing uninstantiated templates.
template class workphone::PositionAnimator2<cpp_sdk_animator_contracts::Target2>;
template class workphone::PositionAnimator3<cpp_sdk_animator_contracts::Target3>;
template class workphone::ScaleAnimator2<cpp_sdk_animator_contracts::Target2>;
template class workphone::ScaleAnimator3<cpp_sdk_animator_contracts::Target3>;

int main()
{
    using namespace cpp_sdk_animator_contracts;
    try
    {
        Runtime runtime;
        playback<PositionAnimator2, Target2>( *runtime.timer, { 0, 4 }, { 8, 12 }, false, "position2" );
        playback<PositionAnimator3, Target3>( *runtime.timer, { 0, 4, 8 }, { 8, 12, 16 }, false, "position3" );
        playback<ScaleAnimator2, Target2>( *runtime.timer, { 0, 4 }, { 8, 12 }, true, "scale2" );
        playback<ScaleAnimator3, Target3>( *runtime.timer, { 0, 4, 8 }, { 8, 12, 16 }, true, "scale3" );
        completionLifetime<PositionAnimator2, Target2>( *runtime.timer, { 0, 4 }, { 8, 12 }, false, "position2" );
        completionLifetime<PositionAnimator3, Target3>( *runtime.timer, { 0, 4, 8 }, { 8, 12, 16 }, false, "position3" );
        completionLifetime<ScaleAnimator2, Target2>( *runtime.timer, { 0, 4 }, { 8, 12 }, true, "scale2" );
        completionLifetime<ScaleAnimator3, Target3>( *runtime.timer, { 0, 4, 8 }, { 8, 12, 16 }, true, "scale3" );
        atomicString();
        std::cout << "Installed animator templates and AtomicString contracts passed\n";
        return 0;
    }
    catch( const std::exception &exception )
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
