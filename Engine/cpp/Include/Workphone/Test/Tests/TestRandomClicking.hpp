#ifndef TestRandomClicking_h__
#define TestRandomClicking_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestRandomClicking : public Test
    {
    public:
        TestRandomClicking();
        ~TestRandomClicking() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestRandomClicking_h__
