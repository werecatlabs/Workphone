#ifndef TestModelCrashing_h__
#define TestModelCrashing_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestActorDestroy : public Test
    {
    public:
        TestActorDestroy();
        ~TestActorDestroy() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestAtomics_h__
