#ifndef __AsyncOperation_h__
#define __AsyncOperation_h__

#include <Workphone/Interface/System/IAsyncOperation.hpp>
#include <functional>

namespace workphone
{

    /**
     * @class AsyncOperation
     * @brief Base implementation of an asynchronous operation that supports completion callbacks.
     *
     * AsyncOperation provides a simple mechanism to register and remove completion callbacks
     * (zero-argument, void-returning callables). When an async operation completes the
     * registered callbacks are expected to be invoked by the concrete implementation
     * that owns or extends this class.
     *
     * This class implements the IAsyncOperation interface.
     *
     * @remarks
     * - Callbacks are stored in @c m_completeEvents.
     * - Removal of callbacks relies on equality comparison of stored std::function objects;
     *   depending on how callables are created and captured, equality may or may not behave
     *   as callers expect. Prefer storing and removing the exact same std::function instance.
     */
    class WPCore_API AsyncOperation : public IAsyncOperation
    {
    public:
        /**
         * @brief Construct a new AsyncOperation.
         *
         * Initializes internal state. Concrete subclasses can extend this to perform
         * additional initialization.
         */
        AsyncOperation();

        /**
         * @brief Destroy the AsyncOperation.
         *
         * Virtual destructor ensures correct cleanup when deleting derived implementations
         * through a pointer to IAsyncOperation.
         */
        ~AsyncOperation() override;

        /**
         * @brief Remove a previously registered completion callback.
         *
         * @param func The callback to remove. The callback must match (==) the stored
         *             std::function instance in order to be removed successfully.
         *
         * @note If multiple identical callbacks were added, this implementation may remove
         *       the first matching instance only (behavior depends on the concrete
         *       implementation). Callers who require specific removal semantics should
         *       keep references to the exact std::function used when adding.
         */
        void removeCompleteEvent( std::function<void()> func ) override;

        /**
         * @brief Add a completion callback to be invoked when the operation finishes.
         *
         * @param func A zero-argument callable that will be invoked on completion.
         *
         * @note Callers should avoid adding long-running or blocking work inside completion
         *       callbacks. If ordering or thread-affinity is required, the callback should
         *       marshal work to the appropriate context.
         */
        void addCompleteEvent( std::function<void()> func ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Storage for registered completion callbacks.
         *
         * Each element is a zero-argument callable invoked when the async operation
         * completes. Subclasses should iterate and invoke these callbacks when completing
         * the operation and clear the list if appropriate.
         */
        Array<std::function<void()>> m_completeEvents;
    };

}  // namespace workphone

#endif  // __AsyncOperation_h__
