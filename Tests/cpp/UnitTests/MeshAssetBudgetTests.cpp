#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace workphone;

namespace
{
    class MeshBudgetWindow
    {
    public:
        explicit MeshBudgetWindow( u64 capacity ) : scheduler( WorldStreamingScheduler::shared() )
        {
            const auto stats = scheduler->stats();
            baseline = stats.used[size_t( StreamMemoryClass::SourceCpu )];
            previous = stats.limits[size_t( StreamMemoryClass::SourceCpu )];
            set( capacity );
        }
        ~MeshBudgetWindow()
        {
            String error;
            scheduler->setBudget( StreamMemoryClass::SourceCpu, previous, error );
        }
        void set( u64 capacity )
        {
            String error;
            BOOST_REQUIRE_MESSAGE( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline + capacity, error ), error );
        }
        u64 used() const { return scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )] - baseline; }
        std::shared_ptr<WorldStreamingScheduler> scheduler;
        u64 baseline = 0, previous = 0;
    };

    SmartPtr<VertexBuffer> triangleVertices( bool secondStream = false )
    {
        auto declaration = make_ptr<VertexDeclaration>();
        declaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3 );
        if( secondStream ) declaration->addElement( 1, 0, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                                   VertexElementType::VET_FLOAT2 );
        auto vertices = make_ptr<VertexBuffer>();
        vertices->setVertexDeclaration( declaration ); vertices->setNumVertices( 3 );
        auto data = static_cast<f32 *>( vertices->createVertexData() );
        const f32 positions[] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::copy( std::begin( positions ), std::end( positions ), data );
        if( secondStream )
        {
            auto uv = static_cast<f32 *>( vertices->createVertexData( 1 ) );
            for( u32 i = 0; i < 6; ++i ) uv[i] = f32( i );
        }
        return vertices;
    }

    SmartPtr<workphone::Mesh> triangleMesh()
    {
        auto indices = make_ptr<IndexBuffer>(); indices->setNumIndices( 3 );
        auto data = static_cast<u32 *>( indices->createIndexData() );
        data[0] = 0; data[1] = 1; data[2] = 2;
        auto submesh = make_ptr<SubMesh>(); submesh->setVertexBuffer( triangleVertices() );
        submesh->setIndexBuffer( indices ); submesh->setMaterialName( "BudgetTriangle" );
        auto mesh = make_ptr<workphone::Mesh>(); mesh->addSubMesh( submesh );
        mesh->updateAABB(); mesh->setBoundingSphereRadius( 1 );
        return mesh;
    }
}

BOOST_AUTO_TEST_SUITE( MeshAssetBudgetTests )

