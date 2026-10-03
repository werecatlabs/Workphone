#ifndef TestModelChange_h__
#define TestModelChange_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestActorLoading : public Test
    {
    public:
        TestActorLoading();
        ~TestActorLoading() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestModelChange_h__
