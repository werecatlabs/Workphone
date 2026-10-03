#ifndef TestModelComponent_h__
#define TestModelComponent_h__

#include "Workphone/Test/Tests/Test.hpp"

namespace workphone
{
    class TestComponent : public Test
    {
    public:
        TestComponent();
        ~TestComponent() override;

        void run() override;

    protected:
        void testBaseComponent();
    };
}  // namespace workphone

#endif  // TestModelComponent_h__
