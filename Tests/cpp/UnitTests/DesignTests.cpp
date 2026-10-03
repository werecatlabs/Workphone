#include "UnitTests.hpp"
#include <Workphone/Test/Design/Rhyming.hpp>
#include <Workphone/Test/Design/RhymingOptimised.hpp>
#include <Workphone/Test/Design/TicTacToe.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <set>
#include <string>
#include <tuple>

using namespace workphone;

namespace
{
    using Move = std::tuple<s32, s32, s32, s32>;

    void runTicTacToeScenario( s32 size, const Array<Move> &moves )
    {
        test::TicTacToe game( size );

        for( const auto &move : moves )
        {
            const auto row = std::get<0>( move );
            const auto col = std::get<1>( move );
            const auto player = std::get<2>( move );
            const auto expectedWinner = std::get<3>( move );

            BOOST_CHECK_EQUAL( game.move( row, col, player ), expectedWinner );
        }
    }

    Array<std::unique_ptr<test::Rhyming>> createRhymingImplementations()
    {
        Array<std::unique_ptr<test::Rhyming>> implementations;
        implementations.push_back( std::make_unique<test::Rhyming>() );
        implementations.push_back( std::make_unique<test::RhymingOptimised>() );
        return implementations;
    }

    void checkRhymeMatch( const std::string &input, const std::string &rhyme, s32 expected )
    {
        auto implementations = createRhymingImplementations();
        for( const auto &implementation : implementations )
        {
            BOOST_CHECK_EQUAL( implementation->rhymeMatch( input, rhyme ), expected );
        }
    }

    void checkRhymeResult( const std::string &input, const Array<std::string> &rhymes,
                           const std::set<std::string> &expectedResults )
    {
        auto implementations = createRhymingImplementations();
        for( const auto &implementation : implementations )
        {
            const auto result = implementation->getRhyme( input, rhymes );
            BOOST_CHECK_MESSAGE( expectedResults.find( result ) != expectedResults.end(),
                                 "Unexpected rhyme result: " << result );
        }
    }
}  // namespace

BOOST_AUTO_TEST_CASE( design_tictactoe_detects_wins_on_every_line_type )
{
    runTicTacToeScenario( 3, {
                                 Move{ 0, 0, 1, 0 },
                                 Move{ 1, 0, 2, 0 },
                                 Move{ 0, 1, 1, 0 },
                                 Move{ 1, 1, 2, 0 },
                                 Move{ 0, 2, 1, 1 },
                             } );

    runTicTacToeScenario( 3, {
                                 Move{ 0, 0, 1, 0 },
                                 Move{ 0, 1, 2, 0 },
                                 Move{ 1, 0, 1, 0 },
                                 Move{ 1, 1, 2, 0 },
                                 Move{ 2, 0, 1, 1 },
                             } );

    runTicTacToeScenario( 3, {
                                 Move{ 0, 0, 1, 0 },
                                 Move{ 0, 2, 2, 0 },
                                 Move{ 1, 1, 1, 0 },
                                 Move{ 0, 1, 2, 0 },
                                 Move{ 2, 2, 1, 1 },
                             } );

    runTicTacToeScenario( 3, {
                                 Move{ 0, 2, 1, 0 },
                                 Move{ 0, 0, 2, 0 },
                                 Move{ 1, 1, 1, 0 },
                                 Move{ 1, 0, 2, 0 },
                                 Move{ 2, 0, 1, 1 },
                             } );
}

BOOST_AUTO_TEST_CASE( design_tictactoe_handles_edges_and_larger_boards )
{
    runTicTacToeScenario( 1, {
                                 Move{ 0, 0, 2, 2 },
                             } );

    runTicTacToeScenario( 4, {
                                 Move{ 0, 0, 1, 0 },
                                 Move{ 1, 1, 1, 0 },
                                 Move{ 2, 2, 1, 0 },
                                 Move{ 3, 3, 1, 1 },
                             } );

    runTicTacToeScenario( 4, {
                                 Move{ 0, 0, 1, 0 },
                                 Move{ 0, 1, 2, 0 },
                                 Move{ 0, 2, 1, 0 },
                                 Move{ 0, 3, 2, 0 },
                             } );
}

BOOST_AUTO_TEST_CASE( design_rhyming_match_counts_suffix_characters )
{
    checkRhymeMatch( "", "", 0 );
    checkRhymeMatch( "cat", "", 0 );
    checkRhymeMatch( "", "cat", 0 );
    checkRhymeMatch( "cat", "hat", 2 );
    checkRhymeMatch( "time", "rhyme", 2 );
    checkRhymeMatch( "nation", "station", 5 );
    checkRhymeMatch( "abc", "xyz", 0 );
    checkRhymeMatch( "same", "same", 4 );
}

BOOST_AUTO_TEST_CASE( design_rhyming_returns_best_candidate_only_when_suffix_is_meaningful )
{
    checkRhymeResult( "cat", { "dog", "sun", "cup" }, { "" } );
    checkRhymeResult( "cat", { "bat", "dog", "car" }, { "bat" } );
    checkRhymeResult( "orange", { "range", "door-hinge", "spoon" }, { "range" } );
}
