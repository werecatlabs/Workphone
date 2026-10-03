#ifndef __CoroutineData_h__
#define __CoroutineData_h__

#include <Workphone/Interface/System/ICoroutineData.hpp>
#include <Workphone/Memory/RawPtr.hpp>

namespace workphone
{

    /**
     * @brief Concrete implementation of ICoroutineData.
     *
     * Stores the coroutine resume line number and an associated object reference.
     * The resume line number is stored in an atomic to allow safe access from
     * multiple threads. The associated object is stored as a RawPtr to an
     * IObject instance.
     *
     * This class provides minimal coroutine control operations (invoke, stop,
     * yield) and accessors for the current resume line number. The exact
     * coroutine behaviour is implemented in the corresponding source file.
     */
    class WPCore_API CoroutineData : public ICoroutineData
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the coroutine data to a safe default state.
         */
        CoroutineData();

        /**
         * @brief Construct with an associated object.
         *
         * @param pObject RawPtr to an IObject that will be associated with this coroutine.
         */
        CoroutineData( RawPtr<IObject> pObject );

        /**
         * @brief Virtual destructor.
         */
        ~CoroutineData() override;

        /**
         * @brief Invoke the coroutine step/function.
         *
         * Calling the function-call operator will execute the coroutine's
         * current step. The detailed behaviour (how the line number is used,
         * how the associated object is accessed) is defined in the .cpp file.
         */
        void operator()();

        /**
         * @brief Get the coroutine resume line number.
         *
         * The line number represents the current resume point of the coroutine.
         * Access is thread-safe because the underlying storage is atomic.
         *
         * @return Current line number as s32.
         */
        s32 getLineNumber() const override;

        /**
         * @brief Set the coroutine resume line number.
         *
         * Use this to update the point at which the coroutine will resume.
         * The operation is thread-safe.
         *
         * @param lineNumber New resume line number.
         */
        void setLineNumber( s32 lineNumber ) override;

        /**
         * @brief Request the coroutine to stop.
         *
         * Semantics: instructs the coroutine to cease execution. The exact
         * mechanics (for example, setting a sentinel line number or notifying
         * a scheduler) are implemented in the source file.
         */
        void stop() override;

        /**
         * @brief Yield execution from the coroutine.
         *
         * Causes the coroutine to yield control back to the caller/scheduler.
         */
        void yield() override;

    protected:
        /// Atomic storage for the coroutine resume/line number (thread-safe).
        atomic_s32 m_lineNumber;

        /// Associated object for the coroutine (non-owning RawPtr).
        RawPtr<IObject> m_object;
    };

}  // namespace workphone

#endif  // __CoroutineData_h__
