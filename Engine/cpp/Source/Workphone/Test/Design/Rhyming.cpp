#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Design/Rhyming.hpp>

namespace workphone
{
    namespace test
    {

        Rhyming::Rhyming() = default;

        Rhyming::~Rhyming() = default;

        int Rhyming::rhymeMatch( const std::string &input, const std::string &rhyme )
        {
            int matches = 0;

            const auto str1 = std::string( input.rbegin(), input.rend() );
            const auto str2 = std::string( rhyme.rbegin(), rhyme.rend() );

            for( size_t i = 0; i < str1.size() && i < str2.size(); ++i )
            {
                if( str1[i] == str2[i] )
                {
                    ++matches;
                }
                else
                {
                    break;
                }
            }

            return matches;
        }

        std::string Rhyming::getRhyme( const std::string &input, const Array<std::string> &rhymes )
        {
            auto numMatching = 0;
            Array<std::string> matches;

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
                Array<std::string> output;
                std::sample( matches.begin(), matches.end(), std::back_inserter( output ), 1,
                             std::mt19937{ std::random_device{}() } );

                assert( !output.empty() );
                return output.front();
            }

            return std::string();
        }

    }  // namespace test
}  // namespace workphone
