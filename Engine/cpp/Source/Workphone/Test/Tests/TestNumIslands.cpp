#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Tests/TestNumIslands.hpp>
#include <cassert>

namespace workphone
{

    TestNumIslands::TestNumIslands() = default;

    TestNumIslands::~TestNumIslands() = default;

    void TestNumIslands::run()
    {
        {
            Array<Array<c8>> grid;
            assert( numIslands( grid ) == 0 );
        }

        {
            Array<Array<c8>> grid{
                { '1', '1', '1', '1', '0' },
                { '1', '1', '0', '1', '0' },
                { '1', '1', '0', '0', '0' },
                { '0', '0', '0', '0', '0' },
            };
            assert( numIslands( grid ) == 1 );
        }

        {
            Array<Array<c8>> grid{
                { '1', '1', '0', '0', '0' },
                { '1', '1', '0', '0', '0' },
                { '0', '0', '1', '0', '0' },
                { '0', '0', '0', '1', '1' },
            };
            assert( numIslands( grid ) == 3 );
        }

        {
            Array<Array<c8>> grid{
                { '1', '0', '1' },
                { '0', '1', '0' },
                { '1', '0', '1' },
            };
            assert( numIslands( grid ) == 5 );
        }
    }

    void TestNumIslands::resetCheck( Array<Array<bool>> &checked )
    {
        for( u32 x = 0; x < (u32)checked.size(); ++x )
        {
            for( u32 y = 0; y < (u32)checked[x].size(); ++y )
            {
                checked[x][y] = false;
            }
        }
    }

    bool TestNumIslands::markIsland( Array<Array<c8>> &grid, Array<Array<bool>> &checked, s32 x, s32 y )
    {
        //std::cout << "visit " << x << "," << y << std::endl;

        bool result = false;
        if( x >= 0 && x < grid.size() )
        {
            if( y >= 0 && y < grid[x].size() )
            {
                if( checked[x][y] == false )
                {
                    if( grid[x][y] != '0' )
                    {
                        //std::cout << "marked " << x << "," << y << std::endl;
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
                    //std::cout << "visited " << x << "," << y << std::endl;
                }
            }
        }

        return result;
    }

    s32 TestNumIslands::numIslands( Array<Array<c8>> &grid )
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
                //std::cout << "check " << x << "," << y << std::endl;

                if( grid[x][y] != '0' )
                {
                    if( markIsland( grid, checked, x, y ) )
                    {
                        //std::cout << x << "," << y << std::endl;
                        count++;
                    }
                }
            }
        }

        return count;
    }

}  // namespace workphone
