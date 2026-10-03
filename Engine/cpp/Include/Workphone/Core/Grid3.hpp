#ifndef WP_Grid3_H
#define WP_Grid3_H

#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/AABB3.hpp>

#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone
{

    /**
     * @brief Represents a 3D regular grid of axis-aligned cells.
     *
     * Grid3 provides utilities for mapping between cell indices and world
     * positions, querying the axis-aligned bounding box of individual cells,
     * and storing grid parameters such as center, cell size and overall
     * extents.
     */
    class WPCore_API Grid3
    {
    public:
        /**
         * @brief Construct a new Grid3 instance.
         *
         * The grid will be created in an uninitialised state. Call
         * initialise() before using any coordinate conversion helpers.
         */
        Grid3();

        /**
         * @brief Destroy the Grid3 instance.
         */
        ~Grid3();

        /**
         * @brief Initialise the grid parameters.
         *
         * @param center World-space center of the grid.
         * @param cellSize Size of a single grid cell along each axis.
         * @param halfExtents Half the total size (extents) of the grid along each axis.
         *
         * The full grid extents are derived from the provided halfExtents.
         * After initialisation, conversion helpers (calculatePosition,
         * calculateGridSlot, getCellBox) will use these parameters.
         */
        void initialise( const Vector3<real_Num> &center, const Vector3<real_Num> &cellSize,
                         const Vector3<real_Num> &halfExtents );

        /**
         * @brief Calculate the world-space minimum corner of the specified grid cell.
         *
         * @param cellIdx Integer cell index in grid coordinates (X, Y, Z).
         * @param[out] position Receives the world-space minimum corner for the
         *                       specified cell index.
         */
        void calculatePosition( const Vector3I &cellIdx, Vector3<real_Num> &position ) const;

        /**
         * @brief Convert a world-space position to a grid cell index.
         *
         * @param position World-space position to query.
         * @param[out] cellIndex Integer cell index that contains the position.
         *
         * Note: this performs a simple floor/truncate conversion and does not
         * clamp indices to any specific range. Consumer code should validate
         * indices if needed.
         */
        void calculateGridSlot( const Vector3<real_Num> &position, Vector3I &cellIndex ) const;

        /**
         * @brief Get the axis-aligned bounding box of the specified cell.
         *
         * @param cellIdx Integer index of the cell.
         * @return AABB3<real_Num> The world-space bounding box covering the cell.
         */
        AABB3<real_Num> getCellBox( const Vector3I &cellIdx ) const;

        /**
         * @brief Check whether initialise() has been called with valid grid values.
         */
        bool isInitialised() const;

        /**
         * @brief Check whether a cell index is inside [0, getNumCells()) on every axis.
         */
        bool isValidCellIndex( const Vector3I &cellIdx ) const;

        /**
         * @brief Check whether a world position is inside the grid extents.
         */
        bool containsPosition( const Vector3<real_Num> &position ) const;

        /**
         * @brief Get the world-space center of the grid.
         *
         * @return const Vector3<real_Num>& Grid center.
         */
        const Vector3<real_Num> &getCenter() const;

        /**
         * @brief Get the half-extents of the grid (distance from center to
         *        each boundary along each axis).
         *
         * @return const Vector3<real_Num>& Grid half-extents.
         */
        const Vector3<real_Num> &getExtents() const;

        /**
         * @brief Get the size of a single grid cell along each axis.
         *
         * @return const Vector3<real_Num>& Cell size.
         */
        const Vector3<real_Num> &getCellSize() const;

        /**
         * @brief Get the number of cells along each axis in the grid.
         *
         * This is derived from the grid extents and cell size.
         *
         * @return Vector3I Number of cells (X, Y, Z).
         */
        Vector3I getNumCells() const;

    private:
        /// @brief The world-space center of the grid.
        Vector3<real_Num> m_center;

        /// @brief The half-extents of the grid (distance from center to edge).
        Vector3<real_Num> m_gridExtents;

        /// @brief Size of an individual grid cell along each axis.
        Vector3<real_Num> m_cellSize;

        /// @brief True after initialise() succeeds with sane values.
        bool m_initialised = false;

        static bool isFinite( real_Num value )
        {
            return std::isfinite( static_cast<double>( value ) );
        }

        static bool isFinite( const Vector3<real_Num> &value )
        {
            return isFinite( value.X() ) && isFinite( value.Y() ) && isFinite( value.Z() );
        }

        bool hasPositiveCellSize() const
        {
            return m_cellSize.X() > real_Num( 0 ) && m_cellSize.Y() > real_Num( 0 ) &&
                   m_cellSize.Z() > real_Num( 0 );
        }

        bool hasNonNegativeExtents() const
        {
            return m_gridExtents.X() >= real_Num( 0 ) && m_gridExtents.Y() >= real_Num( 0 ) &&
                   m_gridExtents.Z() >= real_Num( 0 );
        }

        void assertReady() const
        {
            assert( m_initialised );
            assert( isFinite( m_center ) );
            assert( isFinite( m_cellSize ) );
            assert( isFinite( m_gridExtents ) );
            assert( hasPositiveCellSize() );
            assert( hasNonNegativeExtents() );

            if( !m_initialised || !isFinite( m_center ) || !isFinite( m_cellSize ) ||
                !isFinite( m_gridExtents ) || !hasPositiveCellSize() || !hasNonNegativeExtents() )
            {
                throw std::logic_error( "Grid3 is not initialised with valid values" );
            }
        }
    };

    /**
     * @brief Inline implementation: compute the world-space minimum corner of a cell.
     *
     * Cell (0,0,0) starts at center - extents. This mirrors Grid2 and makes
     * getCellBox(cell).getMinimum() line up with calculatePosition(cell).
     */
    inline void Grid3::calculatePosition( const Vector3I &cellIdx, Vector3<real_Num> &position ) const
    {
        assertReady();
        assert( isValidCellIndex( cellIdx ) );
        if( !isValidCellIndex( cellIdx ) )
        {
            throw std::out_of_range( "Grid3::calculatePosition cell index is outside the grid" );
        }

        position.X() =
            ( static_cast<real_Num>( cellIdx.X() ) * m_cellSize.X() ) + m_center.X() - m_gridExtents.X();
        position.Y() =
            ( static_cast<real_Num>( cellIdx.Y() ) * m_cellSize.Y() ) + m_center.Y() - m_gridExtents.Y();
        position.Z() =
            ( static_cast<real_Num>( cellIdx.Z() ) * m_cellSize.Z() ) + m_center.Z() - m_gridExtents.Z();

        assert( isFinite( position ) );
    }

    /**
     * @brief Inline implementation: convert world-space position to cell index.
     */
    inline void Grid3::calculateGridSlot( const Vector3<real_Num> &position, Vector3I &cellIndex ) const
    {
        assertReady();
        assert( isFinite( position ) );
        if( !isFinite( position ) )
        {
            throw std::invalid_argument( "Grid3::calculateGridSlot position must be finite" );
        }

        Vector3<real_Num> offset = ( position - m_center ) + m_gridExtents;
        Vector3<real_Num> result = offset / m_cellSize;

        const real_Num tolerance = static_cast<real_Num>( 0.0015 );
        assert( result.X() >= static_cast<real_Num>( std::numeric_limits<s32>::min() ) );
        assert( result.X() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
        assert( result.Y() >= static_cast<real_Num>( std::numeric_limits<s32>::min() ) );
        assert( result.Y() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
        assert( result.Z() >= static_cast<real_Num>( std::numeric_limits<s32>::min() ) );
        assert( result.Z() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );

        if( result.X() < static_cast<real_Num>( std::numeric_limits<s32>::min() ) ||
            result.X() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
            result.Y() < static_cast<real_Num>( std::numeric_limits<s32>::min() ) ||
            result.Y() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
            result.Z() < static_cast<real_Num>( std::numeric_limits<s32>::min() ) ||
            result.Z() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) )
        {
            throw std::out_of_range( "Grid3::calculateGridSlot result is outside s32 range" );
        }

        cellIndex.set( static_cast<s32>( result.X() + tolerance ),
                       static_cast<s32>( result.Y() + tolerance ),
                       static_cast<s32>( result.Z() + tolerance ) );
    }

    /**
     * @brief Inline implementation: return the AABB of the given cell index.
     */
    inline AABB3<real_Num> Grid3::getCellBox( const Vector3I &cellIdx ) const
    {
        assertReady();
        assert( isValidCellIndex( cellIdx ) );
        if( !isValidCellIndex( cellIdx ) )
        {
            throw std::out_of_range( "Grid3::getCellBox cell index is outside the grid" );
        }

        Vector3<real_Num> minimum;
        calculatePosition( cellIdx, minimum );
        auto maximum = minimum + m_cellSize;

        assert( minimum.X() <= maximum.X() );
        assert( minimum.Y() <= maximum.Y() );
        assert( minimum.Z() <= maximum.Z() );

        return AABB3<real_Num>( minimum, maximum );
    }

    inline bool Grid3::isInitialised() const
    {
        return m_initialised && isFinite( m_center ) && isFinite( m_cellSize ) &&
               isFinite( m_gridExtents ) && hasPositiveCellSize() && hasNonNegativeExtents();
    }

    inline bool Grid3::isValidCellIndex( const Vector3I &cellIdx ) const
    {
        assert( isInitialised() );
        if( !isInitialised() )
        {
            return false;
        }

        const auto numCells = getNumCells();
        return cellIdx.X() >= 0 && cellIdx.Y() >= 0 && cellIdx.Z() >= 0 && cellIdx.X() < numCells.X() &&
               cellIdx.Y() < numCells.Y() && cellIdx.Z() < numCells.Z();
    }

    inline bool Grid3::containsPosition( const Vector3<real_Num> &position ) const
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

        const auto minimum = m_center - m_gridExtents;
        const auto maximum = m_center + m_gridExtents;

        return position.X() >= minimum.X() && position.X() <= maximum.X() &&
               position.Y() >= minimum.Y() && position.Y() <= maximum.Y() &&
               position.Z() >= minimum.Z() && position.Z() <= maximum.Z();
    }

}  // namespace workphone

#endif
