#ifndef __WP_ICoroutineData_h__
#define __WP_ICoroutineData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <boost/coroutine2/coroutine.hpp>

namespace workphone
{

    /**
     * @brief Interface for coroutine-backed data objects.
     *
     * ICoroutineData provides an abstraction for objects driven by a Boost.Coroutine2
     * coroutine. Implementations can be stepped, yielded and stopped by the owning
     * worker/manager. The interface inherits from ISharedObject so instances can be
     * managed with the project's shared-pointer semantics.
     */
    class WPCore_API ICoroutineData : public ISharedObject
    {
    public:
        /** Alias for the Boost coroutine type used by coroutine-backed objects. */
        using CoroutineType = boost::coroutines2::coroutine<void>;

        /** Pull-side type (coroutine caller / consumer). */
        using PullType = boost::coroutines2::coroutine<void>::pull_type;

        /** Push-side type (coroutine body / producer). */
        using PushType = boost::coroutines2::coroutine<void>::push_type;

        ICoroutineData();

        /** Virtual destructor. Implementations should clean up coroutine resources. */
        ~ICoroutineData() override;

        /**
         * @brief Get the current execution line number for this coroutine.
         *
         * This typically represents a saved continuation point used by the coroutine
         * implementation for debugging, scheduling or serialization. Return value may
         * be negative or zero depending on implementation semantics.
         *
         * @return The stored line number / continuation identifier.
         */
        virtual s32 getLineNumber() const = 0;

        /**
         * @brief Set the current execution line number for this coroutine.
         *
         * Implementations should persist the provided line number so the coroutine
         * can resume from the correct continuation point.
         *
         * @param lineNumber The line number / continuation identifier to store.
         */
        virtual void setLineNumber( s32 lineNumber ) = 0;

        /**
         * @brief Yield execution of the coroutine.
         *
         * When called from within a coroutine, this should suspend the coroutine's
         * execution and return control to the caller/owner. Implementations may
         * update internal state before yielding.
         */
        virtual void yield() = 0;

        /**
         * @brief Stop the coroutine and transition it to a finished state.
         *
         * This indicates the coroutine should cease further execution and release any
         * associated resources. After calling `stop()`, `yield()` should not resume
         * execution. Implementations should handle repeated calls safely.
         */
        virtual void stop() = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // __WP_ICoroutineData_h__
