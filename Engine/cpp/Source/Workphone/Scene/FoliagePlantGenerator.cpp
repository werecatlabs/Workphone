#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/FoliagePlantGenerator.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f;
        float unit( u64 seed )
        {
            return float( render::foliageStableHash( seed ) >> 40 ) / 16777216.f;
        }
        void quad( render::FoliageMeshData &m, Vector3F a, Vector3F b, Vector3F c, Vector3F d )
        {
            auto normal = ( b - a ).crossProduct( c - a );
            normal.normalise();
            const auto base = u32( m.positions.size() );
            for( auto p : { a, b, c, d } )
                m.positions.push_back( p );
            for( int i = 0; i < 4; ++i )
                m.normals.push_back( normal );
            for( auto uv : { Vector2F( 0, 1 ), Vector2F( 1, 1 ), Vector2F( 1, 0 ), Vector2F( 0, 0 ) } )
                m.uvs.push_back( uv );
            for( auto index : { base, base + 1, base + 2, base, base + 2, base + 3 } )
                m.indices.push_back( index );
        }
        void cylinder( render::FoliageMeshData &mesh, Vector3F start, Vector3F end, float radius,
                       u32 sides )
        {
            auto axis = end - start;
            axis.normalise();
            auto right = axis.crossProduct( std::abs( axis.y ) > .9f ? Vector3F( 1, 0, 0 )
                                                                     : Vector3F( 0, 1, 0 ) );
            right.normalise();
            auto front = right.crossProduct( axis );
            for( u32 side = 0; side < sides; ++side )
            {
                auto a = ( right * std::cos( 2 * pi * side / sides ) +
                           front * std::sin( 2 * pi * side / sides ) ) *
                         radius;
                auto b = ( right * std::cos( 2 * pi * ( side + 1 ) / sides ) +
                           front * std::sin( 2 * pi * ( side + 1 ) / sides ) ) *
                         radius;
                quad( mesh, start + a, start + b, end + b * .6f, end + a * .6f );
            }
        }
        void bounds( render::FoliageSpecies &species )
        {
            auto minimum = Vector3F( std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max(),
                                     std::numeric_limits<f32>::max() );
            auto maximum = -minimum;
            for( const auto &mesh : species.lods )
                for( auto p : mesh.positions )
                {
                    minimum.x = std::min( minimum.x, p.x );
                    minimum.y = std::min( minimum.y, p.y );
                    minimum.z = std::min( minimum.z, p.z );
                    maximum.x = std::max( maximum.x, p.x );
                    maximum.y = std::max( maximum.y, p.y );
                    maximum.z = std::max( maximum.z, p.z );
                }
            species.boundsMinimum = minimum;
            species.boundsMaximum = maximum;
        }
    }  // namespace
    bool generateFoliagePlant( const FoliagePlantRecipe &r, render::FoliageSpecies &output,
                               String &error )
    {
        try
        {
            if( r.speciesId.empty() || u32( r.kind ) > 3 || !std::isfinite( r.height ) ||
                r.height < .05f || r.height > 100 || !std::isfinite( r.radius ) || r.radius < .01f ||
                r.radius > 50 || !std::isfinite( r.trunkRadius ) || r.trunkRadius < .005f ||
                r.trunkRadius > r.radius || r.branches < 1 || r.branches > 128 )
                throw std::runtime_error(
                    "Plant recipe requires finite dimensions, 1..128 branches and a species ID." );
            render::FoliageSpecies species;
            species.id = r.speciesId;
            species.kind = r.kind;
            species.wind = r.wind;
            species.materials = { r.bark, r.leaves };
            const bool grass =
                r.kind == render::FoliageKind::Grass || r.kind == render::FoliageKind::GroundCover;
            for( u32 lod = 0; lod < 3; ++lod )
            {
                auto &mesh = species.lods[lod];
                const u32 branches = std::max( 1u, r.branches >> lod ), sides = 8 - lod * 2;
                Array<Vector3F> tips;
                if( !grass )
                {
                    cylinder( mesh, {}, { 0, r.height, 0 }, r.trunkRadius, sides );
                    for( u32 i = 0; i < branches; ++i )
                    {
                        const auto yaw = unit( u64( r.seed ) * 31 + i * ( 1u << lod ) ) * 2 * pi;
                        const auto y =
                            r.height * ( .25f + .6f * float( i + 1 ) / float( branches + 1 ) );
                        auto tip = Vector3F( std::cos( yaw ) * r.radius,
                                             std::min( r.height, y + r.height * .1f ),
                                             std::sin( yaw ) * r.radius );
                        cylinder( mesh, { 0, y, 0 }, tip, r.trunkRadius * .4f,
                                  std::max( 3u, sides / 2 ) );
                        tips.push_back( tip );
                    }
                    species.lods[lod].sections.push_back( { 0, u32( mesh.indices.size() ), 0 } );
                }
                const auto leafStart = u32( mesh.indices.size() );
                const auto cards = grass ? branches : std::max( 2u, branches * ( 3 - lod ) );
                for( u32 i = 0; i < cards; ++i )
                {
                    const auto yaw = unit( u64( r.seed ) * 17 + i * ( 1u << lod ) ) * 2 * pi;
                    auto right = Vector3F( std::cos( yaw ), 0, std::sin( yaw ) );
                    const float halfWidth = grass ? r.radius / std::sqrt( float( branches ) )
                                                  : r.radius * .35f * ( 1 + .3f * lod );
                    auto centre = grass ? Vector3F( std::cos( yaw ) * r.radius * unit( i + r.seed ), 0,
                                                    std::sin( yaw ) * r.radius * unit( i + r.seed ) )
                                        : tips[i % tips.size()];
                    const float cardHeight =
                        grass ? r.height * ( .7f + .3f * unit( i + 71 * r.seed ) ) : r.height * .3f;
                    const auto up = Vector3F( 0, cardHeight, 0 );
                    // Real textured cards, generated independently at each LOD. The stable seed
                    // controls the near/mid clump; UVs cover the configured cutout atlas slot.
                    quad( mesh, centre - right * halfWidth, centre + right * halfWidth,
                          centre + up + right * halfWidth * .5f, centre + up - right * halfWidth * .5f );
                }
                mesh.sections.push_back( { leafStart, u32( mesh.indices.size() ) - leafStart, 1 } );
            }
            if( grass )
            {
                species.densityFractions = { 1, .5f, .2f };
                species.maximumDistance = 80;
                species.shadowDistance = 20;
            }
            bounds( species );
            if( !render::validateFoliageSpecies( species, error ) )
                return false;
            output = std::move( species );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool importFoliageMesh( SmartPtr<IMesh> mesh, const String &id,
                            const Array<render::FoliageMaterial> &materials,
                            render::FoliageSpecies &output, String &error )
    {
        try
        {
            if( !mesh || materials.empty() )
                throw std::runtime_error(
                    "Foliage import requires CPU mesh buffers and explicit materials." );
            render::FoliageSpecies species;
            species.id = id;
            species.materials = materials;
            auto &m = species.lods[0];
            u32 sectionIndex = 0;
            for( auto section : mesh->getSubMeshes() )
            {
                if( !section || section->getUseSharedVertices() )
                    throw std::runtime_error(
                        "Foliage import requires resolved section-local CPU vertices." );
                auto vb = section->getVertexBuffer();
                auto ib = section->getIndexBuffer();
                if( !vb || !ib || !ib->getIndexData() || !vb->getVertexDeclaration() ||
                    ib->getNumIndices() % 3 )
                    throw std::runtime_error( "Foliage import requires indexed CPU triangles." );
                auto declaration = vb->getVertexDeclaration();
                auto position =
                    declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                auto uv =
                    declaration->findElementBySemantic( VertexElementSemantic::VES_TEXTURE_COORDINATES );
                if( !position || !normal || !uv ||
                    position->getType() != VertexElementType::VET_FLOAT3 ||
                    normal->getType() != VertexElementType::VET_FLOAT3 ||
                    ( uv->getType() != VertexElementType::VET_FLOAT2 &&
                      uv->getType() != VertexElementType::VET_FLOAT3 ) )
                    throw std::runtime_error(
                        "Foliage import requires float position/normal/UV attributes." );
                if( m.positions.size() + vb->getNumVertices() > render::foliageMaximumMeshVertices ||
                    m.indices.size() + ib->getNumIndices() > render::foliageMaximumMeshIndices ||
                    sectionIndex >= materials.size() )
                    throw std::runtime_error( "Foliage import exceeds geometry/material bounds." );
                auto read = [&]( SmartPtr<IVertexElement> e, u32 vertex, float *out, u32 count ) {
                    auto stride = declaration->getSize( e->getSource() );
                    auto data = static_cast<const u8 *>( vb->getVertexData( e->getSource() ) );
                    if( !data || e->getOffset() + count * sizeof( float ) > stride )
                        throw std::runtime_error( "Invalid foliage CPU vertex stream." );
                    std::memcpy( out, data + size_t( vertex ) * stride + e->getOffset(),
                                 count * sizeof( float ) );
                };
                const auto base = u32( m.positions.size() );
                for( u32 v = 0; v < vb->getNumVertices(); ++v )
                {
                    float p[3], n[3], t[2];
                    read( position, v, p, 3 );
                    read( normal, v, n, 3 );
                    read( uv, v, t, 2 );
                    m.positions.push_back( { p[0], p[1], p[2] } );
                    m.normals.push_back( { n[0], n[1], n[2] } );
                    m.uvs.push_back( { t[0], t[1] } );
                }
                m.sections.push_back( { u32( m.indices.size() ), ib->getNumIndices(), sectionIndex++ } );
                for( u32 i = 0; i < ib->getNumIndices(); ++i )
                {
                    auto index = ib->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                     ? static_cast<const u16 *>( ib->getIndexData() )[i]
                                     : static_cast<const u32 *>( ib->getIndexData() )[i];
                    if( index >= vb->getNumVertices() )
                        throw std::runtime_error( "Foliage mesh index is outside its vertex buffer." );
                    m.indices.push_back( base + index );
                }
            }
            // Imported assets preserve topology at all distances until authored LODs replace
            // these slots. Native plant generation above produces reduced independent meshes.
            species.lods[1] = m;
            species.lods[2] = m;
            bounds( species );
            if( !render::validateFoliageSpecies( species, error ) )
                return false;
            output = std::move( species );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::scene
