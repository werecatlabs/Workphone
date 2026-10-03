#ifndef TicTacToe_h__
#define TicTacToe_h__

#include <Workphone/Test/Fakes/ShapeFake.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Map.hpp>

namespace workphone
{
    namespace test
    {

        class WPCore_API TicTacToe
        {
        public:
            Array<Array<int>> board;
            int n;

            TicTacToe( int n );

            int move( int row, int col, int player );

            bool checkDiagonal( int player );

            bool checkAntiDiagonal( int player );

            bool checkCol( int col, int player );

            bool checkRow( int row, int player );
        };

    }  // namespace test
}  // namespace workphone

#endif  // TicTacToe_h__
