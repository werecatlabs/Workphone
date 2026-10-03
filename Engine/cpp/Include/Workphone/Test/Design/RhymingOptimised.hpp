#ifndef RhymingOptimised_h__
#define RhymingOptimised_h__

#include <Workphone/Test/Design/Rhyming.hpp>

namespace workphone
{
    namespace test
    {

        /** Optimised rhyming class.  */
        class WPCore_API RhymingOptimised : public Rhyming
        {
        public:
            RhymingOptimised();
            ~RhymingOptimised() override;

            /** Checks the number of characters that match from the end of the string.
             * Time complexity: O(n)
             * Space complexity: O(n)
             *
             */
            int rhymeMatch( const std::string &input, const std::string &rhyme ) override;

            /** Finds the best rhyming match.
             * Time complexity: O(n)
             * Space complexity: O(n)
             */
            std::string getRhyme( const std::string &input, const Array<std::string> &rhymes ) override;
        };

    }  // namespace test
}  // namespace workphone

#endif  // RhymingOptimised_h__
