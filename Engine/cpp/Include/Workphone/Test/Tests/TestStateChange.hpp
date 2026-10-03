#ifndef TestStateChange_h__
#define TestStateChange_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestStateChange : public Test
    {
    public:
        TestStateChange();
        ~TestStateChange() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestStateChange_h__
