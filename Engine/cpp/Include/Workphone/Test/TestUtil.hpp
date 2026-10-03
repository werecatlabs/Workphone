#ifndef __WP_TestUtil_h__
#define __WP_TestUtil_h__

#include <Workphone/Test/Fakes/ShapeFake.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/Map.hpp>

namespace workphone
{
    namespace test
    {

        class WPCore_API TestUtil
        {
        public:
            static void setupDefault();
            static void setupGraphics();
            static void setupGame();

            static void setupFactories();

            static void destroyDefault();

            static Array<s32> twoSum( Array<s32> &nums, s32 target );

            static void twoSumII( Array<s32> &nums, s32 i, Array<Array<s32>> &res );

            static Array<Array<s32>> threeSum( Array<s32> &nums );

            /** Trapping Rain Water
             * Time complexity: O(n). Stores the maximum heights upto a point using 2 iterations of
            /* O(n) each. We finally update \text{ans}ans using the stored values in O(n).
             * Space complexity: O(n)  space for left_max and right_max arrays.
             */
            static int trap( Array<s32> &height );

            /** Trapping Rain Water
             * Time complexity: O(n)
             */
            static int trap_On( Array<s32> &height );

            /** Sparse Matrix Multiplication Naive Iteration
             * Time complexity: O(m⋅k⋅n)
             *  Iterate over all m⋅k elements of the matrix mat1.
             *  For each element of matrix mat1, we iterate over all n columns of the matrix mat2.
             *  Leads to a time complexity of m⋅k⋅n.
             * Space complexity: O(1)
             *  We use a matrix ans of size m×n to output the multiplication result which is not included
             * in auxiliary space.
             */
            static Array<Array<s32>> multiply_naive_iteration( Array<Array<s32>> &mat1,
                                                               Array<Array<s32>> &mat2 );

            /** Sparse Matrix Multiplication List of Lists
             * Time complexity: O(m⋅k⋅n)
             */
            static Array<Array<Pair<int, int>>> compressMatrix( Array<Array<s32>> &matrix );

            /** Sparse Matrix Multiplication List of Lists
             * Time complexity: O(m⋅k⋅n)
             */
            static Array<Array<s32>> multiply( Array<Array<s32>> &mat1, Array<Array<s32>> &mat2 );
        };

    }  // namespace test
}  // namespace workphone

#endif  // TestUtil_h__
