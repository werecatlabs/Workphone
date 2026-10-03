#ifndef WP_Grid2d_H
#define WP_Grid2d_H

#include <Workphone/Math/Vector2.hpp>

#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone
{
    /** Represents a 2d grid. */
    class WPCore_API Grid2
    {
    public:
        /** Default constructor*/
        Grid2();

        /** Constructor. */
        Grid2( const Vector2<real_Num> &center, const Vector2<real_Num> &cellSize,
               const Vector2<real_Num> &halfExtents );

        /** Destructor*/
        ~Grid2();

        /** Calculates the world-space minimum corner of the cell index passed. */
        void calculatePosition( const Vector2I &cellIndex, Vector2<real_Num> &position ) const;

        /** Calculates the cell the position pass is in. */
        void calculateCell( const Vector2<real_Num> &position, Vector2I &cellIndex ) const;

        /** Calculates the cell by rounding up. */
        void calculateCellRound( const Vector2<real_Num> &position, Vector2I &cellIndex ) const;

        /** Checks whether the grid has valid finite extents and a positive cell size. */
        bool isInitialised() const;

        /** Checks whether a cell index is inside [0, getNumCells()) on both axes. */
        bool isValidCellIndex( const Vector2I &cellIndex ) const;

        /** Checks whether a world-space position is inside the grid extents. */
        bool containsPosition( const Vector2<real_Num> &position ) const;

        /** Sets the center of the grid. */
        void setCenter( const Vector2<real_Num> &center );

        /** Gets the center of the grid. */
        const Vector2<real_Num> &getCenter() const;

        /** Sets the extents of the grid. */
        void setExtents( const Vector2<real_Num> &halfExtents );

        /** Gets the extents of the grid. */
        const Vector2<real_Num> &getExtents() const;

        /** Sets the cell size. */
        void setCellSize( const Vector2<real_Num> &cellSize );

        /** Gets the cell size. */
        const Vector2<real_Num> &getCellSize() const;

        /** Gets the number of the cells. */
        Vector2I getNumCells() const;

    private:
        /// The grid's center position.
        Vector2<real_Num> m_center;

        /// The boundaries of the grid.
        Vector2<real_Num> m_halfExtents;

        /// The size of each cell.
        Vector2<real_Num> m_cellSize;

        static bool isFinite( real_Num value )
        {
            return std::isfinite( static_cast<double>( value ) );
        }

        static bool isFinite( const Vector2<real_Num> &value )
        {
            return isFinite( value.X() ) && isFinite( value.Y() );
        }

        bool hasPositiveCellSize() const
        {
            return m_cellSize.X() > real_Num( 0 ) && m_cellSize.Y() > real_Num( 0 );
        }

        bool hasNonNegativeExtents() const
        {
            return m_halfExtents.X() >= real_Num( 0 ) && m_halfExtents.Y() >= real_Num( 0 );
        }

        void assertReady() const
        {
            assert( isFinite( m_center ) );
            assert( isFinite( m_cellSize ) );
            assert( isFinite( m_halfExtents ) );
            assert( hasPositiveCellSize() );
            assert( hasNonNegativeExtents() );

            if( !isInitialised() )
            {
                throw std::logic_error( "Grid2 is not initialised with valid values" );
            }
        }

        static void checkS32Range( const Vector2<real_Num> &value, const char *message )
        {
            assert( value.X() >= static_cast<real_Num>( std::numeric_limits<s32>::min() ) );
            assert( value.X() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
            assert( value.Y() >= static_cast<real_Num>( std::numeric_limits<s32>::min() ) );
            assert( value.Y() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );

            if( value.X() < static_cast<real_Num>( std::numeric_limits<s32>::min() ) ||
                value.X() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
                value.Y() < static_cast<real_Num>( std::numeric_limits<s32>::min() ) ||
                value.Y() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) )
            {
                throw std::out_of_range( message );
            }
        }
    };

    inline void Grid2::calculatePosition( const Vector2I &cellIndex, Vector2<real_Num> &position ) const
    {
        assertReady();
        assert( isValidCellIndex( cellIndex ) );
        if( !isValidCellIndex( cellIndex ) )
        {
            throw std::out_of_range( "Grid2::calculatePosition cell index is outside the grid" );
        }

        position.X() = ( ( static_cast<real_Num>( cellIndex.X() ) * m_cellSize.X() ) + m_center.X() ) -
                       m_halfExtents.X();
        position.Y() = ( ( static_cast<real_Num>( cellIndex.Y() ) * m_cellSize.Y() ) + m_center.Y() ) -
                       m_halfExtents.Y();

        assert( isFinite( position ) );
    }

    inline void Grid2::calculateCell( const Vector2<real_Num> &position, Vector2I &cellIndex ) const
    {
        assertReady();
        assert( isFinite( position ) );
        if( !isFinite( position ) )
        {
            throw std::invalid_argument( "Grid2::calculateCell position must be finite" );
        }

        Vector2<real_Num> offset = ( position - m_center ) + m_halfExtents;
        Vector2<real_Num> result = offset / m_cellSize;
        checkS32Range( result, "Grid2::calculateCell result is outside s32 range" );

        const real_Num tollerance = static_cast<real_Num>( 0.0015 );
        cellIndex.set( static_cast<s32>( result.X() + tollerance ),
                       static_cast<s32>( result.Y() + tollerance ) );
    }

    inline void Grid2::calculateCellRound( const Vector2<real_Num> &position, Vector2I &cellIndex ) const
    {
        assertReady();
        assert( isFinite( position ) );
        if( !isFinite( position ) )
        {
            throw std::invalid_argument( "Grid2::calculateCellRound position must be finite" );
        }

        Vector2<real_Num> offset = ( position - m_center ) + m_halfExtents;
        Vector2<real_Num> result = offset / m_cellSize;
        checkS32Range( result, "Grid2::calculateCellRound result is outside s32 range" );

        cellIndex.set( Math<real_Num>::Round( result.X() ), Math<real_Num>::Round( result.Y() ) );
    }

    inline bool Grid2::isInitialised() const
    {
        return isFinite( m_center ) && isFinite( m_cellSize ) && isFinite( m_halfExtents ) &&
               hasPositiveCellSize() && hasNonNegativeExtents();
    }

    inline bool Grid2::isValidCellIndex( const Vector2I &cellIndex ) const
    {
        assert( isInitialised() );
        if( !isInitialised() )
        {
            return false;
        }

        const auto numCells = getNumCells();
        return cellIndex.X() >= 0 && cellIndex.Y() >= 0 && cellIndex.X() < numCells.X() &&
               cellIndex.Y() < numCells.Y();
    }

    inline bool Grid2::containsPosition( const Vector2<real_Num> &position ) const
    {
        assert( isInitialised() );
        if( !isInitialised() )
        {
            return false;
        }

        assert( isFinite( position ) );
        if( !isFinite( position ) )
        {
            return false;
        }

        const auto minimum = m_center - m_halfExtents;
        const auto maximum = m_center + m_halfExtents;

        return position.X() >= minimum.X() && position.X() <= maximum.X() &&
               position.Y() >= minimum.Y() && position.Y() <= maximum.Y();
    }

}  // namespace workphone

#endif
