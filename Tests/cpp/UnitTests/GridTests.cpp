#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

void resetCheck( Array<Array<bool>> &checked )
{
    for( int x = 0; x < checked.size(); ++x )
    {
        for( int y = 0; y < checked[x].size(); ++y )
        {
            checked[x][y] = false;
        }
    }
}

bool markIsland( Array<Array<String>> &grid, Array<Array<bool>> &checked, int x, int y )
{
    std::cout << "visit " << x << "," << y << std::endl;

    bool result = false;
    if( x >= 0 && x < grid.size() )
    {
        if( y >= 0 && y < grid[x].size() )
        {
            if( checked[x][y] == false )
            {
                if( grid[x][y] != "0" )
                {
                    std::cout << "marked " << x << "," << y << std::endl;
                    checked[x][y] = true;

                    markIsland( grid, checked, x + 1, y );
                    markIsland( grid, checked, x - 1, y );
                    markIsland( grid, checked, x, y + 1 );
                    markIsland( grid, checked, x, y - 1 );

                    result = true;
                }
            }
            else
            {
                std::cout << "visited " << x << "," << y << std::endl;
            }
        }
    }

    return result;
}

int numIslands( Array<Array<String>> &grid )
{
    Array<Array<bool>> checked;
    checked.resize( grid.size() );

    for( size_t i = 0; i < checked.size(); ++i )
    {
        checked[i].resize( grid[i].size() );
    }

    resetCheck( checked );

    int count = 0;
    for( int x = 0; x < grid.size(); ++x )
    {
        for( int y = 0; y < grid[x].size(); ++y )
        {
            std::cout << "check " << x << "," << y << std::endl;

            if( grid[x][y] != "0" )
            {
                if( markIsland( grid, checked, x, y ) )
                {
                    std::cout << x << "," << y << std::endl;
                    count++;
                }
            }
        }
    }

    return count;
}

BOOST_AUTO_TEST_CASE( island_tests )
{
    using namespace workphone;

    Array<Array<Array<String>>> tests;

    tests.push_back( Array<Array<String>>( { { "1", "1", "1", "1", "0" },
                                             { "1", "1", "0", "1", "0" },
                                             { "1", "1", "0", "0", "0" },
                                             { "0", "0", "0", "0", "0" } } ) );

    tests.push_back( Array<Array<String>>( { { "1", "1", "0", "0", "0" },
                                             { "1", "1", "0", "0", "0" },
                                             { "0", "0", "1", "0", "0" },
                                             { "0", "0", "0", "1", "1" } } ) );

    tests.push_back( Array<Array<String>>( { { "1", "0", "1", "0", "1", "1" } } ) );

    BOOST_CHECK( numIslands( tests[0] ) == 1 );
    BOOST_CHECK( numIslands( tests[1] ) == 3 );
    BOOST_CHECK( numIslands( tests[2] ) == 3 );
}
