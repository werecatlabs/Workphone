#ifndef ITest_h__
#define ITest_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @brief An interface for a test class.
     */
    class ITest : public ISharedObject
    {
    public:
        /**
         * @brief Destructor for ITest.
         */
        ~ITest() override;

        /**
         * @brief Runs the test.
         */
        virtual void run() = 0;

        /**
         * @brief Reports the results of the test.
         */
        virtual void report() = 0;

        /**
         * @brief Returns whether the test is enabled.
         * @return True if the test is enabled, false otherwise.
         */
        virtual bool isEnabled() const = 0;

        /**
         * @brief Sets whether the test is enabled.
         * @param enabled The new enabled state of the test.
         */
        virtual void setEnabled( bool enabled ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ITest_h__
