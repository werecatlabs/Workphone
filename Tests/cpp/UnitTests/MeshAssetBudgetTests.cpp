#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include "../MeshSerializerStagingBudgetContracts.hpp"
#include <array>
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

    template <class T> u64 storageBytes( size_t capacity )
    {
        return capacity ? sourceBudgetStorageBytes( capacity * sizeof( T ), alignof( T ) ) : 0;
    }

    u64 declarationBytes( size_t capacity = 4 )
    {
        return storageBytes<SmartPtr<IVertexElement>>( capacity );
    }

    u64 streamMetadataBytes( size_t streams )
    {
        return storageBytes<u8 *>( streams ) + storageBytes<StreamBudget>( streams ) +
               storageBytes<u64>( streams );
    }

    u64 streamScratchBytes( size_t streams )
    {
        // Match the physical staging record: its lease and data coexist with the
        // published streams until every replacement has been admitted.
        struct Replacement
        {
            StreamBudget lease;
            std::unique_ptr<u8[]> data;
            u64 bytes;
        };
        return storageBytes<Replacement>( streams );
    }

    u64 vertexResidency( size_t streams, u64 geometryBytes )
    {
        return declarationBytes() + streamMetadataBytes( streams ) + geometryBytes;
    }

    u64 triangleBuildPeak( bool secondStream = false )
    {
        const auto first = vertexResidency( 1, 36 ) + streamScratchBytes( 1 );
        if( !secondStream ) return first;
        return std::max( first, vertexResidency( 1, 36 ) + streamMetadataBytes( 2 ) +
                                   streamScratchBytes( 2 ) + 24 );
    }

    class VertexBufferProbe : public VertexBuffer
    {
    public:
        auto shape() const -> std::array<size_t, 6>
        {
            return { m_vertexData.size(), m_vertexData.capacity(), m_vertexBudgets.size(),
                     m_vertexBudgets.capacity(), m_vertexBytes.size(), m_vertexBytes.capacity() };
        }
        bool sourceInventories() const
        {
            return m_vertexData.get_allocator().isSourceBudgeted() &&
                   m_vertexBudgets.get_allocator().isSourceBudgeted() &&
                   m_vertexBytes.get_allocator().isSourceBudgeted();
        }
        const StreamBudgetLease *lease( size_t stream ) const
        {
            return stream < m_vertexBudgets.size() ? m_vertexBudgets[stream].get() : nullptr;
        }
        void reserveMetadata( size_t pointers, size_t leases, size_t sizes )
        {
            m_vertexData.reserve( pointers ); m_vertexBudgets.reserve( leases ); m_vertexBytes.reserve( sizes );
        }
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

    void warmMeshFactories()
    {
        // Keep retained factory inventories in the baseline, including enough
        // element slots for declaration-growth and clone-unwind assertions.
        auto declaration = make_ptr<VertexDeclaration>();
        for( u32 i = 0; i < 8; ++i )
            declaration->addElement( 0, i * sizeof( f32 ), VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                     VertexElementType::VET_FLOAT1, i );
        auto clone = declaration->clone();
    }
}

BOOST_AUTO_TEST_SUITE( MeshAssetBudgetTests )

