#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/ProceduralModel.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Properties.hpp>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        // ================================================================
        // Shared constants
        // ================================================================

        static constexpr float k2Pi = 6.28318530717958647692f;
        static constexpr float kMinDimension = 1e-6f;  ///< Smallest acceptable non-zero size.

        // ================================================================
        // ProceduralMesh
        // ================================================================

        void ProceduralMesh::clear()
        {
            vertices.clear();
            indices.clear();
        }

        bool ProceduralMesh::hasGeometry() const
        {
            return !indices.empty() && ( indices.size() % 3 == 0 );
        }

        uint32_t ProceduralMesh::addVertex( const Vector3F &p )
        {
            // Guard: a uint32_t index can address at most UINT32_MAX vertices.
            if( vertices.size() >= static_cast<size_t>( std::numeric_limits<uint32_t>::max() ) )
            {
                WP_LOG_ERROR( "ProceduralMesh::addVertex - vertex limit reached (UINT32_MAX)." );
                return std::numeric_limits<uint32_t>::max();
            }

            vertices.push_back( { p, {} } );
            return static_cast<uint32_t>( vertices.size() - 1 );
        }

        uint32_t ProceduralMesh::addVertex( const ProceduralVertex &v )
        {
            if( vertices.size() >= static_cast<size_t>( std::numeric_limits<uint32_t>::max() ) )
            {
                WP_LOG_ERROR( "ProceduralMesh::addVertex - vertex limit reached (UINT32_MAX)." );
                return std::numeric_limits<uint32_t>::max();
            }
            vertices.push_back( v );
            return static_cast<uint32_t>( vertices.size() - 1 );
        }

        void ProceduralMesh::computeBoundingBox()
        {
            // No-op stub - WPGeometryKit previously provided an empty implementation.
        }

        void ProceduralMesh::addTriangle( uint32_t a, uint32_t b, uint32_t c )
        {
            const auto vSize = static_cast<uint32_t>( vertices.size() );

            if( a >= vSize || b >= vSize || c >= vSize )
            {
                WP_LOG_ERROR(
                    "ProceduralMesh::addTriangle - index out of range "
                    "(a=" +
                    std::to_string( a ) + " b=" + std::to_string( b ) + " c=" + std::to_string( c ) +
                    " vertexCount=" + std::to_string( vSize ) + ")." );
                return;
            }

            indices.push_back( a );
            indices.push_back( b );
            indices.push_back( c );
        }

        void ProceduralMesh::append( const ProceduralMesh &other, const Vector3F &offset )
        {
            if( other.vertices.empty() )
            {
                return;  // Nothing to append.
            }

            // Guard: the combined vertex count must not exceed UINT32_MAX.
            const size_t combinedSize = vertices.size() + other.vertices.size();
            if( combinedSize > static_cast<size_t>( std::numeric_limits<uint32_t>::max() ) )
            {
                WP_LOG_ERROR(
                    "ProceduralMesh::append - combined vertex count would exceed UINT32_MAX." );
                return;
            }

            const uint32_t base = static_cast<uint32_t>( vertices.size() );

            for( const auto &v : other.vertices )
            {
                vertices.push_back( { v.position + offset, v.normal } );
            }

            const uint32_t otherVSize = static_cast<uint32_t>( other.vertices.size() );
            for( auto idx : other.indices )
            {
                if( idx >= otherVSize )
                {
                    WP_LOG_ERROR( "ProceduralMesh::append - source index " + std::to_string( idx ) +
                                  " is out of range in other mesh." );
                    continue;
                }

                indices.push_back( base + idx );
            }
        }

        void ProceduralMesh::computeNormals()
        {
            if( vertices.empty() )
            {
                return;
            }

            // Zero all normals first.
            for( auto &v : vertices )
            {
                v.normal = {};
            }

            // Ignore any trailing incomplete triangle.
            const size_t triCount = indices.size() / 3;

            if( triCount == 0 )
            {
                return;
            }

            const auto vSize = static_cast<uint32_t>( vertices.size() );

            for( size_t t = 0; t < triCount; ++t )
            {
                const uint32_t i0 = indices[t * 3 + 0];
                const uint32_t i1 = indices[t * 3 + 1];
                const uint32_t i2 = indices[t * 3 + 2];

                if( i0 >= vSize || i1 >= vSize || i2 >= vSize )
                {
                    WP_LOG_ERROR( "ProceduralMesh::computeNormals - triangle " + std::to_string( t ) +
                                  " has out-of-range index, skipping." );
                    continue;
                }

                const Vector3F e1 = vertices[i1].position - vertices[i0].position;
                const Vector3F e2 = vertices[i2].position - vertices[i0].position;
                const Vector3F fn = e1.crossProduct( e2 );

                vertices[i0].normal = vertices[i0].normal + fn;
                vertices[i1].normal = vertices[i1].normal + fn;
                vertices[i2].normal = vertices[i2].normal + fn;
            }

            // Normalise each accumulated normal; leave degenerate normals as zero.
            for( auto &v : vertices )
            {
                const float len = v.normal.length();
                if( len > kMinDimension )
                {
                    v.normal = v.normal / len;
                }
            }
        }

        // ================================================================
        // ModelRule base
        // ================================================================

        ModelRule::~ModelRule() = default;

        SmartPtr<Properties> ModelRule::getProperties() const
        {
            return workphone::make_ptr<Properties>();
        }

        void ModelRule::setProperties( SmartPtr<Properties> /*properties*/ )
        {
            // Default: no-op. Derived classes override to apply values.
        }

        // ================================================================
        // BoxRule
        // ================================================================

        BoxRule::BoxRule() = default;

        BoxRule::BoxRule( const Vector3F &size_, const Vector3F &offset_ ) :
            size( size_ ),
            offset( offset_ )
        {
        }

        void BoxRule::build( ProceduralMesh &outMesh ) const
        {
            try
            {
                // Clamp each axis to a minimum positive extent so we never emit
                // degenerate geometry from a zero or negative size component.
                const float sx = std::max( kMinDimension, size.x );
                const float sy = std::max( kMinDimension, size.y );
                const float sz = std::max( kMinDimension, size.z );

                const float hx = sx * 0.5f;
                const float hy = sy * 0.5f;
                const float hz = sz * 0.5f;

                const uint32_t v0 = outMesh.addVertex( offset + Vector3F{ -hx, -hy, -hz } );
                const uint32_t v1 = outMesh.addVertex( offset + Vector3F{ hx, -hy, -hz } );
                const uint32_t v2 = outMesh.addVertex( offset + Vector3F{ hx, hy, -hz } );
                const uint32_t v3 = outMesh.addVertex( offset + Vector3F{ -hx, hy, -hz } );

                const uint32_t v4 = outMesh.addVertex( offset + Vector3F{ -hx, -hy, hz } );
                const uint32_t v5 = outMesh.addVertex( offset + Vector3F{ hx, -hy, hz } );
                const uint32_t v6 = outMesh.addVertex( offset + Vector3F{ hx, hy, hz } );
                const uint32_t v7 = outMesh.addVertex( offset + Vector3F{ -hx, hy, hz } );

                // Abort if any vertex allocation failed (returns UINT32_MAX on overflow).
                const uint32_t sentinel = std::numeric_limits<uint32_t>::max();
                if( v0 == sentinel || v7 == sentinel )
                {
                    WP_LOG_ERROR( "BoxRule::build - vertex buffer overflow, skipping." );
                    return;
                }

                // Back  (-Z face)
                outMesh.addTriangle( v0, v2, v1 );
                outMesh.addTriangle( v0, v3, v2 );
                // Front (+Z face)
                outMesh.addTriangle( v4, v5, v6 );
                outMesh.addTriangle( v4, v6, v7 );
                // Left  (-X face)
                outMesh.addTriangle( v0, v4, v7 );
                outMesh.addTriangle( v0, v7, v3 );
                // Right (+X face)
                outMesh.addTriangle( v1, v2, v6 );
                outMesh.addTriangle( v1, v6, v5 );
                // Bottom (-Y face)
                outMesh.addTriangle( v0, v1, v5 );
                outMesh.addTriangle( v0, v5, v4 );
                // Top   (+Y face)
                outMesh.addTriangle( v3, v7, v6 );
                outMesh.addTriangle( v3, v6, v2 );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<Properties> BoxRule::getProperties() const
        {
            try
            {
                auto props = ModelRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "BoxRule::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                props->setProperty( "sizeX", size.x );
                props->setProperty( "sizeY", size.y );
                props->setProperty( "sizeZ", size.z );
                props->setProperty( "offsetX", offset.x );
                props->setProperty( "offsetY", offset.y );
                props->setProperty( "offsetZ", offset.z );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void BoxRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "BoxRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto sx = size.x;
                auto sy = size.y;
                auto sz = size.z;
                auto ox = offset.x;
                auto oy = offset.y;
                auto oz = offset.z;

                properties->getPropertyValue( "sizeX", sx );
                properties->getPropertyValue( "sizeY", sy );
                properties->getPropertyValue( "sizeZ", sz );
                properties->getPropertyValue( "offsetX", ox );
                properties->getPropertyValue( "offsetY", oy );
                properties->getPropertyValue( "offsetZ", oz );

                size = Vector3F{ std::max( kMinDimension, sx ), std::max( kMinDimension, sy ),
                                 std::max( kMinDimension, sz ) };
                offset = Vector3F{ ox, oy, oz };
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // CylinderRule
        // ================================================================

        CylinderRule::CylinderRule() = default;

        CylinderRule::CylinderRule( float radius_, float height_, int segments_,
                                    const Vector3F &offset_ ) :
            radius( radius_ ),
            height( height_ ),
            segments( std::max( 3, segments_ ) ),
            offset( offset_ )
        {
        }

        void CylinderRule::build( ProceduralMesh &outMesh ) const
        {
            try
            {
                const float r = std::max( kMinDimension, radius );
                const float h = std::max( kMinDimension, height );
                const int s = std::max( 3, segments );

                const float halfH = h * 0.5f;

                // Reserve index arrays up front to avoid partial-build on allocation
                // failure.
                std::vector<uint32_t> top;
                std::vector<uint32_t> bottom;
                top.reserve( static_cast<size_t>( s ) );
                bottom.reserve( static_cast<size_t>( s ) );

                const uint32_t topCenter = outMesh.addVertex( offset + Vector3F{ 0.0f, halfH, 0.0f } );
                const uint32_t bottomCenter =
                    outMesh.addVertex( offset + Vector3F{ 0.0f, -halfH, 0.0f } );

                const uint32_t sentinel = std::numeric_limits<uint32_t>::max();
                if( topCenter == sentinel || bottomCenter == sentinel )
                {
                    WP_LOG_ERROR( "CylinderRule::build - vertex buffer overflow on cap centres." );
                    return;
                }

                for( int i = 0; i < s; ++i )
                {
                    const float a = ( static_cast<float>( i ) / static_cast<float>( s ) ) * k2Pi;
                    const float x = std::cos( a ) * r;
                    const float z = std::sin( a ) * r;

                    const uint32_t t = outMesh.addVertex( offset + Vector3F{ x, halfH, z } );
                    const uint32_t b = outMesh.addVertex( offset + Vector3F{ x, -halfH, z } );

                    if( t == sentinel || b == sentinel )
                    {
                        WP_LOG_ERROR( "CylinderRule::build - vertex buffer overflow at segment " +
                                      std::to_string( i ) + "." );
                        return;
                    }

                    top.push_back( t );
                    bottom.push_back( b );
                }

                for( int i = 0; i < s; ++i )
                {
                    const int n = ( i + 1 ) % s;

                    // Side quad (two triangles).
                    outMesh.addTriangle( bottom[i], bottom[n], top[n] );
                    outMesh.addTriangle( bottom[i], top[n], top[i] );

                    // Top and bottom caps.
                    outMesh.addTriangle( topCenter, top[i], top[n] );
                    outMesh.addTriangle( bottomCenter, bottom[n], bottom[i] );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<Properties> CylinderRule::getProperties() const
        {
            try
            {
                auto props = ModelRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "CylinderRule::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                props->setProperty( "radius", radius );
                props->setProperty( "height", height );
                props->setProperty( "segments", segments );
                props->setProperty( "offsetX", offset.x );
                props->setProperty( "offsetY", offset.y );
                props->setProperty( "offsetZ", offset.z );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CylinderRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CylinderRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto r = radius;
                auto h = height;
                auto s = segments;
                auto ox = offset.x;
                auto oy = offset.y;
                auto oz = offset.z;

                properties->getPropertyValue( "radius", r );
                properties->getPropertyValue( "height", h );
                properties->getPropertyValue( "segments", s );
                properties->getPropertyValue( "offsetX", ox );
                properties->getPropertyValue( "offsetY", oy );
                properties->getPropertyValue( "offsetZ", oz );

                radius = std::max( kMinDimension, r );
                height = std::max( kMinDimension, h );
                segments = std::max( 3, s );
                offset = Vector3F{ ox, oy, oz };
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // ArrayRule
        // ================================================================

        ArrayRule::ArrayRule( std::shared_ptr<ModelRule> child_, int count_, const Vector3F &step_ ) :
            child( std::move( child_ ) ),
            count( std::max( 1, count_ ) ),
            step( step_ )
        {
        }

        void ArrayRule::build( ProceduralMesh &outMesh ) const
        {
            if( !child )
            {
                WP_LOG_ERROR( "ArrayRule::build - child rule is null." );
                return;
            }

            try
            {
                // Build the child mesh once and replicate it.
                ProceduralMesh childMesh;
                child->build( childMesh );

                if( !childMesh.hasGeometry() )
                {
                    WP_LOG_ERROR( "ArrayRule::build - child produced no geometry, skipping array." );
                    return;
                }

                for( int i = 0; i < count; ++i )
                {
                    outMesh.append( childMesh, step * static_cast<float>( i ) );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<Properties> ArrayRule::getProperties() const
        {
            try
            {
                auto props = ModelRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "ArrayRule::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                props->setProperty( "count", count );
                props->setProperty( "stepX", step.x );
                props->setProperty( "stepY", step.y );
                props->setProperty( "stepZ", step.z );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void ArrayRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "ArrayRule::setProperties - null properties provided." );
                return;
            }

            try
            {
                auto c = count;
                auto sx = step.x;
                auto sy = step.y;
                auto sz = step.z;

                properties->getPropertyValue( "count", c );
                properties->getPropertyValue( "stepX", sx );
                properties->getPropertyValue( "stepY", sy );
                properties->getPropertyValue( "stepZ", sz );

                count = std::max( 1, c );
                step = Vector3F{ sx, sy, sz };
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // CompositeRule
        // ================================================================

        void CompositeRule::add( std::shared_ptr<ModelRule> rule )
        {
            if( !rule )
            {
                WP_LOG_ERROR( "CompositeRule::add - null rule rejected." );
                return;
            }

            children.push_back( std::move( rule ) );
        }

        void CompositeRule::build( ProceduralMesh &outMesh ) const
        {
            for( const auto &rule : children )
            {
                if( !rule )
                {
                    WP_LOG_ERROR( "CompositeRule::build - null child rule skipped." );
                    continue;
                }

                try
                {
                    rule->build( outMesh );
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                    // Continue with remaining children.
                }
            }
        }

        SmartPtr<Properties> CompositeRule::getProperties() const
        {
            try
            {
                auto props = ModelRule::getProperties();
                if( !props )
                {
                    WP_LOG_ERROR( "CompositeRule::getProperties - failed to allocate Properties." );
                    return nullptr;
                }

                props->setProperty( "childCount", static_cast<int>( children.size() ) );

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CompositeRule::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "CompositeRule::setProperties - null properties provided." );
                return;
            }

            // childCount is structural (children must be added through add());
            // intentionally read-only from the property panel.
        }

        // ================================================================
        // ProceduralModelComponent
        // ================================================================

        SmartPtr<Properties> ProceduralModelComponent::getProperties() const
        {
            try
            {
                auto props = workphone::make_ptr<Properties>();
                if( !props )
                {
                    WP_LOG_ERROR( "ProceduralModelComponent::getProperties - allocation failed." );
                    return nullptr;
                }

                props->setProperty( "dirty", dirty );
                props->setProperty( "vertexCount", static_cast<int>( bakedMesh.vertices.size() ) );
                props->setProperty( "indexCount", static_cast<int>( bakedMesh.indices.size() ) );
                props->setProperty( "hasRootRule", ( rootRule != nullptr ) );

                // Let the root rule expose its own parameters as a child group.
                if( rootRule )
                {
                    auto ruleProps = rootRule->getProperties();
                    if( ruleProps )
                    {
                        ruleProps->setName( "rootRule" );
                        props->addChild( ruleProps );
                    }
                }

                return props;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void ProceduralModelComponent::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "ProceduralModelComponent::setProperties - null properties provided." );
                return;
            }

            try
            {
                // Allow the editor to force a rebuild by toggling the dirty flag.
                properties->getPropertyValue( "dirty", dirty );

                // Forward root-rule parameters to the rule itself.
                if( rootRule )
                {
                    auto ruleProps = properties->getChild( "rootRule" );
                    if( ruleProps )
                    {
                        rootRule->setProperties( ruleProps );
                        dirty = true;  // Parameters changed; mesh is stale.
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        // ================================================================
        // ProceduralModelSystem
        // ================================================================

        void ProceduralModelSystem::bake( ProceduralModelComponent &component )
        {
            if( !component.dirty )
            {
                return;  // Nothing to do.
            }

            if( !component.rootRule )
            {
                WP_LOG_ERROR( "ProceduralModelSystem::bake - rootRule is null, cannot bake." );
                component.dirty = false;  // Nothing will ever build; mark clean.
                return;
            }

            try
            {
                // Build into a staging mesh so the previous result is preserved on
                // failure.
                ProceduralMesh staged;
                component.rootRule->build( staged );

                if( staged.hasGeometry() )
                {
                    staged.computeNormals();
                    component.bakedMesh = std::move( staged );
                }
                else
                {
                    WP_LOG_ERROR( "ProceduralModelSystem::bake - rule tree produced no geometry." );
                }

                component.dirty = false;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                // Leave component.dirty = true so a retry is possible.
            }
        }

    }  // namespace procedural
}  // namespace workphone
