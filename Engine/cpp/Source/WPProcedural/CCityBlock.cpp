#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/BlockNode.hpp>
#include <Workphone/Interface/Procedural/ILot.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CCityBlock, CProceduralObject<ICityBlock> );

        CCityBlock::CCityBlock() = default;

        CCityBlock::~CCityBlock()
        {
            unload( nullptr );
        }

        void CCityBlock::addPoint( const Vector3<real_Num> &point )
        {
            auto node = workphone::make_ptr<BlockNode>();
            node->setPosition( point );
            addNode( node );
        }

        void CCityBlock::removePoint( const Vector3<real_Num> &point )
        {
            const auto it = std::find_if( m_nodes.begin(), m_nodes.end(), [&]( const auto &node ) {
                return node && node->getPosition() == point;
            } );
            if( it != m_nodes.end() )
            {
                m_nodes.erase( it );
            }
        }

        void CCityBlock::addNode( SmartPtr<IProceduralNode> node )
        {
            if( node && std::find( m_nodes.begin(), m_nodes.end(), node ) == m_nodes.end() )
            {
                m_nodes.push_back( node );
            }
        }

        void CCityBlock::removeNode( SmartPtr<IProceduralNode> node )
        {
            const auto it = std::find( m_nodes.begin(), m_nodes.end(), node );
            if( it != m_nodes.end() )
            {
                m_nodes.erase( it );
            }
        }

        Array<Vector3<real_Num>> CCityBlock::getPoints() const
        {
            Array<Vector3<real_Num>> points;
            points.reserve( m_nodes.size() );
            for( const auto &node : m_nodes )
            {
                if( node )
                {
                    points.push_back( node->getPosition() );
                }
            }
            return points;
        }

        Polygon2<real_Num> CCityBlock::getPolygon() const
        {
            Polygon2<real_Num> polygon;
            for( const auto &point : getPoints() )
            {
                polygon.addPoint( { point.X(), point.Z() } );
            }
            return polygon;
        }

        Polygon3<real_Num> CCityBlock::getPolygon3() const
        {
            return Polygon3<real_Num>( getPoints() );
        }

        AABB3<real_Num> CCityBlock::getBounds() const
        {
            const auto points = getPoints();
            AABB3<real_Num> bounds;
            bounds.setNull();
            if( !points.empty() )
            {
                bounds = AABB3<real_Num>( points.front(), points.front() );
                for( const auto &point : points )
                {
                    bounds.merge( point );
                }
            }
            return bounds;
        }

        void CCityBlock::updateBounds()
        {
            m_Polygon = getPolygon();
            m_Bounds = getBounds();
            if( m_nodes.empty() )
            {
                m_BoundingSphere = Sphere3<real_Num>( Vector3<real_Num>::zero(), 0 );
            }
            else
            {
                m_BoundingSphere = Sphere3<real_Num>( m_Bounds.getCenter(),
                                                      m_Bounds.getExtent().length() * real_Num( 0.5 ) );
            }
        }

        bool CCityBlock::equals( SmartPtr<ICityBlock> other ) const
        {
            if( !other )
            {
                return false;
            }
            auto a = getPoints(), b = other->getPoints();
            // Accept an optional repeated closing point, rotations and either winding.
            if( a.size() > 1 && a.front() == a.back() )
            {
                a.pop_back();
            }
            if( b.size() > 1 && b.front() == b.back() )
            {
                b.pop_back();
            }
            if( a.size() != b.size() )
            {
                return false;
            }
            if( a.empty() )
            {
                return true;
            }
            const auto count = a.size();
            for( size_t start = 0; start < count; ++start )
            {
                bool forward = true, reverse = true;
                for( size_t i = 0; i < count; ++i )
                {
                    forward = forward && a[i] == b[( start + i ) % count];
                    reverse = reverse && a[i] == b[( start + count - i ) % count];
                }
                if( forward || reverse )
                {
                    return true;
                }
            }
            return false;
        }

        SmartPtr<ISharedObject> CCityBlock::clone()
        {
            auto block = workphone::make_ptr<CCityBlock>();
            block->setName( getName() );
            block->setWorldTransform( getWorldTransform() );
            if( m_properties )
            {
                *block->m_properties = *m_properties;
            }
            for( const auto &node : m_nodes )
            {
                if( node )
                {
                    auto copy = workphone::make_ptr<BlockNode>();
                    copy->setName( node->getName() );
                    copy->setWorldTransform( node->getWorldTransform() );
                    block->addNode( copy );
                }
            }
            // Lots have independent scene ownership and are shared rather than duplicated.
            block->m_lots = m_lots;
            block->updateBounds();
            return block;
        }

        Array<SmartPtr<ILot>> CCityBlock::getLots() const
        {
            return m_lots;
        }

        void CCityBlock::setLots( Array<SmartPtr<ILot>> lots )
        {
            m_lots = std::move( lots );
        }

        void CCityBlock::unload( SmartPtr<ISharedObject> data )
        {
            // Release references without unloading nodes/lots still owned by another object.
            m_nodes.clear();
            m_lots.clear();
            CProceduralObject<ICityBlock>::unload( data );
            updateBounds();
        }
    }  // namespace procedural
}  // namespace workphone
