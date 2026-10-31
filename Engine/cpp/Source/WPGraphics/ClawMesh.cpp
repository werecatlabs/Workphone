#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMesh.hpp>
#if WP_FOLIAGE_SOURCE_RESOURCES
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#endif
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_mesh_reader.h"
#include "workphone_graphics_object.h"
#include <workphone_graphics_renderer.h>
#include "ClawAssetBudget.hpp"
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
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMesh, GraphicsMesh );

        namespace
        {
            class NativeMeshLifecycleLock
            {
            public:
                NativeMeshLifecycleLock() : m_graphics( currentGraphics() ), m_lock( m_graphics )
                {
                }

            private:
                static SmartPtr<IGraphicsSystem> currentGraphics()
                {
                    auto app = core::IApplicationManager::instancePtr();
                    return app ? app->getGraphicsSystem() : nullptr;
                }

                SmartPtr<IGraphicsSystem> m_graphics;
                ScopedLock<IGraphicsSystem> m_lock;
            };

            // City/HLOD activation observes Loaded only after actual GPU
            // admission. Keep the previous native mesh until every candidate
            // range has uploaded successfully on the current render owner.
            wp_renderer_dx11 *activeNativeRenderer()
            {
                auto application = core::IApplicationManager::instancePtr();
                auto graphics = application ? application->getGraphicsSystem() : nullptr;
                auto renderer = graphics
                                    ? dynamic_pointer_cast<ClawRendererDX11>( graphics->getRenderer() )
                                    : nullptr;
                return renderer && renderer->isLoaded()
                           ? wp_renderer_get_dx11( renderer->getNativeRenderer() )
                           : nullptr;
            }

            bool prepareNativeGeometry( wp_graphics_mesh *mesh )
            {
                auto native = activeNativeRenderer();
                if( !native )
                {
                    return true;
                }
                const auto count = wp_graphics_mesh_get_index_count( mesh )
                                       ? wp_graphics_mesh_get_index_count( mesh )
                                       : wp_graphics_mesh_get_vertex_count( mesh );
                const auto topology = wp_graphics_mesh_get_primitive_type( mesh );
                if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_LIST )
                {
                    return count >= 3 && asset_budget::prepareMesh( native, mesh, 0, count );
                }
                if( topology == WORKPHONE_PRIMITIVE_TRIANGLE_STRIP )
                {
                    const auto sections = wp_graphics_mesh_get_submesh_count( mesh );
                    if( !sections )
                    {
                        return count >= 3 && asset_budget::prepareMesh( native, mesh, 0, count );
                    }
                    for( s32 i = 0; i < sections; ++i )
                    {
                        const auto section = wp_graphics_mesh_get_submesh( mesh, i );
                        if( !section || section->index_count < 3 ||
                            !asset_budget::prepareMesh( native, mesh, section->index_start,
                                                        section->index_count ) )
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            void destroyNativeMesh( wp_graphics_mesh *mesh )
            {
                if( mesh )
                {
                    ClawRendererDX11::forgetMesh( mesh );
                    wp_graphics_mesh_destroy( mesh );
                }
            }

            // Convert the registered CPU resource without creating a temporary mesh file.
            wp_graphics_mesh *createProceduralNativeMesh( SmartPtr<workphone::IMesh> mesh )
            {
                if( !mesh )
                {
                    return nullptr;
                }
                StreamBudget workingBudget;
                Array<wp_graphics_mesh_vertex_pnt> vertices;
                Array<wp_u32> indices;
                Array<Pair<wp_u32, wp_u32>> ranges;
                Array<wp_u32> materials;
                u64 vertexCount = 0, indexCount = 0, sectionCount = 0;
                const auto sections = mesh->getNumSubMeshes();
                for( u32 sectionIndex = 0; sectionIndex < sections; ++sectionIndex )
                {
                    const auto section = mesh->getSubMesh( sectionIndex );
                    if( !section || section->getUseSharedVertices() )
                    {
                        return nullptr;
                    }
                    auto vb = section->getVertexBuffer();
                    auto ib = section->getIndexBuffer();
                    if( !vb || !ib )
                    {
                        return nullptr;
                    }
                    if( !vb->getNumVertices() || !ib->getNumIndices() )
                    {
                        continue;
                    }
                    vertexCount += vb->getNumVertices();
                    indexCount += ib->getNumIndices();
                    ++sectionCount;
                    if( vertexCount > std::numeric_limits<u32>::max() ||
                        indexCount > std::numeric_limits<u32>::max() )
                    {
                        return nullptr;
                    }
                }
                if( !vertexCount || !indexCount )
                {
                    return nullptr;
                }
                // Reserve all flattening scratch before vector growth. Native
                // storage obtains independent SourceCpu leases for its copy.
                workingBudget = asset_budget::reserveCpuBytes(
                    vertexCount * sizeof( vertices[0] ) + indexCount * sizeof( indices[0] ) +
                        sectionCount * ( sizeof( ranges[0] ) + sizeof( materials[0] ) ),
                    true );
                if( !workingBudget )
                {
                    return nullptr;
                }
                vertices.reserve( static_cast<size_t>( vertexCount ) );
                indices.reserve( static_cast<size_t>( indexCount ) );
                ranges.reserve( static_cast<size_t>( sectionCount ) );
                materials.reserve( static_cast<size_t>( sectionCount ) );
                for( u32 sectionIndex = 0; sectionIndex < sections; ++sectionIndex )
                {
                    const auto section = mesh->getSubMesh( sectionIndex );
                    if( !section || section->getUseSharedVertices() )
                    {
                        return nullptr;
                    }
                    auto vb = section->getVertexBuffer();
                    auto ib = section->getIndexBuffer();
                    if( !vb || !ib )
                    {
                        return nullptr;
                    }
                    if( !vb->getNumVertices() || !ib->getNumIndices() )
                    {
                        continue;
                    }
                    auto declaration = vb->getVertexDeclaration();
                    if( !declaration )
                    {
                        return nullptr;
                    }
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
                    {
                        return nullptr;
                    }
                    const auto limit = std::numeric_limits<wp_u32>::max();
                    if( vertices.size() + vb->getNumVertices() > limit ||
                        indices.size() + ib->getNumIndices() > limit || ib->getNumIndices() % 3 != 0 ||
                        !ib->getIndexData() || vertices.size() + vb->getNumVertices() > vertexCount ||
                        indices.size() + ib->getNumIndices() > indexCount ||
                        ranges.size() >= sectionCount )
                    {
                        return nullptr;
                    }
                    const auto base = static_cast<wp_u32>( vertices.size() );
                    auto read = [&]( SmartPtr<IVertexElement> element, u32 vertex, float *out,
                                     u32 count ) {
                        const auto stride = declaration->getSize( element->getSource() );
                        const auto data =
                            static_cast<const u8 *>( vb->getVertexData( element->getSource() ) );
                        if( !data || element->getOffset() + count * sizeof( float ) > stride )
                        {
                            return false;
                        }
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
                        {
                            return nullptr;
                        }
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
                        {
                            return nullptr;
                        }
                        indices.push_back( base + index );
                    }
                }
                if( vertices.empty() || indices.empty() )
                {
                    return nullptr;
                }
                std::unique_ptr<wp_graphics_mesh, decltype( &wp_graphics_mesh_destroy )> result(
                    asset_budget::createMesh(), &wp_graphics_mesh_destroy );
                if( !result ||
                    !wp_graphics_mesh_set_vertices( result.get(), WORKPHONE_VERTEX_FORMAT_PNT,
                                                    vertices.data(),
                                                    static_cast<wp_u32>( vertices.size() ) ) ||
                    !wp_graphics_mesh_set_indices_u32( result.get(), indices.data(),
                                                       static_cast<wp_u32>( indices.size() ) ) )
                {
                    return nullptr;
                }
                for( size_t i = 0; i < ranges.size(); ++i )
                {
                    if( wp_graphics_mesh_add_submesh( result.get(), ranges[i].first, ranges[i].second,
                                                      materials[i] ) < 0 )
                    {
                        return nullptr;
                    }
                }
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
            NativeMeshLifecycleLock lifecycleLock;
            // Meshes are initially queued by ClawScene before MeshRenderer supplies
            // their resource path. Permit the later queued load to populate an
            // already-created, but still empty, native mesh.
            if( m_mesh && wp_graphics_mesh_get_vertex_count( m_mesh ) > 0 )
            {
                setLoadingState( prepareNativeGeometry( m_mesh ) ? LoadingState::Loaded
                                                                 : LoadingState::Unloaded );
                return;
            }

            setLoadingState( LoadingState::Loading );

            if( !m_mesh )
            {
                m_mesh = asset_budget::createMesh();
                if( !m_mesh )
                {
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }
            }
            if( m_renderObject )
            {
                wp_graphics_object_set_mesh( m_renderObject, m_mesh );
            }

            GraphicsMesh::load( data );

            if( StringUtil::isNullOrEmpty( m_meshName.str() ) )
            {
                // Scene creation may enqueue an empty placeholder before the
                // component supplies its path; it is not GPU-ready geometry.
                setLoadingState( activeNativeRenderer() ? LoadingState::Unloaded
                                                        : LoadingState::Loaded );
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

                auto workingBudget = asset_budget::reserveCpuBytes( streamSize, true );
                if( !workingBudget )
                {
                    WP_LOG_WARNING( "ClawMesh::load: CPU decode budget rejected: " + m_meshName.str() );
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

                loadedMesh = wp_graphics_mesh_read_from_buffer_budgeted(
                    bytes.data(), static_cast<wp_u32>( bytes.size() ),
                    &asset_budget::meshMemoryCallbacks );
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

            if( !prepareNativeGeometry( loadedMesh ) )
            {
                destroyNativeMesh( loadedMesh );
                WP_LOG_WARNING( "ClawMesh::load: GPU asset budget or upload rejected: " +
                                m_meshName.str() );
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

        bool ClawMesh::loadResource(std::shared_ptr<const resource::RuntimeResource> resource,String &error)
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            NativeMeshLifecycleLock lifecycleLock;
            if(!resource) { error="Missing cooked mesh"; return false; }
            if(!MeshResourceCompiler::validate(*resource,error)) return false;
            auto candidate=wp_graphics_mesh_read_from_buffer_budgeted(resource->payload.data(),
                static_cast<wp_u32>(resource->payload.size()),&asset_budget::meshMemoryCallbacks);
            if(!candidate) { error="Cooked mesh decode budget exhausted"; return false; }
            if(!prepareNativeGeometry(candidate))
            { destroyNativeMesh(candidate); error="Cooked mesh upload failed"; return false; }
            auto previous=m_mesh; m_mesh=candidate;
            if(m_renderObject) {
                wp_graphics_object_set_submit_data(m_renderObject,this);
                wp_graphics_object_set_mesh(m_renderObject,m_mesh);
                wp_graphics_object_set_local_aabb(m_renderObject,wp_graphics_mesh_get_local_aabb(m_mesh));
            }
            clearSkinningData();
            destroyNativeMesh(previous);
            setLoadingState(LoadingState::Loaded); return true;
#else
            error="Cooked resource support is unavailable in this build"; return false;
#endif
        }

        void ClawMesh::reload( SmartPtr<ISharedObject> data )
        {
            NativeMeshLifecycleLock lifecycleLock;
            auto previous = m_mesh;
            const auto previousState = getLoadingState();
            m_mesh = nullptr;
            try
            {
                load( data );
            }
            catch( ... )
            {
                destroyNativeMesh( m_mesh );
                m_mesh = previous;
                setLoadingState( previousState );
                if( m_renderObject )
                {
                    wp_graphics_object_set_mesh( m_renderObject, m_mesh );
                }
                throw;
            }
            if( !isLoaded() )
            {
                destroyNativeMesh( m_mesh );
                m_mesh = previous;
                setLoadingState( previousState );
                if( m_renderObject )
                {
                    wp_graphics_object_set_mesh( m_renderObject, m_mesh );
                }
                return;
            }
            destroyNativeMesh( previous );
            clearSkinningData();
        }

        void ClawMesh::unload( SmartPtr<ISharedObject> data )
        {
            NativeMeshLifecycleLock lifecycleLock;
            clearSkinningData();
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
            NativeMeshLifecycleLock lifecycleLock;
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
                            {
                                setMaterial( material, index );
                            }
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
            NativeMeshLifecycleLock lifecycleLock;
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
            NativeMeshLifecycleLock lifecycleLock;
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
            NativeMeshLifecycleLock lifecycleLock;
            m_castShadows = castShadows;
            GraphicsMesh::setCastShadows( castShadows );
            if( m_renderObject )
            {
                wp_graphics_object_set_cast_shadows( m_renderObject, castShadows ? 1 : 0 );
            }
        }

        bool ClawMesh::getCastShadows() const
        {
            return m_castShadows.load();
        }

        void ClawMesh::setReceiveShadows( bool receiveShadows )
        {
            NativeMeshLifecycleLock lifecycleLock;
            m_receiveShadows = receiveShadows;
            GraphicsMesh::setReceiveShadows( receiveShadows );
            if( m_renderObject )
            {
                wp_graphics_object_set_receive_shadows( m_renderObject, receiveShadows ? 1 : 0 );
            }
        }

        bool ClawMesh::getReceiveShadows() const
        {
            return m_receiveShadows.load();
        }

        void ClawMesh::setVisibilityFlags( u32 flags )
        {
            NativeMeshLifecycleLock lifecycleLock;
            GraphicsMesh::setVisibilityFlags( flags );
            if( m_renderObject )
            {
                wp_graphics_object_set_visibility_flags( m_renderObject, flags );
            }
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
            ProgressiveMeshOptions staged( options );
            GraphicsMesh::setProgressiveMeshOptions( options );
            m_options = std::move( staged );
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
            NativeMeshLifecycleLock lifecycleLock;
            if( !m_mesh || vertices.size() != wp_graphics_mesh_get_vertex_count( m_mesh ) ||
                vertices.empty() )
            {
                return false;
            }
            const auto format = wp_graphics_mesh_get_vertex_format( m_mesh );
            if( format != WORKPHONE_VERTEX_FORMAT_PNT && format != WORKPHONE_VERTEX_FORMAT_PNTC )
            {
                return false;
            }
            auto workingBudget = asset_budget::reserveCpuBytes(
                static_cast<u64>( WP_SKIN_MAX_JOINTS ) * sizeof( wp_mat4f ), true );
            auto inputBudget = asset_budget::reserveCpuBytes(
                static_cast<u64>( vertices.size() ) * sizeof( wp_skin_vertex ), false );
            auto outputBudget = asset_budget::reserveCpuBytes(
                static_cast<u64>( vertices.size() ) * sizeof( wp_skin_result ), false );
            if( !workingBudget || !inputBudget || !outputBudget )
            {
                return false;
            }
            // Validate indices/weights with an identity palette before publishing bind data.
            Array<wp_mat4f> identity( WP_SKIN_MAX_JOINTS );
            for( auto &matrix : identity )
            {
                std::memset( &matrix, 0, sizeof( matrix ) );
                for( u32 i = 0; i < 4; ++i )
                {
                    matrix.m[i][i] = 1.0f;
                }
            }
            Array<wp_skin_result> validated( vertices.size() );
            if( !wp_skin_vertices( vertices.data(), static_cast<wp_u32>( vertices.size() ),
                                   identity.data(), static_cast<wp_u32>( identity.size() ),
                                   validated.data() ) )
            {
                return false;
            }
            // Array's copy constructor preserves caller capacity. Allocate the
            // exact bind size so unused caller capacity cannot bypass admission.
            auto bind = std::make_shared<Array<wp_skin_vertex>>( vertices.size() );
            std::copy( vertices.begin(), vertices.end(), bind->begin() );
            clearSkinningData();
            m_skinInputBudget = std::move( inputBudget );
            m_skinVertices = std::move( bind );
            m_skinOutputBudget = std::move( outputBudget );
            m_skinOutput = std::move( validated );
            return true;
        }

        bool ClawMesh::hasSkinningData() const
        {
            return m_skinVertices && !m_skinVertices->empty();
        }

        void ClawMesh::clearSkinningData()
        {
            m_poseSkeleton = nullptr;
            m_poseBones = makeSourceBudgetSnapshot<SmartPtr<IBone>>();
            m_inverseBind = makeSourceBudgetSnapshot<wp_mat4f>();
            m_posePalette = makeSourceBudgetSnapshot<wp_mat4f>();
            m_skinVertices.reset();
            m_skinInputBudget.reset();
            Array<wp_skin_result>().swap( m_skinOutput );
            m_skinOutputBudget.reset();
        }

        bool ClawMesh::applySkinningPalette( const Array<wp_mat4f> &palette )
        {
            NativeMeshLifecycleLock lifecycleLock;
            if( !m_mesh || !m_skinVertices || m_skinVertices->empty() ||
                m_skinVertices->size() != wp_graphics_mesh_get_vertex_count( m_mesh ) ||
                palette.empty() || palette.size() > WP_SKIN_MAX_JOINTS )
            {
                return false;
            }
            if( !wp_skin_vertices( m_skinVertices->data(), static_cast<wp_u32>( m_skinVertices->size() ),
                                   palette.data(), static_cast<wp_u32>( palette.size() ),
                                   m_skinOutput.data() ) )
            {
                return false;
            }
            const auto count = wp_graphics_mesh_get_vertex_count( m_mesh );
            const auto stride = wp_graphics_mesh_get_vertex_stride( m_mesh );
            const auto source = static_cast<const u8 *>( wp_graphics_mesh_get_vertices( m_mesh ) );
            if( !source || stride < sizeof( wp_vec3f ) * 2 )
            {
                return false;
            }
            auto workingBudget =
                asset_budget::reserveCpuBytes( static_cast<u64>( count ) * stride, true );
            if( !workingBudget )
            {
                return false;
            }
            Array<u8> vertices( static_cast<size_t>( count ) * stride );
            wp_graphics_obb previousBounds;
            const bool hasPreviousBounds =
                wp_graphics_mesh_get_local_obb( m_mesh, &previousBounds ) != 0;
            std::memcpy( vertices.data(), source, vertices.size() );
            for( u32 i = 0; i < count; ++i )
            {
                std::memcpy( vertices.data() + static_cast<size_t>( i ) * stride,
                             &m_skinOutput[i].position, sizeof( wp_vec3f ) );
                std::memcpy( vertices.data() + static_cast<size_t>( i ) * stride + sizeof( wp_vec3f ),
                             &m_skinOutput[i].normal, sizeof( wp_vec3f ) );
            }
            if( !wp_graphics_mesh_set_vertices( m_mesh, wp_graphics_mesh_get_vertex_format( m_mesh ),
                                                vertices.data(), count ) )
            {
                return false;
            }
            // Reproject every deformed position; previous extents never constrain the new pose.
            // Content generation drives transactional GPU replacement. Retain
            // the old cache until the next generation is actually admitted.
            // Refresh a loose orientation when its surface proxy exceeds the current AABB by 50%.
            wp_graphics_mesh_refit_bounds_adaptive( m_mesh,
                                                    hasPreviousBounds ? &previousBounds : nullptr, 1.5 );
            if( m_renderObject )
            {
                wp_graphics_object_set_local_aabb( m_renderObject,
                                                   wp_graphics_mesh_get_local_aabb( m_mesh ) );
            }
            return true;
        }

        bool ClawMesh::applySkeletonPose( SmartPtr<ISkeleton> skeleton, String &error )
        {
            NativeMeshLifecycleLock lifecycleLock;
            error.clear();
            if( !m_mesh || !isLoaded() )
            {
                error = "Animated mesh is waiting for its render resource.";
                return false;
            }
            auto rig = dynamic_pointer_cast<MeshSkeleton>( skeleton );
            if( !rig )
            {
                error = "Claw skeletal playback requires a MeshSkeleton.";
                return false;
            }
            try
            {
                auto model = []( SmartPtr<IBone> bone, bool binding ) {
                    auto matrix = Matrix4F::identity();
                    size_t depth = 0;
                    for( auto current = bone; current; current = current->getParent() )
                    {
                        if( ++depth > WP_SKIN_MAX_JOINTS )
                        {
                            throw std::runtime_error( "Cyclic or oversized skin hierarchy" );
                        }
                        auto concrete = dynamic_pointer_cast<Bone>( current );
                        if( !concrete )
                        {
                            throw std::runtime_error( "Claw requires accessible bone binding poses" );
                        }
                        Matrix4F local;
                        local.makeTransform(
                            binding ? concrete->getBindingPosition() : current->getPosition(),
                            Vector3F( 1, 1, 1 ),
                            binding ? concrete->getBindingOrientation() : current->getOrientation() );
                        matrix = local * matrix;
                    }
                    return matrix;
                };
                auto nativeMatrix = []( const Matrix4F &matrix ) {
                    wp_mat4f value;
                    for( u32 r = 0; r < 4; ++r )
                    {
                        for( u32 c = 0; c < 4; ++c )
                        {
                            value.m[r][c] = matrix[r][c];
                        }
                    }
                    return value;
                };
                if( m_poseSkeleton != skeleton || !hasSkinningData() )
                {
                    auto bones = makeSourceBudgetSnapshot( rig->getBones() );
                    size_t joints = 0;
                    for( const auto &bone : bones )
                    {
                        if( bone )
                        {
                            joints = std::max( joints, size_t( bone->getBoneHandle() ) + 1 );
                        }
                    }
                    if( !joints || joints > WP_SKIN_MAX_JOINTS )
                    {
                        throw std::runtime_error( "Claw supports at most 256 skin joints" );
                    }
                    auto inverse = makeSourceBudgetSnapshot<wp_mat4f>();
                    inverse.resize( joints );
                    auto palette = makeSourceBudgetSnapshot<wp_mat4f>();
                    palette.resize( joints );
                    for( auto &entry : inverse )
                    {
                        entry = nativeMatrix( Matrix4F::identity() );
                    }
                    for( const auto &bone : bones )
                    {
                        if( bone )
                        {
                            inverse[bone->getBoneHandle()] =
                                nativeMatrix( model( bone, true ).inverse() );
                        }
                    }
                    const auto count = wp_graphics_mesh_get_vertex_count( m_mesh );
                    const auto stride = wp_graphics_mesh_get_vertex_stride( m_mesh );
                    const auto raw = static_cast<const u8 *>( wp_graphics_mesh_get_vertices( m_mesh ) );
                    const auto format = wp_graphics_mesh_get_vertex_format( m_mesh );
                    if( !raw || ( format != WORKPHONE_VERTEX_FORMAT_PNT &&
                                  format != WORKPHONE_VERTEX_FORMAT_PNTC ) )
                    {
                        throw std::runtime_error( "Unsupported animated vertex format" );
                    }
                    auto vertices = makeSourceBudgetSnapshot<wp_skin_vertex>();
                    vertices.resize( count );
                    for( u32 i = 0; i < count; ++i )
                    {
                        std::memcpy( &vertices[i].position, raw + size_t( i ) * stride,
                                     sizeof( wp_vec3f ) );
                        std::memcpy( &vertices[i].normal,
                                     raw + size_t( i ) * stride + sizeof( wp_vec3f ),
                                     sizeof( wp_vec3f ) );
                    }
                    auto app = core::IApplicationManager::instance();
                    auto manager = app ? app->getMeshManager() : nullptr;
                    auto resource = manager ? dynamic_pointer_cast<IMeshResource>(
                                                  manager->loadFromFile( m_meshName.str() ) )
                                            : nullptr;
                    auto source = resource ? resource->getMesh() : nullptr;
                    if( !source )
                    {
                        throw std::runtime_error( "Cannot load animated CPU mesh weights" );
                    }
                    size_t base = 0, weighted = 0;
                    for( u32 i = 0; i < source->getNumSubMeshes(); ++i )
                    {
                        auto section = source->getSubMesh( i );
                        auto buffer = section ? section->getVertexBuffer() : nullptr;
                        if( !buffer || section->getUseSharedVertices() )
                        {
                            throw std::runtime_error( "Unsupported shared skin vertex layout" );
                        }
                        if( base + buffer->getNumVertices() > count )
                        {
                            throw std::runtime_error( "Skin weight and render vertex layouts differ" );
                        }
                        for( const auto &assignment : section->getBoneAssignments() )
                        {
                            if( assignment && assignment->getWeight() > 0 )
                            {
                                if( assignment->getVertexIndex() >= buffer->getNumVertices() ||
                                    assignment->getBoneIndex() >= joints )
                                {
                                    throw std::runtime_error( "Invalid skin bone/vertex index" );
                                }
                                auto &vertex = vertices[base + assignment->getVertexIndex()];
                                u32 slot = 0;
                                while( slot < WP_SKIN_INFLUENCES && vertex.weights[slot] > 0 )
                                {
                                    ++slot;
                                }
                                if( slot == WP_SKIN_INFLUENCES )
                                {
                                    throw std::runtime_error(
                                        "Claw supports at most four weights per vertex" );
                                }
                                vertex.joints[slot] = assignment->getBoneIndex();
                                vertex.weights[slot] = assignment->getWeight();
                                ++weighted;
                            }
                        }
                        base += buffer->getNumVertices();
                    }
                    if( !weighted )
                    {
                        error = "Mesh cache contains no skin weights; reimport the animated model.";
                        return false;
                    }
                    if( base != count )
                    {
                        throw std::runtime_error( "Incomplete skin vertex layout" );
                    }
                    for( auto &vertex : vertices )
                    {
                        float sum = 0;
                        for( float weight : vertex.weights )
                        {
                            sum += weight;
                        }
                        if( sum > 0 )
                        {
                            for( float &weight : vertex.weights )
                            {
                                weight /= sum;
                            }
                        }
                    }
                    if( !setSkinningData( vertices ) )
                    {
                        throw std::runtime_error( "Skin binding admission or validation failed" );
                    }
                    m_poseBones = std::move( bones );
                    m_inverseBind = std::move( inverse );
                    m_posePalette = std::move( palette );
                    m_poseSkeleton = skeleton;
                }
                for( const auto &bone : m_poseBones )
                {
                    if( bone )
                    {
                        const auto handle = bone->getBoneHandle();
                        Matrix4F inverse;
                        for( u32 r = 0; r < 4; ++r )
                        {
                            for( u32 c = 0; c < 4; ++c )
                            {
                                inverse[r][c] = m_inverseBind[handle].m[r][c];
                            }
                        }
                        m_posePalette[handle] = nativeMatrix( model( bone, false ) * inverse );
                    }
                }
                if( !applySkinningPalette( m_posePalette ) )
                {
                    error = "Skin pose admission or validation failed; retaining previous geometry.";
                    return false;
                }
                return true;
            }
            catch( const std::exception &exception )
            {
                error = String( "Skeletal playback: " ) + exception.what();
                return false;
            }
        }

        bool ClawMesh::updateGeometry( const Array<wp_graphics_mesh_vertex_pntc> &vertices,
                                       const Array<u32> &indices )
        {
            NativeMeshLifecycleLock lifecycleLock;
            if( !m_mesh || vertices.empty() || indices.empty() || indices.size() % 3 != 0 ||
                vertices.size() > std::numeric_limits<u32>::max() ||
                indices.size() > std::numeric_limits<u32>::max() )
            {
                return false;
            }
            for( auto index : indices )
            {
                if( index >= vertices.size() )
                {
                    return false;
                }
            }
            std::unique_ptr<wp_graphics_mesh, decltype( &destroyNativeMesh )> candidate(
                asset_budget::createMesh(), &destroyNativeMesh );
            if( !candidate ||
                !wp_graphics_mesh_set_vertices( candidate.get(), WORKPHONE_VERTEX_FORMAT_PNTC,
                                                vertices.data(), static_cast<u32>( vertices.size() ) ) ||
                !wp_graphics_mesh_set_indices_u32( candidate.get(), indices.data(),
                                                   static_cast<u32>( indices.size() ) ) )
            {
                return false;
            }
            if( wp_graphics_mesh_add_submesh( candidate.get(), 0, static_cast<u32>( indices.size() ),
                                              0 ) < 0 ||
                !prepareNativeGeometry( candidate.get() ) )
            {
                return false;
            }
            // New authored geometry needs an explicit new binding; equal vertex
            // counts alone do not make the old skin indices/positions compatible.
            clearSkinningData();
            auto previous = m_mesh;
            m_mesh = candidate.release();
            wp_graphics_mesh_compute_aabb( m_mesh );
            destroyNativeMesh( previous );
            if( m_renderObject )
            {
                wp_graphics_object_set_mesh( m_renderObject, m_mesh );
                wp_graphics_object_set_local_aabb( m_renderObject,
                                                   wp_graphics_mesh_get_local_aabb( m_mesh ) );
            }
            setLoadingState( LoadingState::Loaded );
            return true;
        }

        SmartPtr<IGraphicsObject> ClawMesh::clone( const String &name ) const
        {
            NativeMeshLifecycleLock lifecycleLock;
            auto mesh = workphone::make_ptr<ClawMesh>();
            mesh->setMeshName( name.empty() ? getMeshName() : name );
            mesh->setProgressiveMeshOptions( getProgressiveMeshOptions() );
            mesh->setMaterialName( getMaterialName() );
            mesh->setMaterial( getMaterial() );
            for( size_t i = 0; i < m_subMaterialNames.size(); ++i )
            {
                mesh->setMaterialName( m_subMaterialNames[i], static_cast<s32>( i ) );
            }
            for( size_t i = 0; i < m_subMaterials.size(); ++i )
            {
                mesh->setMaterial( m_subMaterials[i], static_cast<s32>( i ) );
            }
            mesh->setHardwareAnimationEnabled( m_hardwareAnimationEnabled );
            mesh->setSkeleton( getSkeleton() );
            mesh->setVisible( isVisible() );
            mesh->setCastShadows( getCastShadows() );
            mesh->setReceiveShadows( getReceiveShadows() );
            mesh->setVisibilityFlags( getVisibilityFlags() );
            if( m_mesh )
            {
                std::unique_ptr<wp_graphics_mesh, decltype( &destroyNativeMesh )> candidate(
                    asset_budget::createMesh(), &destroyNativeMesh );
                const auto vertices = wp_graphics_mesh_get_vertex_count( m_mesh );
                const auto indices = wp_graphics_mesh_get_index_count( m_mesh );
                if( !candidate ||
                    ( vertices && !wp_graphics_mesh_set_vertices(
                                      candidate.get(), wp_graphics_mesh_get_vertex_format( m_mesh ),
                                      wp_graphics_mesh_get_vertices( m_mesh ), vertices ) ) )
                {
                    return nullptr;
                }
                if( indices )
                {
                    const auto copied =
                        wp_graphics_mesh_get_index_format( m_mesh ) == WORKPHONE_INDEX_FORMAT_UINT16
                            ? wp_graphics_mesh_set_indices_u16(
                                  candidate.get(),
                                  static_cast<const uint16_t *>(
                                      wp_graphics_mesh_get_indices( m_mesh ) ),
                                  indices )
                            : wp_graphics_mesh_set_indices_u32(
                                  candidate.get(),
                                  static_cast<const wp_u32 *>( wp_graphics_mesh_get_indices( m_mesh ) ),
                                  indices );
                    if( !copied )
                    {
                        return nullptr;
                    }
                }
                wp_graphics_mesh_set_primitive_type( candidate.get(),
                                                     wp_graphics_mesh_get_primitive_type( m_mesh ) );
                for( wp_s32 i = 0; i < wp_graphics_mesh_get_submesh_count( m_mesh ); ++i )
                {
                    const auto section = wp_graphics_mesh_get_submesh( m_mesh, i );
                    if( !section ||
                        wp_graphics_mesh_add_submesh( candidate.get(), section->index_start,
                                                      section->index_count, section->material_id ) < 0 )
                    {
                        return nullptr;
                    }
                }
                wp_graphics_mesh_compute_aabb( candidate.get() );
                if( !prepareNativeGeometry( candidate.get() ) )
                {
                    return nullptr;
                }
                mesh->m_mesh = candidate.release();
                auto outputBudget = asset_budget::reserveCpuBytes(
                    static_cast<u64>( m_skinOutput.size() ) * sizeof( wp_skin_result ), false );
                if( !m_skinOutput.empty() && !outputBudget )
                {
                    return nullptr;
                }
                mesh->m_skinInputBudget = m_skinInputBudget;
                mesh->m_skinVertices = m_skinVertices;
                mesh->m_skinOutputBudget = std::move( outputBudget );
                mesh->m_skinOutput = m_skinOutput;
                mesh->setLoadingState( getLoadingState() );
            }
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
