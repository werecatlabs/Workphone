#ifndef TestVideo_h__
#define TestVideo_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestVideo : public Test
    {
    public:
        TestVideo();
        ~TestVideo() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestVideo_h__
