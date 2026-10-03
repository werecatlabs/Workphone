#ifndef TestSceneryChange_h__
#define TestSceneryChange_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestSceneChange : public Test
    {
    public:
        TestSceneChange();
        ~TestSceneChange() override;

        void run() override;

        void report() override;
    };
}  // namespace workphone

#endif  // TestSceneryChange_h__
