#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Design/TicTacToe.hpp"

namespace workphone
{
    namespace test
    {

        TicTacToe::TicTacToe( int n )
        {
            board.assign( n, Array<int>( n, 0 ) );
            this->n = n;
        }

        int TicTacToe::move( int row, int col, int player )
        {
            board[row][col] = player;
            if( checkCol( col, player ) || checkRow( row, player ) ||
                ( row == col && checkDiagonal( player ) ) ||
                ( row == n - col - 1 && checkAntiDiagonal( player ) ) )
            {
                return player;
            }
            // No one wins
            return 0;
        }

        bool TicTacToe::checkDiagonal( int player )
        {
            for( int row = 0; row < n; row++ )
            {
                if( board[row][row] != player )
                    return false;
            }
            return true;
        }

        bool TicTacToe::checkAntiDiagonal( int player )
        {
            for( int row = 0; row < n; row++ )
            {
                if( board[row][n - row - 1] != player )
                    return false;
            }
            return true;
        }

        bool TicTacToe::checkCol( int col, int player )
        {
            for( int row = 0; row < n; row++ )
            {
                if( board[row][col] != player )
                    return false;
            }
            return true;
        }

        bool TicTacToe::checkRow( int row, int player )
        {
            for( int col = 0; col < n; col++ )
            {
                if( board[row][col] != player )
                    return false;
            }
            return true;
        }

    }  // namespace test
}  // namespace workphone
