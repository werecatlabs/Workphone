#ifndef TestAutoConfiguration_h__
#define TestAutoConfiguration_h__

#include "Workphone/Test/Tests/Test.hpp"
#include <string>

namespace workphone
{

    // automatically test the configuration of the system, such as checking for required
    // environment variables, verifying that necessary files are present,
    // and ensuring that the system meets the minimum requirements for running the application.
    class TestAutoConfiguration : public Test
    {
    public:
        TestAutoConfiguration();
        ~TestAutoConfiguration() override;

        void run() override;
        void report() override;

    private:
        bool m_isConfigured{ false };
        std::string m_report;
    };
}  // namespace workphone

#endif  // TestAutoConfiguration_h__