BOOST_AUTO_TEST_CASE( cold_mesh_metadata_is_admitted_before_constructor_publication )
{
    TestGuard guard; MeshBudgetWindow budget( 0 );
    BOOST_CHECK_THROW( make_ptr<VertexBufferProbe>(), std::bad_alloc );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
    budget.set( storageBytes<u8 *>( 1 ) - 1 );
    BOOST_CHECK_THROW( make_ptr<VertexBufferProbe>(), std::bad_alloc );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
    budget.set( storageBytes<u8 *>( 1 ) );
    auto vertices = make_ptr<VertexBufferProbe>();
    BOOST_CHECK( vertices->sourceInventories() );
    BOOST_CHECK_EQUAL( budget.used(), storageBytes<u8 *>( 1 ) );
    vertices = nullptr;
    BOOST_CHECK_EQUAL( budget.used(), 0u );
    budget.set( declarationBytes() - 1 );
    BOOST_CHECK_THROW( make_ptr<VertexDeclaration>(), std::bad_alloc );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
    budget.set( declarationBytes() );
    auto declaration = make_ptr<VertexDeclaration>();
    BOOST_CHECK( declaration->getElements().get_allocator().isSourceBudgeted() );
    BOOST_CHECK_EQUAL( declaration->getElements().capacity(), 4u );
    BOOST_CHECK_EQUAL( budget.used(), declarationBytes() );
    declaration->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( stream_inventory_and_staging_denial_preserve_every_published_owner )
{
    TestGuard guard; warmMeshFactories(); MeshBudgetWindow budget( triangleBuildPeak() );
    auto declaration = make_ptr<VertexDeclaration>();
    declaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3 );
    declaration->addElement( 3, 0, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                             VertexElementType::VET_FLOAT2 );
    auto vertices = make_ptr<VertexBufferProbe>();
    vertices->setVertexDeclaration( declaration ); vertices->setNumVertices( 3 );
    auto positions = static_cast<f32 *>( vertices->createVertexData() ); positions[0] = 19.f;
    const auto shape = vertices->shape();
    const auto lease = vertices->lease( 0 );
    const auto resident = vertexResidency( 1, 36 );
    BOOST_CHECK_EQUAL( budget.used(), resident );
    const auto pointers = storageBytes<u8 *>( 4 );
    const auto leases = storageBytes<StreamBudget>( 4 );
    const auto sizes = storageBytes<u64>( 4 );
    const auto scratch = streamScratchBytes( 4 );
    const u64 partial[] = { pointers - 1, pointers + leases - 1, pointers + leases + sizes - 1,
                            pointers + leases + sizes + scratch - 1 };
    for( const auto headroom : partial )
    {
        budget.set( resident + headroom );
        BOOST_CHECK_THROW( vertices->createVertexData( 3 ), std::bad_alloc );
        BOOST_CHECK( vertices->shape() == shape );
        BOOST_CHECK( vertices->lease( 0 ) == lease );
        BOOST_CHECK( vertices->getVertexData() == positions );
        BOOST_CHECK( !vertices->getVertexData( 3 ) );
        BOOST_CHECK_EQUAL( positions[0], 19.f );
        BOOST_CHECK_EQUAL( vertices->getNumVertices(), 3u );
        BOOST_CHECK( vertices->getVertexDeclaration() == declaration );
        BOOST_CHECK_EQUAL( budget.used(), resident );
    }
    const auto growth = pointers + leases + sizes;
    budget.set( resident + growth + scratch + 23 );
    BOOST_CHECK_THROW( vertices->createVertexData( 3 ), std::runtime_error );
    BOOST_CHECK( vertices->shape() == shape );
    BOOST_CHECK( vertices->lease( 0 ) == lease );
    BOOST_CHECK_EQUAL( budget.used(), resident );
    budget.set( resident + growth + scratch + 24 );
    BOOST_REQUIRE( vertices->createVertexData( 3 ) );
    BOOST_CHECK_EQUAL( budget.used(), vertexResidency( 4, 60 ) );
    BOOST_CHECK_EQUAL( vertices->shape()[0], 4u );
    BOOST_CHECK_EQUAL( vertices->shape()[2], 4u );
    BOOST_CHECK_EQUAL( vertices->shape()[4], 4u );
    BOOST_CHECK( vertices->getVertexData() == positions );
    BOOST_CHECK( vertices->lease( 0 ) == lease );
    budget.set( budget.used() );
    BOOST_CHECK_NO_THROW( vertices->unload( nullptr ) );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( declaration_growth_refusal_keeps_elements_and_retries_without_partial_publication )
{
    TestGuard guard; warmMeshFactories(); MeshBudgetWindow budget( declarationBytes() );
    auto declaration = make_ptr<VertexDeclaration>();
    for( u32 i = 0; i < 4; ++i )
        declaration->addElement( 0, i * sizeof( f32 ), VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                 VertexElementType::VET_FLOAT1, i );
    const auto first = declaration->getElements()[0];
    BOOST_CHECK_THROW( declaration->addElement( 0, 16, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                               VertexElementType::VET_FLOAT1, 4 ), std::bad_alloc );
    BOOST_CHECK_EQUAL( declaration->getElements().size(), 4u );
    BOOST_CHECK_EQUAL( declaration->getElements().capacity(), 4u );
    BOOST_CHECK( declaration->getElements()[0] == first );
    BOOST_CHECK_EQUAL( declaration->getSize( 0 ), 16u );
    BOOST_CHECK_EQUAL( budget.used(), declarationBytes() );
    budget.set( declarationBytes() + declarationBytes( 8 ) );
    BOOST_REQUIRE( declaration->addElement( 0, 16, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                          VertexElementType::VET_FLOAT1, 4 ) );
    BOOST_CHECK_EQUAL( declaration->getElements().size(), 5u );
    BOOST_CHECK_EQUAL( declaration->getElements().capacity(), 8u );
    BOOST_CHECK_EQUAL( budget.used(), declarationBytes( 8 ) );
    budget.set( budget.used() );
    BOOST_CHECK_NO_THROW( declaration->unload( nullptr ) );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( escaped_mesh_metadata_copies_keep_spare_capacity_charged_after_unload )
{
    TestGuard guard; warmMeshFactories();
    const auto owner = declarationBytes( 13 ) + storageBytes<u8 *>( 9 ) +
                       storageBytes<StreamBudget>( 7 ) + storageBytes<u64>( 11 ) + 36;
    const auto escaped = storageBytes<u8 *>( 23 ) + declarationBytes( 37 ) + declarationBytes( 29 );
    MeshBudgetWindow budget( owner + escaped + triangleBuildPeak() + declarationBytes( 13 ) );
    auto declaration = make_ptr<VertexDeclaration>();
    declaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3 );
    declaration->getElements().reserve( 13 );
    auto vertices = make_ptr<VertexBufferProbe>();
    vertices->setVertexDeclaration( declaration ); vertices->setNumVertices( 3 );
    auto positions = vertices->createVertexData();
    vertices->reserveMetadata( 9, 7, 11 );
    BOOST_CHECK_EQUAL( budget.used(), owner );
    budget.set( owner );
    BOOST_CHECK_THROW( vertices->getDataArray(), std::bad_alloc );
    BOOST_CHECK_THROW( declaration->findElementsBySource( 0 ), std::bad_alloc );
    BOOST_CHECK_THROW( Array<SmartPtr<IVertexElement>>( declaration->getElements() ), std::bad_alloc );
    BOOST_CHECK_EQUAL( budget.used(), owner );
    budget.set( owner + escaped + storageBytes<u8 *>( 9 ) + declarationBytes( 13 ) + declarationBytes( 1 ) );
    auto pointers = vertices->getDataArray();
    auto elements = declaration->getElements();
    auto filtered = declaration->findElementsBySource( 0 );
    BOOST_CHECK( pointers.get_allocator().isSourceBudgeted() );
    BOOST_CHECK( elements.get_allocator().isSourceBudgeted() );
    BOOST_CHECK( filtered.get_allocator().isSourceBudgeted() );
    BOOST_CHECK_EQUAL( pointers.capacity(), 9u );
    BOOST_CHECK_EQUAL( elements.capacity(), 13u );
    BOOST_CHECK_EQUAL( filtered.capacity(), 1u );
    BOOST_CHECK( pointers[0] == positions );
    pointers.reserve( 23 ); elements.reserve( 37 ); filtered.reserve( 29 );
    BOOST_CHECK_EQUAL( budget.used(), owner + escaped );
    budget.set( budget.used() );
    BOOST_CHECK_NO_THROW( vertices->unload( nullptr ) );
    BOOST_CHECK_EQUAL( budget.used(), escaped );
    BOOST_CHECK( declaration->getElements().empty() );
    BOOST_CHECK_EQUAL( pointers.size(), 1u );
    BOOST_CHECK_EQUAL( elements.size(), 1u );
    BOOST_CHECK_EQUAL( filtered.size(), 1u );
    // Pointer snapshots retain metadata only; their payload pointer is borrowed.
    pointers = makeSourceBudgetSnapshot<u8 *>();
    BOOST_CHECK_EQUAL( budget.used(), declarationBytes( 37 ) + declarationBytes( 29 ) );
    elements = makeSourceBudgetSnapshot<SmartPtr<IVertexElement>>();
    filtered = makeSourceBudgetSnapshot<SmartPtr<IVertexElement>>();
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( admission_counts_real_allocations_and_preserves_failed_replacements )
{
    TestGuard guard; warmMeshFactories(); MeshBudgetWindow budget( triangleBuildPeak( true ) + 12 );
    auto vertices = triangleVertices( true );
    auto indices = make_ptr<IndexBuffer>(); indices->setNumIndices( 3 );
    auto indexData = static_cast<u32 *>( indices->createIndexData() ); indexData[0] = 17;
    auto positions = vertices->getVertexData(), uv = vertices->getVertexData( 1 );
    const auto declaration = vertices->getVertexDeclaration();
    const auto resident = vertexResidency( 2, 60 ) + 12;
    BOOST_CHECK_EQUAL( budget.used(), resident );
    auto alias = vertices;
    BOOST_CHECK_EQUAL( budget.used(), resident );
    budget.set( resident );

    indices->setNumIndices( 60 ); indices->setIndexType( IIndexBuffer::Type::IT_16BIT );
    BOOST_CHECK_THROW( indices->createIndexData(), std::runtime_error );
    BOOST_CHECK( indices->getIndexData() == indexData );
    BOOST_CHECK_EQUAL( indexData[0], 17u );
    BOOST_CHECK_EQUAL( indices->getNumIndices(), 3u );
    BOOST_CHECK_EQUAL( indices->getIndexSize(), 4u );
    BOOST_CHECK( indices->getIndexType() == IIndexBuffer::Type::IT_32BIT );

    budget.set( resident + declarationBytes() );
    auto replacementDeclaration = make_ptr<VertexDeclaration>();
    replacementDeclaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION,
                                       VertexElementType::VET_FLOAT4 );
    vertices->setNumVertices( std::numeric_limits<u32>::max() );
    vertices->setVertexDeclaration( replacementDeclaration );
    // Admit scratch, then refuse the enormous geometry reservation.
    budget.set( resident + declarationBytes() + streamScratchBytes( 2 ) );
    BOOST_CHECK_THROW( vertices->createVertexData(), std::runtime_error );
    BOOST_CHECK_EQUAL( vertices->getNumVertices(), 3u );
    BOOST_CHECK( vertices->getVertexDeclaration() == declaration );
    BOOST_CHECK( vertices->getVertexData() == positions );
    BOOST_CHECK( vertices->getVertexData( 1 ) == uv );
    replacementDeclaration = nullptr;
    BOOST_CHECK_EQUAL( budget.used(), resident );
    BOOST_CHECK( !vertices->getVertexData( 65536 ) );
    budget.set( resident );
    vertices->unload( nullptr ); indices->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( multistream_growth_commits_together_and_clones_reserve_distinct_storage )
{
    TestGuard guard;
    warmMeshFactories();
    MeshBudgetWindow budget( triangleBuildPeak( true ) );
    auto vertices = triangleVertices( true );
    auto positions = vertices->getVertexData(), uv = vertices->getVertexData( 1 );
    const auto original = vertexResidency( 2, 60 );
    // Metadata is retained once per buffer, while replacement scratch and both
    // geometry generations coexist. Admit 72 bytes, then deny the 48-byte UVs.
    budget.set( original + streamScratchBytes( 2 ) + 72 + 47 );
    vertices->setNumVertices( 6 );
    // The first replacement fits; the second stream exhausts admission.
    BOOST_CHECK_THROW( vertices->createVertexData(), std::runtime_error );
    BOOST_CHECK_EQUAL( vertices->getNumVertices(), 3u );
    BOOST_CHECK( vertices->getVertexData() == positions );
    BOOST_CHECK( vertices->getVertexData( 1 ) == uv );
    BOOST_CHECK_EQUAL( budget.used(), original );
    budget.set( original + streamScratchBytes( 2 ) + 120 );
    vertices->setNumVertices( 6 ); vertices->createVertexData();
    const auto grown = vertexResidency( 2, 120 );
    BOOST_CHECK_EQUAL( budget.used(), grown );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( vertices->getVertexData( 1 ) )[5], 5.f );
    const auto cloneGrowth = declarationBytes() + streamMetadataBytes( 1 ) + 72 +
                             streamMetadataBytes( 2 ) + streamScratchBytes( 2 );
    budget.set( grown + cloneGrowth + 47 );
    BOOST_CHECK_THROW( vertices->clone(), std::runtime_error );
    BOOST_CHECK_EQUAL( budget.used(), grown );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( vertices->getVertexData( 1 ) )[5], 5.f );
    budget.set( grown + cloneGrowth + 48 );
    auto clone = vertices->clone();
    BOOST_CHECK_EQUAL( budget.used(), grown * 2 );
    BOOST_CHECK( clone->getVertexData() != vertices->getVertexData() );
    BOOST_CHECK( clone->getVertexData( 1 ) != vertices->getVertexData( 1 ) );
    BOOST_CHECK_EQUAL( static_cast<f32 *>( clone->getVertexData( 1 ) )[5], 5.f );
    budget.set( budget.used() );
    clone = nullptr; vertices = nullptr;
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( resource_retirement_keeps_direct_buffer_pins_charged )
{
    TestGuard guard; warmMeshFactories(); MeshBudgetWindow budget( triangleBuildPeak() + 96 );
    auto manager = make_ptr<MeshManager>();
    auto resource = dynamic_pointer_cast<IMeshResource>( manager->createOrRetrieve(
        "Cache/cpu-budget-pins.fbmeshbin" ).first );
    BOOST_REQUIRE( resource );
    const auto inventory = budget.used();
    BOOST_CHECK_GT( inventory, 0u );
    budget.set( inventory + triangleBuildPeak() + 12 );
    auto geometry = triangleMesh();
    auto submesh = geometry->getSubMesh( 0 );
    auto vertices = submesh->getVertexBuffer(); auto indices = submesh->getIndexBuffer();
    resource->setMesh( geometry ); resource->setLoadingState( LoadingState::Loaded );
    const auto meshResident = vertexResidency( 1, 36 ) + 12;
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    BOOST_CHECK( !manager->releaseUnusedResource( resource ) );
    geometry = nullptr; submesh = nullptr;
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
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
    TestGuard guard; warmMeshFactories();
    // FileSystem opens cooked files through FileDataStream and refreshPath uses
    // JobFunction. Keep both retained factory inventories in the baseline.
    { auto stream = guard.factoryManager->make_ptr<FileDataStream>(); }
    {
        auto factory = guard.factoryManager->getFactoryByName( "workphone::JobFunction" );
        BOOST_REQUIRE( factory );
        auto job = factory->make_object<ISharedObject>();
    }
    MeshBudgetWindow budget( triangleBuildPeak() + 12 );
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
    const auto meshResident = vertexResidency( 1, 36 ) + 12;
    // Cooked import reads indices first. Stream scratch and the filtered
    // declaration snapshot are transient, so they need headroom beyond resident
    // geometry and all three stream inventories.
    const auto importScratch = std::max( streamScratchBytes( 1 ), declarationBytes( 1 ) );
    const auto importPeak = meshResident + importScratch;
    budget.set( inventory + importPeak );
    resource->load( nullptr );
    BOOST_REQUIRE( resource->isLoaded() );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    const auto aliasPath = Path::getRelativePath( guard.applicationManager->getProjectPath(), file.c_str() );
    BOOST_CHECK_MESSAGE( manager->createOrRetrieve( aliasPath ).first == resource,
                         "Source aliases must share one resource: " << file << " and " << aliasPath );
    auto old = manager->loadMesh( file.c_str() );
    BOOST_REQUIRE( old ); BOOST_CHECK( old == resource->getMesh() );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );

    // Staging needs capacity for both generations. Refusal keeps the original
    // loaded mesh and its geometry, then a larger budget permits a real reload.
    budget.set( inventory + meshResident + importPeak - 1 );
    resource->reload( nullptr );
    BOOST_CHECK( resource->isLoaded() ); BOOST_CHECK( resource->getMesh() == old );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    budget.set( inventory + meshResident + importPeak ); resource->reload( nullptr );
    BOOST_REQUIRE( resource->isLoaded() ); BOOST_CHECK( resource->getMesh() != old );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident * 2 );
    BOOST_CHECK_EQUAL( old->getSubMesh( 0 )->getVertexBuffer()->getNumVertices(), 3u );
    old = nullptr; BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    const auto lastGood = resource->getMesh().get();
    { std::ofstream corrupt( file, std::ios::binary | std::ios::trunc ); corrupt << "invalid mesh"; }
    resource->reload( nullptr );
    BOOST_CHECK( resource->isLoaded() ); BOOST_CHECK( resource->getMesh().get() == lastGood );
    BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory );
    { std::ofstream restored( file, std::ios::binary | std::ios::trunc ); restored.write( original.data(), original.size() ); }
    auto reloaded = manager->loadMesh( aliasPath );
    BOOST_REQUIRE( reloaded ); BOOST_CHECK_EQUAL( budget.used(), inventory + meshResident );
    BOOST_CHECK( reloaded == resource->getMesh() );
    reloaded = nullptr;
    BOOST_REQUIRE( manager->releaseUnusedResource( resource ) );
    BOOST_CHECK_EQUAL( budget.used(), inventory );
    manager->unload( nullptr );
    BOOST_CHECK_EQUAL( budget.used(), 0u );
}

BOOST_AUTO_TEST_CASE( serializer_staging_obeys_source_admission_and_unwinds )
{
    TestGuard guard;
    mesh_serializer_staging_budget_contracts::run();
}

BOOST_AUTO_TEST_SUITE_END()
