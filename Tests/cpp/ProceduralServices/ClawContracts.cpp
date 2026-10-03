#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <workphone_graphics_object.h>
#include <stdexcept>
#include <memory>
using namespace workphone;
namespace
{
    void check( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }
}  // namespace
void checkClawContracts()
{
    auto app = core::IApplicationManager::instance();
    auto manager = workphone::make_ptr<MeshManager>();
    auto previousManager = app->getMeshManager();
    app->setMeshManager( manager );
    auto mesh = workphone::make_ptr<workphone::Mesh>();
    Array<Vector3<real_Num>> positions{ { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
    Array<Vector3<real_Num>> normals( 3, { 0, 0, 1 } );
    Array<Vector2<real_Num>> uv{ { 0, 0 }, { 1, 0 }, { 0, 1 } };
    Array<u32> indices{ 0, 1, 2 };
    auto first = MeshUtil::createMesh( positions, normals, uv, indices );
    auto second = MeshUtil::createMesh( positions, normals, uv, indices );
    mesh->addSubMesh( first->getSubMesh( 0 ) );
    mesh->addSubMesh( second->getSubMesh( 0 ) );
    auto resource = workphone::dynamic_pointer_cast<IMeshResource>( manager->create( "12e51f20-a2f7-4529-afc1-c7c0f6f78000" ) );
    resource->setFilePath( "__procedural/native-test.meshbin" );
    resource->setMesh( mesh );
    resource->setLoadingState( LoadingState::Loaded );
    manager->addMeshResource( resource );
    std::unique_ptr<wp_graphics_object, decltype( &wp_graphics_object_destroy )> object(
        wp_graphics_object_create(), &wp_graphics_object_destroy );
    check( object != nullptr, "Native render object was not created" );
    auto graphics = workphone::make_ptr<render::ClawMesh>();
    graphics->setMeshName( resource->getFilePath() );
    graphics->bindNativeRenderObject( object.get() );
    graphics->load( nullptr );
    check( graphics->isLoaded(), "Claw did not load the registered CPU mesh" );
    auto native = graphics->getNativeMesh();
    check( wp_graphics_mesh_get_vertex_count( native ) == 6, "Claw did not preserve section vertices" );
    check( wp_graphics_mesh_get_index_count( native ) == 6 &&
               wp_graphics_mesh_get_submesh_count( native ) == 2,
           "Claw did not preserve section index ranges" );
    const auto nativeIndices = static_cast<const wp_u32 *>( wp_graphics_mesh_get_indices( native ) );
    check( nativeIndices[3] == 3 && nativeIndices[5] == 5, "Claw did not rebase section indices" );
    check( wp_graphics_object_get_mesh( object.get() ) == native,
           "Claw did not bind the generated mesh" );
    mesh->removeAllSubMeshes();
    mesh->addSubMesh( first->getSubMesh( 0 ) );
    graphics->reload( nullptr );
    native = graphics->getNativeMesh();
    check( graphics->isLoaded() && wp_graphics_mesh_get_vertex_count( native ) == 3,
           "Claw did not reload changed CPU geometry" );
    check( wp_graphics_object_get_mesh( object.get() ) == native,
           "Claw reload detached the render object" );
    auto ib = first->getSubMesh( 0 )->getIndexBuffer();
    if( ib->getIndexType() == IIndexBuffer::Type::IT_16BIT )
        static_cast<u16 *>( ib->getIndexData() )[0] = 99;
    else
        static_cast<u32 *>( ib->getIndexData() )[0] = 99;
    graphics->reload( nullptr );
    check( !graphics->isLoaded(), "Claw accepted an out-of-range generated mesh index" );
    graphics->unload( nullptr );
    graphics = nullptr;
    manager->removeMeshResource( resource );
    app->setMeshManager( previousManager );
}
