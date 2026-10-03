#ifndef TestInput_h__
#define TestInput_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestInput : public Test
    {
    public:
        TestInput();
        ~TestInput() override;

        void run() override;
        void report() override;
    };
}  // namespace workphone

#endif  // TestInput_h__
