#ifndef ITestManager_h__
#define ITestManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @brief An interface for a test manager class.
     */
    class ITestManager : public ISharedObject
    {
    public:
        /**
         * @brief Destructor for ITestManager.
         */
        ~ITestManager() override;

        /**
         * @brief Updates the test manager.
         */
        void update() override = 0;

        /**
         * @brief Starts the test manager.
         */
        virtual void start() = 0;

        /**
         * @brief Stops the test manager.
         */
        virtual void stop() = 0;

        /**
         * @brief Returns whether the test manager is currently running.
         * @return True if the test manager is running, false otherwise.
         */
        virtual bool isRunning() const = 0;

        /**
         * @brief Sets whether the test manager is running.
         * @param running The new running state of the test manager.
         */
        virtual void setIsRunning( bool running ) = 0;

        /**
         * @brief Gets an array of all tests managed by the test manager.
         * @return An array of shared pointers to the tests.
         */
        virtual Array<SmartPtr<ITest>> getTests() const = 0;

        /**
         * @brief Sets the tests managed by the test manager.
         * @param tests An array of shared pointers to the tests.
         */
        virtual void setTests( Array<SmartPtr<ITest>> tests ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // ITestManager_h__
