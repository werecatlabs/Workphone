#ifndef TestSystemSettings_h__
#define TestSystemSettings_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestSystemSettings : public Test
    {
    public:
        TestSystemSettings();
        ~TestSystemSettings() override;

        void run() override;

    protected:
        void testSystemSettings();
        void testQuickSettings();
    };
}  // namespace workphone

#endif  // TestSystemSettings_h__
