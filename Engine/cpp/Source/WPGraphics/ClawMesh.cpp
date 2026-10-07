#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_mesh_reader.h"
#include "workphone_graphics_object.h"
#include <limits>
#include <cstring>
#include <memory>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMesh, GraphicsMesh );
        namespace
        {
            // Convert the registered CPU resource without creating a temporary mesh file.
            wp_graphics_mesh *createProceduralNativeMesh( SmartPtr<workphone::IMesh> mesh )
            {
                if( !mesh )
                    return nullptr;
                Array<wp_graphics_mesh_vertex_pnt> vertices;
                Array<wp_u32> indices;
                Array<Pair<wp_u32, wp_u32>> ranges;
                Array<wp_u32> materials;
                for( const auto &section : mesh->getSubMeshes() )
                {
                    if( !section || section->getUseSharedVertices() )
                        return nullptr;
                    auto vb = section->getVertexBuffer();
                    auto ib = section->getIndexBuffer();
                    if( !vb || !ib )
                        return nullptr;
                    if( !vb->getNumVertices() || !ib->getNumIndices() )
                        continue;
                    auto declaration = vb->getVertexDeclaration();
                    if( !declaration )
                        return nullptr;
                    auto position =
                        declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    auto normal =
                        declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    auto uv = declaration->findElementBySemantic(
                        VertexElementSemantic::VES_TEXTURE_COORDINATES );
                    if( !position || !normal || !uv ||
                        position->getType() != VertexElementType::VET_FLOAT3 ||
                        normal->getType() != VertexElementType::VET_FLOAT3 ||
                        ( uv->getType() != VertexElementType::VET_FLOAT2 &&
                          uv->getType() != VertexElementType::VET_FLOAT3 ) )
                        return nullptr;
                    const auto limit = std::numeric_limits<wp_u32>::max();
                    if( vertices.size() + vb->getNumVertices() > limit ||
                        indices.size() + ib->getNumIndices() > limit || ib->getNumIndices() % 3 != 0 ||
                        !ib->getIndexData() )
                        return nullptr;
                    const auto base = static_cast<wp_u32>( vertices.size() );
                    auto read = [&]( SmartPtr<IVertexElement> element, u32 vertex, float *out,
                                     u32 count ) {
                        const auto stride = declaration->getSize( element->getSource() );
                        const auto data =
                            static_cast<const u8 *>( vb->getVertexData( element->getSource() ) );
                        if( !data || element->getOffset() + count * sizeof( float ) > stride )
                            return false;
                        std::memcpy(
                            out, data + static_cast<size_t>( vertex ) * stride + element->getOffset(),
                            count * sizeof( float ) );
                        return true;
                    };
                    for( u32 i = 0; i < vb->getNumVertices(); ++i )
                    {
                        float p[3], n[3], t[2];
                        if( !read( position, i, p, 3 ) || !read( normal, i, n, 3 ) ||
                            !read( uv, i, t, 2 ) )
                            return nullptr;
                        wp_graphics_mesh_vertex_pnt vertex{};
                        vertex.position = { p[0], p[1], p[2] };
                        vertex.normal = { n[0], n[1], n[2] };
                        vertex.uv = { t[0], t[1] };
                        vertices.push_back( vertex );
                    }
                    ranges.emplace_back( static_cast<wp_u32>( indices.size() ), ib->getNumIndices() );
                    materials.push_back(
                        static_cast<wp_u32>( StringUtil::getHash( section->getMaterialName() ) ) );
                    for( u32 i = 0; i < ib->getNumIndices(); ++i )
                    {
                        const auto index = ib->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                               ? static_cast<const u16 *>( ib->getIndexData() )[i]
                                               : static_cast<const u32 *>( ib->getIndexData() )[i];
                        if( index >= vb->getNumVertices() )
                            return nullptr;
                        indices.push_back( base + index );
                    }
                }
                if( vertices.empty() || indices.empty() )
                    return nullptr;
                std::unique_ptr<wp_graphics_mesh, decltype( &wp_graphics_mesh_destroy )> result(
                    wp_graphics_mesh_create(), &wp_graphics_mesh_destroy );
                if( !result ||
                    !wp_graphics_mesh_set_vertices( result.get(), WORKPHONE_VERTEX_FORMAT_PNT,
                                                    vertices.data(),
                                                    static_cast<wp_u32>( vertices.size() ) ) ||
                    !wp_graphics_mesh_set_indices_u32( result.get(), indices.data(),
                                                       static_cast<wp_u32>( indices.size() ) ) )
                    return nullptr;
                for( size_t i = 0; i < ranges.size(); ++i )
                    if( wp_graphics_mesh_add_submesh( result.get(), ranges[i].first, ranges[i].second,
                                                      materials[i] ) < 0 )
                        return nullptr;
                wp_graphics_mesh_compute_aabb( result.get() );
                return result.release();
            }
        }  // namespace

        ClawMesh::ClawMesh()
        {
        }

        ClawMesh::~ClawMesh()
        {
            if( m_mesh )
            {
                ClawRendererDX11::forgetMesh( m_mesh );
                wp_graphics_mesh_destroy( m_mesh );
            }
        }

        void ClawMesh::load( SmartPtr<ISharedObject> data )
        {
            // Meshes are initially queued by ClawScene before MeshRenderer supplies
            // their resource path. Permit the later queued load to populate an
            // already-created, but still empty, native mesh.
            if( m_mesh && wp_graphics_mesh_get_vertex_count( m_mesh ) > 0 )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            if( !m_mesh )
                m_mesh = wp_graphics_mesh_create();
            if( m_renderObject )
                wp_graphics_object_set_mesh( m_renderObject, m_mesh );

            GraphicsMesh::load( data );

            if( StringUtil::isNullOrEmpty( m_meshName.str() ) )
            {
                setLoadingState( LoadingState::Loaded );
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            wp_graphics_mesh *loadedMesh = nullptr;
            if( m_meshName.str().rfind( "__procedural/", 0 ) == 0 )
            {
                auto manager = applicationManager->getMeshManager();
                auto resource = manager ? workphone::dynamic_pointer_cast<IMeshResource>(
                                              manager->loadFromFile( m_meshName.str() ) )
                                        : nullptr;
                loadedMesh = resource ? createProceduralNativeMesh( resource->getMesh() ) : nullptr;
            }
            else
            {
                auto fileSystem = applicationManager->getFileSystemPtr();

                auto stream = fileSystem->open( m_meshName, true, true, false, false, false );
                if( !stream && fileSystem )
                {
                    stream = fileSystem->open( m_meshName, true, true, false, true, true );
                }

                const auto streamSize = stream ? stream->size() : 0;
                if( !stream || streamSize == 0 ||
                    streamSize > static_cast<size_Num>( std::numeric_limits<wp_u32>::max() ) )
                {
                    WP_LOG_ERROR( "ClawMesh::load: could not open mesh: " + m_meshName.str() );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                Array<u8> bytes( streamSize );
                if( stream->read( bytes.data(), streamSize ) != streamSize )
                {
                    WP_LOG_ERROR( "ClawMesh::load: incomplete mesh read: " + m_meshName.str() );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                loadedMesh = wp_graphics_mesh_read_from_buffer( bytes.data(),
                                                                static_cast<wp_u32>( bytes.size() ) );
            }

            if( !loadedMesh || wp_graphics_mesh_get_vertex_count( loadedMesh ) == 0 )
            {
                if( loadedMesh )
                {
                    wp_graphics_mesh_destroy( loadedMesh );
                }

                WP_LOG_ERROR( "ClawMesh::load: unsupported or invalid mesh: " + m_meshName.str() );
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            if( m_mesh )
            {
                ClawRendererDX11::forgetMesh( m_mesh );
                wp_graphics_mesh_destroy( m_mesh );
            }

            m_mesh = loadedMesh;

            if( m_renderObject )
            {
                wp_graphics_object_set_submit_data( m_renderObject, this );
                wp_graphics_object_set_mesh( m_renderObject, m_mesh );
                wp_graphics_object_set_local_aabb( m_renderObject,
                                                   wp_graphics_mesh_get_local_aabb( m_mesh ) );
            }

            setLoadingState( LoadingState::Loaded );
        }

        void ClawMesh::reload( SmartPtr<ISharedObject> data )
        {
            m_skinVertices.clear();
            m_skinOutput.clear();
            // Keep the scene's native render object attached while replacing its mesh.
            if( m_renderObject )
                wp_graphics_object_set_mesh( m_renderObject, nullptr );
            if( m_mesh )
            {
                ClawRendererDX11::forgetMesh( m_mesh );
                wp_graphics_mesh_destroy( m_mesh );
                m_mesh = nullptr;
            }
            load( data );
        }

        void ClawMesh::unload( SmartPtr<ISharedObject> data )
        {
            m_skinVertices.clear();
            m_skinOutput.clear();
            setLoadingState( LoadingState::Unloading );

            bindNativeRenderObject( nullptr );

            if( m_mesh )
            {
                ClawRendererDX11::forgetMesh( m_mesh );
                wp_graphics_mesh_destroy( m_mesh );
                m_mesh = nullptr;
            }

            GraphicsMesh::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        wp_graphics_mesh *ClawMesh::getNativeMesh() const
        {
            return m_mesh;
        }

        void ClawMesh::bindNativeRenderObject( wp_graphics_object *object )
        {
            if( m_renderObject == object )
            {
                return;
            }

            if( m_renderObject )
            {
                wp_graphics_object_set_submit_data( m_renderObject, nullptr );
                wp_graphics_object_set_mesh( m_renderObject, nullptr );
                wp_graphics_object_set_material( m_renderObject, nullptr );
            }

            m_renderObject = object;

            if( m_renderObject )
            {
                wp_graphics_object_set_submit_data( m_renderObject, this );
                wp_graphics_object_set_mesh( m_renderObject, m_mesh );
                wp_graphics_object_set_visible( m_renderObject, isVisible() ? 1 : 0 );
                wp_graphics_object_set_visibility_flags( m_renderObject, getVisibilityFlags() );
                wp_graphics_object_set_cast_shadows( m_renderObject, getCastShadows() ? 1 : 0 );
                wp_graphics_object_set_receive_shadows( m_renderObject, getReceiveShadows() ? 1 : 0 );

                if( auto material = dynamic_pointer_cast<ClawMaterial>( getMaterial() ) )
                {
                    wp_graphics_object_set_material( m_renderObject, material->getNativeMaterial() );
                }
            }
        }

        wp_graphics_object *ClawMesh::getNativeRenderObject() const
        {
            return m_renderObject;
        }

        AABB3<real_Num> ClawMesh::getLocalAABB() const
        {
            if( m_mesh )
            {
                const auto bounds = wp_graphics_mesh_get_local_aabb( m_mesh );
                return AABB3<real_Num>( bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x,
                                        bounds.max.y, bounds.max.z );
            }

            return {};
        }

        void ClawMesh::setMaterialName( const String &materialName, s32 index )
        {
            GraphicsMesh::setMaterialName( materialName, index );

            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto materialManager = graphicsSystem->getMaterialManager() )
                    {
                        if( auto resource = materialManager->getByName( materialName ) )
                        {
                            if( auto material = dynamic_pointer_cast<IMaterial>( resource ) )
                                setMaterial( material, index );
                        }
                    }
                }
            }
        }

        String ClawMesh::getMaterialName( s32 index ) const
        {
            return GraphicsMesh::getMaterialName( index );
        }

        void ClawMesh::setMaterial( SmartPtr<IMaterial> material, s32 index )
        {
            GraphicsMesh::setMaterial( material, index );
            if( m_renderObject && index <= 0 )
            {
                auto clawMaterial = dynamic_pointer_cast<ClawMaterial>( material );
                wp_graphics_object_set_material(
                    m_renderObject, clawMaterial ? clawMaterial->getNativeMaterial() : nullptr );
            }
        }

        SmartPtr<IMaterial> ClawMesh::getMaterial( s32 index ) const
        {
            return GraphicsMesh::getMaterial( index );
        }

        void ClawMesh::setHardwareAnimationEnabled( bool enabled )
        {
            GraphicsMesh::setHardwareAnimationEnabled( enabled );
        }

        void ClawMesh::checkVertexProcessing()
        {
            GraphicsMesh::checkVertexProcessing();
        }

        SmartPtr<IAnimationController> ClawMesh::getAnimationController()
        {
            return GraphicsMesh::getAnimationController();
        }

        void ClawMesh::setVisible( bool visible )
        {
            m_visible = visible;
            GraphicsMesh::setVisible( visible );
            if( m_renderObject )
            {
                wp_graphics_object_set_visible( m_renderObject, visible ? 1 : 0 );
            }
        }

        bool ClawMesh::isVisible() const
        {
            return m_visible.load();
        }

        void ClawMesh::setCastShadows( bool castShadows )
        {
            m_castShadows = castShadows;
            GraphicsMesh::setCastShadows( castShadows );
            if( m_renderObject )
                wp_graphics_object_set_cast_shadows( m_renderObject, castShadows ? 1 : 0 );
        }

        bool ClawMesh::getCastShadows() const
        {
            return m_castShadows.load();
        }

        void ClawMesh::setReceiveShadows( bool receiveShadows )
        {
            m_receiveShadows = receiveShadows;
            GraphicsMesh::setReceiveShadows( receiveShadows );
            if( m_renderObject )
                wp_graphics_object_set_receive_shadows( m_renderObject, receiveShadows ? 1 : 0 );
        }

        bool ClawMesh::getReceiveShadows() const
        {
            return m_receiveShadows.load();
        }

        void ClawMesh::setVisibilityFlags( u32 flags )
        {
            GraphicsMesh::setVisibilityFlags( flags );
            if( m_renderObject )
                wp_graphics_object_set_visibility_flags( m_renderObject, flags );
        }

        String ClawMesh::getMeshName() const
        {
            return GraphicsMesh::getMeshName();
        }

        void ClawMesh::setMeshName( const String &meshName )
        {
            m_meshName = meshName;
            GraphicsMesh::setMeshName( meshName );
        }

        ProgressiveMeshOptions ClawMesh::getProgressiveMeshOptions() const
        {
            return GraphicsMesh::getProgressiveMeshOptions();
        }

        void ClawMesh::setProgressiveMeshOptions( const ProgressiveMeshOptions &options )
        {
            m_options = options;
            GraphicsMesh::setProgressiveMeshOptions( options );
        }

        SmartPtr<IGraphicsSkeleton> ClawMesh::getSkeleton() const
        {
            return GraphicsMesh::getSkeleton();
        }

        void ClawMesh::setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton )
        {
            GraphicsMesh::setSkeleton( skeleton );
        }

        bool ClawMesh::setSkinningData( const Array<wp_skin_vertex> &vertices )
        {
            if( !m_mesh || vertices.size() != wp_graphics_mesh_get_vertex_count( m_mesh ) || vertices.empty() ) return false;
            const auto format = wp_graphics_mesh_get_vertex_format( m_mesh );
            if( format != WORKPHONE_VERTEX_FORMAT_PNT && format != WORKPHONE_VERTEX_FORMAT_PNTC ) return false;
            // Validate indices/weights with an identity palette before publishing bind data.
            Array<wp_mat4f> identity( WP_SKIN_MAX_JOINTS );
            for( auto &matrix : identity )
            {
                std::memset( &matrix, 0, sizeof(matrix) );
                for( u32 i = 0; i < 4; ++i ) matrix.m[i][i] = 1.0f;
            }
            Array<wp_skin_result> validated( vertices.size() );
            if( !wp_skin_vertices( vertices.data(), static_cast<wp_u32>( vertices.size() ),
                identity.data(), static_cast<wp_u32>( identity.size() ), validated.data() ) ) return false;
            m_skinVertices = vertices;
            m_skinOutput = std::move( validated );
            return true;
        }

        bool ClawMesh::hasSkinningData() const { return !m_skinVertices.empty(); }

        bool ClawMesh::applySkinningPalette( const Array<wp_mat4f> &palette )
        {
            if( !m_mesh || m_skinVertices.empty() || m_skinVertices.size() != wp_graphics_mesh_get_vertex_count( m_mesh ) ||
                palette.empty() || palette.size() > WP_SKIN_MAX_JOINTS ) return false;
            if( !wp_skin_vertices( m_skinVertices.data(), static_cast<wp_u32>( m_skinVertices.size() ),
                palette.data(), static_cast<wp_u32>( palette.size() ), m_skinOutput.data() ) ) return false;
            const auto count = wp_graphics_mesh_get_vertex_count( m_mesh );
            const auto stride = wp_graphics_mesh_get_vertex_stride( m_mesh );
            const auto source = static_cast<const u8 *>( wp_graphics_mesh_get_vertices( m_mesh ) );
            if( !source || stride < sizeof(wp_vec3f) * 2 ) return false;
            Array<u8> vertices( static_cast<size_t>(count) * stride );
            std::memcpy( vertices.data(), source, vertices.size() );
            for( u32 i = 0; i < count; ++i )
            {
                std::memcpy( vertices.data() + static_cast<size_t>(i) * stride, &m_skinOutput[i].position, sizeof(wp_vec3f) );
                std::memcpy( vertices.data() + static_cast<size_t>(i) * stride + sizeof(wp_vec3f), &m_skinOutput[i].normal, sizeof(wp_vec3f) );
            }
            if( !wp_graphics_mesh_set_vertices( m_mesh, wp_graphics_mesh_get_vertex_format(m_mesh), vertices.data(), count ) ) return false;
            ClawRendererDX11::forgetMesh( m_mesh );
            wp_graphics_mesh_compute_aabb( m_mesh );
            if( m_renderObject ) wp_graphics_object_set_local_aabb( m_renderObject, wp_graphics_mesh_get_local_aabb(m_mesh) );
            return true;
        }

        SmartPtr<IGraphicsObject> ClawMesh::clone( const String &name ) const
        {
            auto mesh = workphone::make_ptr<ClawMesh>();
            mesh->setMeshName( name.empty() ? getMeshName() : name );
            mesh->setProgressiveMeshOptions( getProgressiveMeshOptions() );
            mesh->setMaterialName( getMaterialName() );
            mesh->setMaterial( getMaterial() );
            mesh->setSkeleton( getSkeleton() );
            mesh->setVisible( isVisible() );
            mesh->setCastShadows( getCastShadows() );
            mesh->setReceiveShadows( getReceiveShadows() );
            mesh->setVisibilityFlags( getVisibilityFlags() );
            return mesh;
        }

        void ClawMesh::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                // Scene-node attachment expects a graphics object, not a mesh resource.
                *ppObject = m_renderObject;
            }
        }
    }  // namespace render
}  // namespace workphone
