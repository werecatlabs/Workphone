#ifndef TestAtomics_h__
#define TestAtomics_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestAtomics : public Test
    {
    public:
        TestAtomics();
        ~TestAtomics() override;

        void run() override;
    };
}  // namespace workphone

#endif  // TestAtomics_h__
