#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Grid2.hpp>

namespace workphone
{

    Grid2::Grid2() = default;

    Grid2::Grid2( const Vector2<real_Num> &center, const Vector2<real_Num> &areaSize,
                  const Vector2<real_Num> &areaBoundry )
    {
        assert( isFinite( center ) );
        assert( isFinite( areaSize ) );
        assert( isFinite( areaBoundry ) );
        assert( areaSize.X() > real_Num( 0 ) );
        assert( areaSize.Y() > real_Num( 0 ) );
        assert( areaBoundry.X() >= real_Num( 0 ) );
        assert( areaBoundry.Y() >= real_Num( 0 ) );

        if( !isFinite( center ) || !isFinite( areaSize ) || !isFinite( areaBoundry ) )
        {
            throw std::invalid_argument(
                "Grid2 constructor requires finite center, cell size, and extents" );
        }

        if( areaSize.X() <= real_Num( 0 ) || areaSize.Y() <= real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid2 constructor cell size must be positive on both axes" );
        }

        if( areaBoundry.X() < real_Num( 0 ) || areaBoundry.Y() < real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid2 constructor half extents must be non-negative" );
        }

        m_center.X() = center.X();
        m_center.Y() = center.Y();

        m_cellSize.X() = areaSize.X();
        m_cellSize.Y() = areaSize.Y();

        m_halfExtents.X() = areaBoundry.X();
        m_halfExtents.Y() = areaBoundry.Y();

        /*m_numNodes = 0;

        m_numBoundingboxes.X() = (int)((m_areaBoundry.X() / m_areaSize.X())*2.0f);
        m_numBoundingboxes.Y() = (int)((m_areaBoundry.Y() / m_areaSize.Y())*2.0f);

        f64 as = 200.0;
        f64 invas = 1.0/as;
        m_invAreaSize.X() = 1.0/(f64)m_areaSize.X();
        m_invAreaSize.Y() = 1.0/(f64)m_areaSize.Y();

        m_maxIndex.X() = m_numBoundingboxes.X() - 1;
        m_maxIndex.Y() = m_numBoundingboxes.Y() - 1;*/
    }

    Grid2::~Grid2() = default;

    void Grid2::setCenter( const Vector2<real_Num> &center )
    {
        assert( isFinite( center ) );
        if( !isFinite( center ) )
        {
            throw std::invalid_argument( "Grid2::setCenter value must be finite" );
        }

        m_center = center;
    }

    auto Grid2::getCenter() const -> const Vector2<real_Num> &
    {
        return m_center;
    }

    void Grid2::setExtents( const Vector2<real_Num> &halfExtents )
    {
        assert( isFinite( halfExtents ) );
        assert( halfExtents.X() >= real_Num( 0 ) );
        assert( halfExtents.Y() >= real_Num( 0 ) );

        if( !isFinite( halfExtents ) )
        {
            throw std::invalid_argument( "Grid2::setExtents value must be finite" );
        }

        if( halfExtents.X() < real_Num( 0 ) || halfExtents.Y() < real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid2::setExtents value must be non-negative" );
        }

        m_halfExtents = halfExtents;
    }

    auto Grid2::getExtents() const -> const Vector2<real_Num> &
    {
        return m_halfExtents;
    }

    void Grid2::setCellSize( const Vector2<real_Num> &cellSize )
    {
        assert( isFinite( cellSize ) );
        assert( cellSize.X() > real_Num( 0 ) );
        assert( cellSize.Y() > real_Num( 0 ) );

        if( !isFinite( cellSize ) )
        {
            throw std::invalid_argument( "Grid2::setCellSize value must be finite" );
        }

        if( cellSize.X() <= real_Num( 0 ) || cellSize.Y() <= real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid2::setCellSize value must be positive" );
        }

        m_cellSize = cellSize;
    }

    auto Grid2::getCellSize() const -> const Vector2<real_Num> &
    {
        return m_cellSize;
    }

    auto Grid2::getNumCells() const -> Vector2I
    {
        assertReady();

        Vector2<real_Num> numCells = ( m_halfExtents / m_cellSize ) * 2.0f;
        assert( numCells.X() >= real_Num( 0 ) );
        assert( numCells.Y() >= real_Num( 0 ) );
        assert( numCells.X() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
        assert( numCells.Y() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );

        if( numCells.X() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
            numCells.Y() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) )
        {
            throw std::out_of_range( "Grid2::getNumCells exceeds s32 range" );
        }

        return { MathF::Round( numCells.X() ), MathF::Round( numCells.Y() ) };
    }
}  // namespace workphone
