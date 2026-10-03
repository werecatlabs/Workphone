#ifndef NumIslands_h__
#define NumIslands_h__

#include <Workphone/Test/Tests/Test.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    class TestNumIslands : public Test
    {
    public:
        TestNumIslands();
        ~TestNumIslands() override;

        void run() override;

        void resetCheck( Array<Array<bool>> &checked );

        bool markIsland( Array<Array<c8>> &grid, Array<Array<bool>> &checked, s32 x, s32 y );

        s32 numIslands( Array<Array<c8>> &grid );
    };

}  // namespace workphone

#endif  // NumIslands_h__
