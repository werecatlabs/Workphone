#pragma once

#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Components/ProceduralSceneGeneration.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace workphone::scene::race
{
    enum class CityBuildingPart { Wall, Window, Entrance, Corner, Roof };

    class CityBuildingKit
    {
    public:
        CityBuildingKit( SceneAssets &assets, size_t buildingCount )
            : m_assets( assets ), m_buildingCount( std::max<size_t>( buildingCount, 1 ) )
        {
            for( size_t role = 0; role < m_modules.size(); ++role )
            {
                const auto &file = m_assets.cityBuildingMeshes[role];
                if( file.empty() ) continue;
                try
                {
                    m_modules[role] = read( file, static_cast<CityBuildingPart>( role ) );
                }
                catch( const std::bad_alloc & )
                {
                    warn( role, "mesh exceeded the source memory budget" );
                }
                catch( const std::exception &failure )
                {
                    warn( role, failure.what() );
                }
            }
        }

        void beginBuilding()
        {
            m_buildingStart = m_assets.cityBuildingModulePlacements;
        }

        template <class Geometry>
        bool append( CityBuildingPart part, Geometry &geometry, const Vector3F &centre,
                     const Vector3F &size, float yaw )
        {
            const auto role = static_cast<size_t>( part );
            const auto &module = m_modules[role];
            if( module.indices.empty() ) return false;
            if( size.x <= 0 || size.y <= 0 || size.z <= 0 ) return false;

            const auto placements = m_assets.cityBuildingModulePlacements[role];
            const auto limit = std::min( 160000 / module.positions.size(),
                                         200000 / ( module.indices.size() / 3 ) );
            const auto perBuilding = std::max<size_t>( limit / m_buildingCount, 1 );
            if( placements >= limit || placements - m_buildingStart[role] >= perBuilding )
            {
                warn( role, "placement budget reached" );
                return false;
            }

            const Vector3F scale( size.x / module.size.x,
                                  module.size.y > .001f ? size.y / module.size.y : 1.f,
                                  module.size.z > .001f ? size.z / module.size.z : 1.f );
            auto anchor = centre;
            if( part == CityBuildingPart::Roof && module.size.y <= .001f )
                anchor.y -= size.y * .5f;
            const float c = std::cos( yaw ), s = std::sin( yaw );
            const auto rotate = [=]( const Vector3F &p ) {
                return Vector3F( c * p.x + s * p.z, p.y, c * p.z - s * p.x );
            };
            const auto base = static_cast<u32>( geometry.positions.size() );
            for( size_t i = 0; i < module.positions.size(); ++i )
            {
                const auto p = module.positions[i] - module.centre;
                const auto n = module.normals[i];
                const auto normal = rotate( { n.x / scale.x, n.y / scale.y, n.z / scale.z } ).normaliseCopy();
                const auto axis = std::abs( normal.y ) < .95f ? Vector3F::unitY() : Vector3F::unitZ();
                const auto tangent = axis.crossProduct( normal ).normaliseCopy();
                geometry.positions.push_back( anchor + rotate( { p.x * scale.x, p.y * scale.y, p.z * scale.z } ) );
                geometry.normals.push_back( normal );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                geometry.uv.push_back( module.uv[i] );
            }
            for( auto i : module.indices ) geometry.indices.push_back( base + i );
            ++m_assets.cityBuildingModulePlacements[role];
            ++m_assets.cityBuildingModuleCount;
            return true;
        }

        void finish()
        {
            for( size_t role = 0; role < m_modules.size(); ++role )
                if( !m_modules[role].indices.empty() && m_assets.cityBuildingModulePlacements[role] == 0 )
                    warn( role, "no placements at the selected quality or layout" );
        }

    private:
        struct Module
        {
            SourceBudgetVector<Vector3F> positions, normals, uv;
            SourceBudgetVector<u32> indices;
            Vector3F centre{}, size{};
        };

        static SmartPtr<IMesh> load( const String &file )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !app ) return nullptr;
            if( file.size() == 36 && Path::getFileExtension( file ).empty() )
            {
                auto database = app->getResourceDatabase();
                if( database )
                {
                    const auto id = UUID::from_string( std::string( file.data(), file.size() ) );
                    if( !id.is_nil() )
                    {
                        auto resource = dynamic_pointer_cast<IMeshResource>( database->loadResource( id ) );
                        return resource ? resource->getMesh() : nullptr;
                    }
                }
            }

            if( !ApplicationUtil::isSupportedMesh( file ) ) return nullptr;
            auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            auto files = app->getFileSystem();
            if( !manager || !files ) return nullptr;
            auto path = StringUtil::cleanupPath( file );
            if( !files->isExistingFile( path ) && !app->getProjectPath().empty() )
                path = Path::getAbsolutePath( app->getProjectPath(), path );
            return files->isExistingFile( path ) ? manager->loadMesh( path ) : nullptr;
        }

        static Module read( const String &file, CityBuildingPart part )
        {
            auto mesh = load( file );
            if( !mesh || mesh->hasSkeleton() || !mesh->getBoneAssignments().empty() )
                throw std::runtime_error( "expected a readable static mesh" );

            Module module;
            const auto finite = []( const Vector3F &v ) {
                return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
            };
            for( auto submesh : mesh->getSubMeshes() )
            {
                if( !submesh || !submesh->getBoneAssignments().empty() )
                    throw std::runtime_error( "expected static triangle submeshes" );
                auto vertices = submesh->getVertexBuffer();
                auto indices = submesh->getIndexBuffer();
                if( !vertices || !indices || !indices->getIndexData() || indices->getNumIndices() % 3 != 0 )
                    throw std::runtime_error( "expected indexed triangles" );
                const auto count = vertices->getNumVertices();
                if( module.positions.size() + count > 2048 ||
                    module.indices.size() + indices->getNumIndices() > 4096 * 3 )
                    throw std::runtime_error( "module exceeds 2048 vertices or 4096 triangles" );

                auto declaration = vertices->getVertexDeclaration();
                if( !declaration ) throw std::runtime_error( "vertex layout missing" );
                const auto stream = [&]( VertexElementSemantic semantic, VertexElementType type ) {
                    auto element = declaration->findElementBySemantic( semantic );
                    const auto stride = element ? declaration->getSize( element->getSource() ) : 0u;
                    const auto bytes = type == VertexElementType::VET_FLOAT3 ? sizeof( f32 ) * 3 : sizeof( f32 ) * 2;
                    const auto data = element && element->getType() == type && element->getOffset() < stride &&
                        bytes <= stride - element->getOffset()
                        ? static_cast<const u8 *>( vertices->getVertexData( element->getSource() ) ) : nullptr;
                    return std::pair{ data ? data + element->getOffset() : nullptr, stride };
                };
                const auto [positions, positionStride] = stream( VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3 );
                const auto [normals, normalStride] = stream( VertexElementSemantic::VES_NORMAL, VertexElementType::VET_FLOAT3 );
                const auto [uv, uvStride] = stream( VertexElementSemantic::VES_TEXTURE_COORDINATES, VertexElementType::VET_FLOAT2 );
                if( !positions ) throw std::runtime_error( "vertex positions missing" );
                const auto base = static_cast<u32>( module.positions.size() );
                for( u32 i = 0; i < count; ++i )
                {
                    const auto vector = [i]( const u8 *data, u32 stride, size_t components ) {
                        std::array<f32, 3> value{};
                        if( data ) std::memcpy( value.data(), data + size_t( i ) * stride, components * sizeof( f32 ) );
                        return Vector3F( value[0], value[1], value[2] );
                    };
                    const auto p = vector( positions, positionStride, 3 );
                    const auto n = vector( normals, normalStride, 3 );
                    const auto tex = vector( uv, uvStride, 2 );
                    if( !finite( p ) || !finite( n ) || !finite( tex ) ||
                        std::max( { std::abs( p.x ), std::abs( p.y ), std::abs( p.z ) } ) > 1000000.f )
                        throw std::runtime_error( "non-finite vertex data" );
                    module.positions.push_back( p );
                    module.normals.push_back( n );
                    module.uv.push_back( tex );
                }
                for( auto i : MeshUtil::getIndices( submesh ) )
                {
                    if( i >= count ) throw std::runtime_error( "triangle index out of bounds" );
                    module.indices.push_back( base + i );
                }
            }
            if( module.positions.empty() || module.indices.empty() )
                throw std::runtime_error( "mesh has no triangles" );
            auto lo = module.positions.front(), hi = lo;
            for( auto p : module.positions )
            {
                lo = { std::min( lo.x, p.x ), std::min( lo.y, p.y ), std::min( lo.z, p.z ) };
                hi = { std::max( hi.x, p.x ), std::max( hi.y, p.y ), std::max( hi.z, p.z ) };
            }
            module.size = hi - lo;
            module.centre = lo + module.size * .5f;
            const bool roof = part == CityBuildingPart::Roof;
            if( !finite( module.size ) || module.size.x <= .001f ||
                ( roof ? module.size.z : module.size.y ) <= .001f )
                throw std::runtime_error( roof ? "roof mesh needs nonzero width and depth"
                                               : "mesh needs nonzero width and height" );
            SourceBudgetVector<Vector3F> faceNormals( module.positions.size(), Vector3F::zero() );
            bool hasArea = false;
            for( size_t i = 0; i < module.indices.size(); i += 3 )
            {
                const auto a = module.indices[i], b = module.indices[i + 1], c = module.indices[i + 2];
                const auto n = ( module.positions[b] - module.positions[a] ).crossProduct( module.positions[c] - module.positions[a] );
                if( !finite( n ) ) throw std::runtime_error( "invalid triangle size" );
                hasArea |= n.lengthSquared() > 1e-16f;
                for( auto index : { a, b, c } ) faceNormals[index] += n;
            }
            if( !hasArea ) throw std::runtime_error( "mesh contains only degenerate triangles" );
            for( size_t i = 0; i < module.normals.size(); ++i )
            {
                auto &normal = module.normals[i];
                if( normal.lengthSquared() < 1e-12f ) normal = faceNormals[i];
                normal = normal.lengthSquared() < 1e-12f ? Vector3F::unitZ() : normal.normaliseCopy();
            }
            return module;
        }

        void warn( size_t role, const char *message )
        {
            if( m_warned[role] ) return;
            m_warned[role] = true;
            constexpr std::array names{ "Wall", "Window", "Entrance", "Corner", "Roof" };
            if( !m_assets.cityBuildingKitDiagnostic.empty() ) m_assets.cityBuildingKitDiagnostic += " | ";
            m_assets.cityBuildingKitDiagnostic += String( names[role] ) + ": " + message;
        }

        SceneAssets &m_assets;
        size_t m_buildingCount;
        std::array<Module, 5> m_modules;
        std::array<u32, 5> m_buildingStart{};
        std::array<bool, 5> m_warned{};
    };
}
