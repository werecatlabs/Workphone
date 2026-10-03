#ifndef IAsyncOperation_h__
#define IAsyncOperation_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Function.hpp>

namespace workphone
{

    /**
     * @brief Interface for an asynchronous operation, used to manage completion events.
     * Inherits from ISharedObject.
     */
    class IAsyncOperation : public ISharedObject
    {
    public:
        /**
         * @brief Destroy the IAsyncOperation object.
         */
        ~IAsyncOperation() override = default;

        /**
         * @brief Remove a completion event function from the async operation.
         *
         * @param func A function<void()> to be removed from the completion event list.
         */
        virtual void removeCompleteEvent( Function<void()> func ) = 0;

        /**
         * @brief Add a completion event function to the async operation.
         *
         * @param func A function<void()> to be added to the completion event list.
         */
        virtual void addCompleteEvent( Function<void()> func ) = 0;
    };

}  // namespace workphone

#endif  // IAsyncOperation_h__
