#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Grid3.hpp>

namespace workphone
{

    Grid3::Grid3() = default;

    Grid3::~Grid3() = default;

    void Grid3::initialise( const Vector3<real_Num> &center, const Vector3<real_Num> &areaSize,
                            const Vector3<real_Num> &areaBoundry )
    {
        assert( isFinite( center ) );
        assert( isFinite( areaSize ) );
        assert( isFinite( areaBoundry ) );
        assert( areaSize.X() > real_Num( 0 ) );
        assert( areaSize.Y() > real_Num( 0 ) );
        assert( areaSize.Z() > real_Num( 0 ) );
        assert( areaBoundry.X() >= real_Num( 0 ) );
        assert( areaBoundry.Y() >= real_Num( 0 ) );
        assert( areaBoundry.Z() >= real_Num( 0 ) );

        if( !isFinite( center ) || !isFinite( areaSize ) || !isFinite( areaBoundry ) )
        {
            throw std::invalid_argument(
                "Grid3::initialise requires finite center, cell size, and extents" );
        }

        if( areaSize.X() <= real_Num( 0 ) || areaSize.Y() <= real_Num( 0 ) ||
            areaSize.Z() <= real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid3::initialise cell size must be positive on every axis" );
        }

        if( areaBoundry.X() < real_Num( 0 ) || areaBoundry.Y() < real_Num( 0 ) ||
            areaBoundry.Z() < real_Num( 0 ) )
        {
            throw std::invalid_argument( "Grid3::initialise half extents must be non-negative" );
        }

        m_center.X() = center.X();
        m_center.Y() = center.Y();
        m_center.Z() = center.Z();

        m_cellSize.X() = areaSize.X();
        m_cellSize.Y() = areaSize.Y();
        m_cellSize.Z() = areaSize.Z();

        m_gridExtents.X() = areaBoundry.X();
        m_gridExtents.Y() = areaBoundry.Y();
        m_gridExtents.Z() = areaBoundry.Z();
        m_initialised = true;

        assert( isInitialised() );

        /*
        m_numNodes = 0;

        m_numBoundingboxes.X() = (int)((m_gridExtents.X() / m_cellSize.X())*2.0f);
        m_numBoundingboxes.Y() = (int)((m_gridExtents.Y() / m_cellSize.Y())*2.0f);
        m_numBoundingboxes.Z() = (int)((m_gridExtents.Z() / m_cellSize.Z())*2.0f);

        f64 as = 200.0;
        f64 invas = 1.0/as;
        m_invAreaSize.X() = 1.0/(f64)m_cellSize.X();
        m_invAreaSize.Y() = 1.0/(f64)m_cellSize.Y();
        m_invAreaSize.Z() = 1.0/(f64)m_cellSize.Z();

        m_maxIndex.X() = m_numBoundingboxes.X() - 1;
        m_maxIndex.Y() = m_numBoundingboxes.Y() - 1;
        m_maxIndex.Z() = m_numBoundingboxes.Z() - 1;*/
    }

    auto Grid3::getCenter() const -> const Vector3<real_Num> &
    {
        return m_center;
    }

    auto Grid3::getExtents() const -> const Vector3<real_Num> &
    {
        return m_gridExtents;
    }

    auto Grid3::getCellSize() const -> const Vector3<real_Num> &
    {
        return m_cellSize;
    }

    auto Grid3::getNumCells() const -> Vector3I
    {
        assertReady();

        Vector3<real_Num> numCells = ( m_gridExtents / m_cellSize ) * 2.0f;
        assert( numCells.X() >= real_Num( 0 ) );
        assert( numCells.Y() >= real_Num( 0 ) );
        assert( numCells.Z() >= real_Num( 0 ) );
        assert( numCells.X() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
        assert( numCells.Y() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );
        assert( numCells.Z() <= static_cast<real_Num>( std::numeric_limits<s32>::max() ) );

        if( numCells.X() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
            numCells.Y() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) ||
            numCells.Z() > static_cast<real_Num>( std::numeric_limits<s32>::max() ) )
        {
            throw std::out_of_range( "Grid3::getNumCells exceeds s32 range" );
        }

        return { MathF::Round( numCells.X() ), MathF::Round( numCells.Y() ),
                 MathF::Round( numCells.Z() ) };
    }
}  // namespace workphone
