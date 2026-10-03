#ifndef Rhyming_h__
#define Rhyming_h__

#include <Workphone/Core/Array.hpp>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <assert.h>

namespace workphone
{
    namespace test
    {

        /** Base rhyming class. Contains a brute force implementation.
         * This was my initial implementation. I used brute force approach for readability and to help me
         * understand the problem.
         */
        class WPCore_API Rhyming
        {
        public:
            Rhyming();
            virtual ~Rhyming();

            /** Checks the number of characters that match from the end of the string.
             * Time complexity: O(n)
             * Space complexity: O(n)
             *
             */
            virtual int rhymeMatch( const std::string &input, const std::string &rhyme );

            /** Finds the best rhyming match.
             * Time complexity: O(n)
             * Space complexity: O(n)
             */
            virtual std::string getRhyme( const std::string &input, const Array<std::string> &rhymes );
        };

    }  // namespace test
}  // namespace workphone

#endif  // Rhyming_h__
