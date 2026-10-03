#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CRoadMeshElement.hpp"
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone::procedural, CRoadMeshElement,
                                   CProceduralObject<IRoadMeshElement> );

        // ---------------------------------------------------------------
        // Property key constants
        // ---------------------------------------------------------------

        static const char *const kPropParentSection = "parentSection";
        static const char *const kPropVertexCount = "vertexCount";
        static const char *const kPropIndexCount = "indexCount";
        static const char *const kPropPolygonCount = "polygonCount";
        static const char *const kPropWidth = "width";
        static const char *const kPropWidthA = "widthA";
        static const char *const kPropWidthB = "widthB";
        static const char *const kPropPavementWidth = "pavementWidth";
        static const char *const kPropIsBuilt = "isBuilt";

        // ---------------------------------------------------------------
        // Internal constants
        // ---------------------------------------------------------------

        static constexpr f32 kMinWidth = 0.001f;
        static constexpr f32 kMaxWidth = 10000.0f;
        static constexpr f32 kDefaultWidth = 3.5f;
        static constexpr u32 kMinPointsForBuild = 2u;
        static constexpr u32 kIndicesPerTriangle = 3u;

        // ---------------------------------------------------------------
        // Lifetime
        // ---------------------------------------------------------------

        CRoadMeshElement::CRoadMeshElement() :
            Width( kDefaultWidth ),
            WidthA( kDefaultWidth ),
            WidthB( kDefaultWidth ),
            PavementWidth( 0.5f ),
            m_isBuilt( false )
        {
        }

        CRoadMeshElement::~CRoadMeshElement()
        {
            unload( nullptr );
        }

        void CRoadMeshElement::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                m_parentSection = nullptr;

                m_vertices.clear();
                m_indices.clear();
                m_polygons.clear();

                TangentA = Vector3F( 0.0f, 0.0f, 0.0f );
                TangentB = Vector3F( 0.0f, 0.0f, 0.0f );
                CenterLine = Line3F();
                Points.clear();
                Box = AABB3F();
                Width = kDefaultWidth;
                WidthA = kDefaultWidth;
                WidthB = kDefaultWidth;
                PavementWidth = 0.5f;

                m_isBuilt = false;

                CProceduralObject<IRoadMeshElement>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Parent section management
        // ---------------------------------------------------------------

        SmartPtr<IRoadSection> CRoadMeshElement::getParentSection() const
        {
            return m_parentSection;
        }

        void CRoadMeshElement::setParentSection( SmartPtr<IRoadSection> section )
        {
            m_parentSection = section;
        }

        // ---------------------------------------------------------------
        // Vertex management
        // ---------------------------------------------------------------

        Array<Vector3<real_Num>> CRoadMeshElement::getVertices() const
        {
            return m_vertices;
        }

        void CRoadMeshElement::setVertices( const Array<Vector3<real_Num>> &vertices )
        {
            try
            {
                if( vertices.empty() )
                {
                    WP_LOG_WARNING( "CRoadMeshElement::setVertices - empty vertex array provided." );
                    m_vertices.clear();
                    invalidateMesh();
                    return;
                }

                m_vertices = vertices;
                invalidateMesh();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadMeshElement::addVertex( const Vector3<real_Num> &vertex )
        {
            try
            {
                m_vertices.push_back( vertex );
                invalidateMesh();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool CRoadMeshElement::removeVertex( u32 index )
        {
            try
            {
                if( index >= m_vertices.size() )
                {
                    WP_LOG_WARNING( "CRoadMeshElement::removeVertex - index out of bounds." );
                    return false;
                }

                m_vertices.erase( m_vertices.begin() + index );
                invalidateMesh();
                return true;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                return false;
            }
        }

        bool CRoadMeshElement::getVertex( u32 index, Vector3<real_Num> &outVertex ) const
        {
            if( index >= m_vertices.size() )
            {
                return false;
            }
            outVertex = m_vertices[index];
            return true;
        }

        void CRoadMeshElement::clearVertices()
        {
            m_vertices.clear();
            invalidateMesh();
        }

        u32 CRoadMeshElement::getVertexCount() const
        {
            return static_cast<u32>( m_vertices.size() );
        }

        // ---------------------------------------------------------------
        // Index management
        // ---------------------------------------------------------------

        Array<u32> CRoadMeshElement::getIndices() const
        {
            return m_indices;
        }

        void CRoadMeshElement::setIndices( const Array<u32> &indices )
        {
            try
            {
                if( !indices.empty() && ( indices.size() % kIndicesPerTriangle != 0 ) )
                {
                    WP_LOG_WARNING(
                        "CRoadMeshElement::setIndices - index count is not a multiple of 3 "
                        "(triangles)." );
                }

                if( !indices.empty() && !m_vertices.empty() )
                {
                    for( u32 i = 0; i < indices.size(); ++i )
                    {
                        if( indices[i] >= m_vertices.size() )
                        {
                            WP_LOG_ERROR( "CRoadMeshElement::setIndices - index out of vertex bounds." );
                            return;
                        }
                    }
                }

                m_indices = indices;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadMeshElement::addIndex( u32 index )
        {
            try
            {
                m_indices.push_back( index );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool CRoadMeshElement::removeIndex( u32 index )
        {
            try
            {
                if( index >= m_indices.size() )
                {
                    WP_LOG_WARNING( "CRoadMeshElement::removeIndex - index out of bounds." );
                    return false;
                }

                m_indices.erase( m_indices.begin() + index );
                return true;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                return false;
            }
        }

        void CRoadMeshElement::clearIndices()
        {
            m_indices.clear();
        }

        u32 CRoadMeshElement::getIndexCount() const
        {
            return static_cast<u32>( m_indices.size() );
        }

        // ---------------------------------------------------------------
        // Polygon management
        // ---------------------------------------------------------------

        Array<Polygon3<real_Num>> CRoadMeshElement::getPolygons() const
        {
            return m_polygons;
        }

        void CRoadMeshElement::setPolygons( const Array<Polygon3<real_Num>> &polygons )
        {
            try
            {
                for( const auto &poly : polygons )
                {
                    if( poly.getNumPoints() < 3u )
                    {
                        WP_LOG_ERROR(
                            "CRoadMeshElement::setPolygons - polygon with less than 3 vertices." );
                        return;
                    }
                }

                m_polygons = polygons;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadMeshElement::clearPolygons()
        {
            m_polygons.clear();
        }

        u32 CRoadMeshElement::getPolygonCount() const
        {
            return static_cast<u32>( m_polygons.size() );
        }

        // ---------------------------------------------------------------
        // Width validation helpers
        // ---------------------------------------------------------------

        bool CRoadMeshElement::isWidthValid( f32 width ) const
        {
            return ( width >= kMinWidth ) && ( width <= kMaxWidth );
        }

        f32 CRoadMeshElement::clampWidth( f32 width ) const
        {
            return std::clamp( width, kMinWidth, kMaxWidth );
        }

        void CRoadMeshElement::invalidateMesh()
        {
            m_isBuilt = false;
        }

        // ---------------------------------------------------------------
        // Width setters with validation
        // ---------------------------------------------------------------

        void CRoadMeshElement::setWidth( f32 width )
        {
            if( !isWidthValid( width ) )
            {
                WP_LOG_WARNING( "CRoadMeshElement::setWidth - width out of valid range, clamping." );
                Width = clampWidth( width );
            }
            else
            {
                Width = width;
            }
            invalidateMesh();
        }

        void CRoadMeshElement::setWidthA( f32 widthA )
        {
            if( !isWidthValid( widthA ) )
            {
                WP_LOG_WARNING( "CRoadMeshElement::setWidthA - width out of valid range, clamping." );
                WidthA = clampWidth( widthA );
            }
            else
            {
                WidthA = widthA;
            }
            invalidateMesh();
        }

        void CRoadMeshElement::setWidthB( f32 widthB )
        {
            if( !isWidthValid( widthB ) )
            {
                WP_LOG_WARNING( "CRoadMeshElement::setWidthB - width out of valid range, clamping." );
                WidthB = clampWidth( widthB );
            }
            else
            {
                WidthB = widthB;
            }
            invalidateMesh();
        }

        void CRoadMeshElement::setPavementWidth( f32 pavementWidth )
        {
            if( pavementWidth < 0.0f )
            {
                WP_LOG_WARNING(
                    "CRoadMeshElement::setPavementWidth - negative width not allowed, setting to 0." );
                PavementWidth = 0.0f;
            }
            else if( pavementWidth > kMaxWidth )
            {
                WP_LOG_WARNING(
                    "CRoadMeshElement::setPavementWidth - width exceeds maximum, clamping." );
                PavementWidth = kMaxWidth;
            }
            else
            {
                PavementWidth = pavementWidth;
            }
            invalidateMesh();
        }

        // ---------------------------------------------------------------
        // Points management (spline control points)
        // ---------------------------------------------------------------

        void CRoadMeshElement::setPoints( const Array<Vector3F> &points )
        {
            try
            {
                if( points.size() < kMinPointsForBuild )
                {
                    WP_LOG_WARNING( "CRoadMeshElement::setPoints - insufficient points for road mesh." );
                }

                Points = points;
                invalidateMesh();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRoadMeshElement::addPoint( const Vector3F &point )
        {
            try
            {
                Points.push_back( point );
                invalidateMesh();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool CRoadMeshElement::removePoint( u32 index )
        {
            try
            {
                if( index >= Points.size() )
                {
                    WP_LOG_WARNING( "CRoadMeshElement::removePoint - index out of bounds." );
                    return false;
                }

                Points.erase( Points.begin() + index );
                invalidateMesh();
                return true;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                return false;
            }
        }

        void CRoadMeshElement::clearPoints()
        {
            Points.clear();
            invalidateMesh();
        }

        u32 CRoadMeshElement::getPointCount() const
        {
            return static_cast<u32>( Points.size() );
        }

        // ---------------------------------------------------------------
        // Build implementation
        // ---------------------------------------------------------------

        void CRoadMeshElement::build()
        {
            try
            {
                if( Points.size() < kMinPointsForBuild )
                {
                    WP_LOG_ERROR( "CRoadMeshElement::build - insufficient points (need at least 2)." );
                    return;
                }

                if( !isWidthValid( Width ) && !isWidthValid( WidthA ) && !isWidthValid( WidthB ) )
                {
                    WP_LOG_ERROR( "CRoadMeshElement::build - all widths are invalid." );
                    return;
                }

                updateCenterLine();
                updateTangents();
                generateVertices();
                generateIndices();
                updateBounds();

                m_isBuilt = true;

                WP_LOG_INFO( "CRoadMeshElement::build - mesh built successfully." );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                m_isBuilt = false;
            }
        }

        // ---------------------------------------------------------------
        // Update mesh (incremental)
        // ---------------------------------------------------------------

        void CRoadMeshElement::updateMesh()
        {
            try
            {
                if( !m_isBuilt )
                {
                    build();
                    return;
                }

                updateCenterLine();
                updateTangents();
                regenerateVertices();
                updateBounds();

                WP_LOG_INFO( "CRoadMeshElement::updateMesh - mesh updated successfully." );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ---------------------------------------------------------------
        // Private helper methods
        // ---------------------------------------------------------------

        void CRoadMeshElement::updateCenterLine()
        {
            if( Points.size() < kMinPointsForBuild )
            {
                return;
            }

            const auto &start = Points[0];
            const auto &end = Points[Points.size() - 1];
            CenterLine = Line3F( start, end );
        }

        void CRoadMeshElement::updateTangents()
        {
            if( Points.size() < kMinPointsForBuild )
            {
                TangentA = Vector3F( 0.0f, 0.0f, 0.0f );
                TangentB = Vector3F( 0.0f, 0.0f, 0.0f );
                return;
            }

            // Calculate tangent at start point
            if( Points.size() >= 2u )
            {
                TangentA = ( Points[1] - Points[0] ).normaliseCopy();
            }
            else
            {
                TangentA = Vector3F( 1.0f, 0.0f, 0.0f );
            }

            // Calculate tangent at end point
            const auto lastIdx = Points.size() - 1u;
            if( Points.size() >= 2u )
            {
                TangentB = ( Points[lastIdx] - Points[lastIdx - 1u] ).normaliseCopy();
            }
            else
            {
                TangentB = Vector3F( 1.0f, 0.0f, 0.0f );
            }
        }

        void CRoadMeshElement::generateVertices()
        {
            m_vertices.clear();

            if( Points.size() < kMinPointsForBuild )
            {
                return;
            }

            const auto segmentCount = Points.size() - 1u;

            for( size_t i = 0; i < Points.size(); ++i )
            {
                const auto &point = Points[i];

                // Interpolate width along the road
                f32 t =
                    segmentCount > 0u ? static_cast<f32>( i ) / static_cast<f32>( segmentCount ) : 0.0f;
                f32 currentWidth = lerpWidth( t );

                // Calculate perpendicular offset for left and right edges
                const auto pointIndex = static_cast<u32>( i );
                Vector3F leftOffset = calculatePerpendicularOffset( pointIndex, currentWidth, true );
                Vector3F rightOffset = calculatePerpendicularOffset( pointIndex, currentWidth, false );

                // Add left and right vertices
                m_vertices.push_back( static_cast<Vector3<real_Num>>( point + leftOffset ) );
                m_vertices.push_back( static_cast<Vector3<real_Num>>( point + rightOffset ) );
            }
        }

        void CRoadMeshElement::regenerateVertices()
        {
            generateVertices();
        }

        f32 CRoadMeshElement::lerpWidth( f32 t ) const
        {
            t = std::clamp( t, 0.0f, 1.0f );
            return WidthA + t * ( WidthB - WidthA );
        }

        Vector3F CRoadMeshElement::calculatePerpendicularOffset( u32 pointIndex, f32 width,
                                                                 bool isLeft ) const
        {
            Vector3F tangent;

            // Get tangent direction
            if( pointIndex == 0u )
            {
                tangent = TangentA;
            }
            else if( pointIndex >= Points.size() - 1u )
            {
                tangent = TangentB;
            }
            else
            {
                const auto &prev = Points[pointIndex - 1u];
                const auto &next = Points[pointIndex + 1u];
                tangent = ( next - prev ).normaliseCopy();
            }

            // Calculate perpendicular (cross with up vector)
            Vector3F up = Vector3F( 0.0f, 1.0f, 0.0f );
            Vector3F perp = tangent.crossProduct( up );

            // Ensure we have a valid perpendicular
            if( perp.lengthSquared() < 0.0001f )
            {
                perp = Vector3F( 0.0f, 0.0f, 1.0f ).crossProduct( tangent );
            }
            perp.normalise();

            // Apply half-width and left/right direction
            f32 direction = isLeft ? 1.0f : -1.0f;
            return perp * ( width * 0.5f * direction );
        }

        void CRoadMeshElement::generateIndices()
        {
            m_indices.clear();

            const auto vertexPairCount = Points.size();
            if( vertexPairCount < kMinPointsForBuild )
            {
                return;
            }

            // Generate triangle indices for quads between consecutive points
            for( size_t i = 0; i < vertexPairCount - 1u; ++i )
            {
                const auto pairIndex = static_cast<u32>( i );
                u32 bottomLeft = pairIndex * 2u;
                u32 bottomRight = pairIndex * 2u + 1u;
                u32 topLeft = ( pairIndex + 1u ) * 2u;
                u32 topRight = ( pairIndex + 1u ) * 2u + 1u;

                // First triangle (bottom-left, top-left, bottom-right)
                m_indices.push_back( bottomLeft );
                m_indices.push_back( topLeft );
                m_indices.push_back( bottomRight );

                // Second triangle (bottom-right, top-left, top-right)
                m_indices.push_back( bottomRight );
                m_indices.push_back( topLeft );
                m_indices.push_back( topRight );
            }
        }

        void CRoadMeshElement::updateBounds()
        {
            if( m_vertices.empty() )
            {
                Box = AABB3F();
                return;
            }

            Box = AABB3F( m_vertices[0], m_vertices[0] );
            for( const auto &vertex : m_vertices )
            {
                Box.merge( vertex );
            }
        }

        // ---------------------------------------------------------------
        // Utility methods
        // ---------------------------------------------------------------

        Vector3F CRoadMeshElement::getCenter() const
        {
            return Box.getCenter();
        }

        f32 CRoadMeshElement::getLength() const
        {
            if( Points.size() < kMinPointsForBuild )
            {
                return 0.0f;
            }

            f32 totalLength = 0.0f;
            for( u32 i = 0; i < Points.size() - 1u; ++i )
            {
                totalLength += ( Points[i + 1u] - Points[i] ).length();
            }
            return totalLength;
        }

        f32 CRoadMeshElement::getAverageWidth() const
        {
            return ( WidthA + WidthB ) * 0.5f;
        }

        bool CRoadMeshElement::isValid() const
        {
            return ( Points.size() >= kMinPointsForBuild ) && m_isBuilt && ( m_vertices.size() >= 4u );
        }

    }  // namespace procedural
}  // namespace workphone
