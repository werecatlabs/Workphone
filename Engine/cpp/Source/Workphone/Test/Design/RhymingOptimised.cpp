#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Design/RhymingOptimised.hpp>

namespace workphone
{
    namespace test
    {

        RhymingOptimised::RhymingOptimised() = default;

        RhymingOptimised::~RhymingOptimised() = default;

        int RhymingOptimised::rhymeMatch( const std::string &input, const std::string &rhyme )
        {
            int matches = 0;

            auto inputIt = input.rbegin();
            auto rhymeIt = rhyme.rbegin();

            for( ; inputIt != input.rend() && rhymeIt != rhyme.rend() && *inputIt == *rhymeIt;
                 ++inputIt, ++rhymeIt )
            {
                ++matches;
            }

            return matches;
        }

        std::string RhymingOptimised::getRhyme( const std::string &input,
                                                const Array<std::string> &rhymes )
        {
            auto numMatching = 0;
            Array<std::string> matches;
            matches.reserve( 12 );

            for( auto rhyme : rhymes )
            {
                const auto matching = rhymeMatch( input, rhyme );
                if( matching > numMatching )
                {
                    matches.clear();
                    matches.push_back( rhyme );
                    numMatching = matching;
                }
                else if( matching == numMatching )
                {
                    matches.push_back( rhyme );
                }
            }

            if( numMatching > 1 && !matches.empty() )
            {
                auto numOutputElements = 1;
                Array<std::string> output;
                output.reserve( numOutputElements );

                std::sample( matches.begin(), matches.end(), std::back_inserter( output ),
                             numOutputElements, std::mt19937{ std::random_device{}() } );

                assert( !output.empty() );
                return output.front();
            }

            return std::string();
        }

    }  // namespace test
}  // namespace workphone