BOOST_AUTO_TEST_CASE( admission_counts_real_allocations_and_preserves_failed_replacements )
{
    TestGuard guard; MeshBudgetWindow budget( 72 );
    auto vertices = triangleVertices( true );
    auto indices = make_ptr<IndexBuffer>(); indices->setNumIndices( 3 );
    auto indexData = static_cast<u32 *>( indices->createIndexData() ); indexData[0] = 17;
    auto positions = vertices->getVertexData(), uv = vertices->getVertexData( 1 );
    const auto declaration = vertices->getVertexDeclaration();
    BOOST_CHECK_EQUAL( budget.used(), 72u );
    auto alias = vertices;
    BOOST_CHECK_EQUAL( budget.used(), 72u );

    indices->setNumIndices( 60 ); indices->setIndexType( IIndexBuffer::Type::IT_16BIT );
    BOOST_CHECK_THROW( indices->createIndexData(), std::runtime_error );
    BOOST_CHECK( indices->getIndexData() == indexData );
    BOOST_CHECK_EQUAL( indexData[0], 17u );
    BOOST_CHECK_EQUAL( indices->getNumIndices(), 3u );
    BOOST_CHECK_EQUAL( indices->getIndexSize(), 4u );
    BOOST_CHECK( indices->getIndexType() == IIndexBuffer::Type::IT_32BIT );

    vertices->setNumVertices( std::numeric_limits<u32>::max() );
    vertices->setVertexDeclaration( make_ptr<VertexDeclaration>() );
    vertices->getVertexDeclaration()->addElement( 0, 0, VertexElementSemantic::VES_POSITION,
                                                  VertexElementType::VET_FLOAT4 );
    BOOST_CHECK_THROW( vertices->createVertexData(), std::runtime_error );
    BOOST_CHECK_EQUAL( vertices->getNumVertices(), 3u );
    BOOST_CHECK( vertices->getVertexDeclaration() == declaration );
    BOOST_CHECK( vertices->getVertexData() == positions );
    BOOST_CHECK( vertices->getVertexData( 1 ) == uv );
    BOOST_CHECK_EQUAL( budget.used(), 72u );
    BOOST_CHECK( !vertices->getVertexData( 65536 ) );
    vertices->unload( nullptr ); indices->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( multistream_growth_commits_together_and_clones_reserve_distinct_storage )
{
    TestGuard guard;
    // Factory inventories/teardown capacity now share SourceCpu admission. Keep
    // their retained metadata in the baseline so the exact geometry limits below
    // still exercise first/second-stream staging and distinct clone buffers.
    { auto warm = triangleVertices(true); auto clone = warm->clone(); }
    MeshBudgetWindow budget( 155 );
    auto vertices = triangleVertices( true );
    auto positions = vertices->getVertexData(), uv = vertices->getVertexData( 1 );
    vertices->setNumVertices( 6 );
    // The first replacement fits; the second stream exhausts admission.
    BOOST_CHECK_THROW( vertices->createVertexData(), std::runtime_error );
    BOOST_CHECK_EQUAL( vertices->getNumVertices(), 3u );
    BOOST_CHECK( vertices->getVertexData() == positions );
    BOOST_CHECK( vertices->getVertexData( 1 ) == uv );
    BOOST_CHECK_EQUAL( budget.used(), 60u );
    budget.set( 240 ); vertices->setNumVertices( 6 ); vertices->createVertexData();
    BOOST_CHECK_EQUAL( budget.used(), 120u );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( vertices->getVertexData( 1 ) )[5], 5.f );
    auto clone = vertices->clone();
    BOOST_CHECK_EQUAL( budget.used(), 240u );
    BOOST_CHECK( clone->getVertexData() != vertices->getVertexData() );
    BOOST_CHECK( clone->getVertexData( 1 ) != vertices->getVertexData( 1 ) );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( clone->getVertexData( 1 ) )[5], 5.f );
    clone = nullptr; vertices = nullptr;
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( resource_retirement_keeps_direct_buffer_pins_charged )
{
    TestGuard guard; MeshBudgetWindow budget( 96 );
    auto manager = make_ptr<MeshManager>();
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        "Cache/cpu-budget-pins.fbmeshbin" ).first );
    BOOST_REQUIRE( resource );
    const auto inventory = budget.used();
    BOOST_CHECK_GT( inventory, 0u );
    budget.set( inventory + 96 );
    auto geometry = triangleMesh();
    auto submesh = geometry->getSubMesh( 0 );
    auto vertices = submesh->getVertexBuffer(); auto indices = submesh->getIndexBuffer();
    resource->setMesh( geometry ); resource->setLoadingState( LoadingState::Loaded );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    geometry = nullptr; submesh = nullptr;
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( vertices->getVertexData() )[3], 1.f );
    BOOST_CHECK_EQUAL( static_cast<u32 *>( indices->getIndexData() )[2], 2u );
    vertices = nullptr; BOOST_CHECK_EQUAL( budget.used(), inventory + 12u );
    indices = nullptr; BOOST_CHECK_EQUAL( budget.used(), inventory );
    resource->setFilePath( "__procedural/budget-pins" ); resource->setMesh( triangleMesh() );
    resource->setLoadingState( LoadingState::Loaded );
    auto procedural = manager->loadMesh( "__procedural/budget-pins" );
    BOOST_REQUIRE( procedural ); BOOST_CHECK( procedural == resource->getMesh() );
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    procedural = nullptr;
    manager->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( cooked_mesh_source_aliases_reload_and_eviction_share_exact_residency )
{
    TestGuard guard; MeshBudgetWindow budget( 48 );
    const auto directory = std::filesystem::temp_directory_path() /
                           ( "wp-mesh-budget-" + std::string( StringUtil::getUUID().c_str() ) );
    std::filesystem::create_directories( directory );
    guard.trackFilesystemPath( directory.generic_string().c_str() );
    const auto file = ( directory / "triangle.fbmeshbin" ).generic_string();
    {
        auto source = triangleMesh();
        MeshSerializer serializer; serializer.exportMesh( source.get(), String( file.c_str() ) );
    }
    BOOST_CHECK_EQUAL( budget.used(), 0u );
    std::ifstream originalInput( file, std::ios::binary );
    const std::string original( ( std::istreambuf_iterator<char>( originalInput ) ), {} );
    originalInput.close();
    BOOST_REQUIRE( !original.empty() );
    guard.fileSystem->refreshPath( directory.generic_string().c_str(), true );
    auto manager = make_ptr<MeshManager>();
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve( file.c_str() ).first );
    BOOST_REQUIRE( resource );
    const auto inventory = budget.used();
    BOOST_CHECK_GT( inventory, 0u );
    budget.set( inventory + 48 );
    resource->load( nullptr );
    BOOST_REQUIRE( resource->isLoaded() );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    const auto aliasPath = Path::getRelativePath( guard.applicationManager->getProjectPath(), file.c_str() );
    BOOST_CHECK_MESSAGE( manager->createOrRetrieve( aliasPath ).first == resource,
                         "Source aliases must share one resource: " << file << " and " << aliasPath );
    auto old = manager->loadMesh( file.c_str() );
    BOOST_REQUIRE( old ); BOOST_CHECK( old == resource->getMesh() );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );

    // Staging needs capacity for both generations. Refusal keeps the original
    // loaded mesh and its geometry, then a larger budget permits a real reload.
    resource->reload( nullptr );
    BOOST_CHECK( resource->isLoaded() ); BOOST_CHECK( resource->getMesh() == old );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    budget.set( inventory + 96 ); resource->reload( nullptr );
    BOOST_REQUIRE( resource->isLoaded() ); BOOST_CHECK( resource->getMesh() != old );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 96u );
    BOOST_CHECK_EQUAL( old->getSubMesh( 0 )->getVertexBuffer()->getNumVertices(), 3u );
    old = nullptr; BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    const auto lastGood = resource->getMesh().get();
    { std::ofstream corrupt( file, std::ios::binary | std::ios::trunc ); corrupt << "invalid mesh"; }
    resource->reload( nullptr );
    BOOST_CHECK( resource->isLoaded() ); BOOST_CHECK( resource->getMesh().get() == lastGood );
    BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory );
    { std::ofstream restored( file, std::ios::binary | std::ios::trunc ); restored.write( original.data(), original.size() ); }
    auto reloaded = manager->loadMesh( aliasPath );
    BOOST_REQUIRE( reloaded ); BOOST_CHECK_EQUAL( budget.used(), inventory + 48u );
    BOOST_CHECK( reloaded == resource->getMesh() );
    reloaded = nullptr;
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory );
    manager->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_SUITE_END()
