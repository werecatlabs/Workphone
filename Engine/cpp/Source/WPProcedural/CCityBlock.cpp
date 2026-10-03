#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CCityBlock, CProceduralObject<ICityBlock> );

        CCityBlock::CCityBlock()
        {
        }

        CCityBlock::~CCityBlock()
        {
        }

        void CCityBlock::addPoint( const Vector3<real_Num> &point )
        {
            m_points.push_back( point );
        }

        void CCityBlock::removePoint( const Vector3<real_Num> &point )
        {
            auto it = std::find( m_points.begin(), m_points.end(), point );
            if( it != m_points.end() )
            {
                m_points.erase( it );
            }
        }

        Array<Vector3<real_Num>> CCityBlock::getPoints() const
        {
            return m_points;
        }

        Polygon2<real_Num> CCityBlock::getPolygon() const
        {
            Polygon2<real_Num> polygon;

            for( auto p : m_points )
            {
                polygon.addPoint( Vector2<real_Num>( p.X(), p.Z() ) );
            }

            return polygon;
        }

        bool CCityBlock::equals( SmartPtr<ICityBlock> other ) const
        {
            return false;
        }

        Array<SmartPtr<ILot>> CCityBlock::getLots() const
        {
            return m_lots;
        }

        void CCityBlock::setLots( Array<SmartPtr<ILot>> lots )
        {
            m_lots = lots;
        }
    }  // namespace procedural
}  // namespace workphone
