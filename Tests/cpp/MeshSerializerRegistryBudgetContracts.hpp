#pragma once

#include "MeshSerializerStagingBudgetContracts.hpp"
#include <optional>

namespace mesh_serializer_registry_budget_contracts
{
    using namespace workphone;
    using mesh_serializer_staging_budget_contracts::Capacity;
    using mesh_serializer_staging_budget_contracts::require;

    class RegistryProbe : public MeshSerializer
    {
    public:
        template<class T> static u64 bodyBytes()
        {
            // SourceBudgetObject's ordinary new uses max_align_t alignment.
            return sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        }
        static auto allocationBytes() -> std::array<u64, 13>
        {
            const auto descriptor = bodyBytes<MeshVersionData>();
            return {
                sourceBudgetStorageBytes( 6 * sizeof( MeshVersionData * ), alignof( MeshVersionData * ) ),
                bodyBytes<MeshSerializerImpl>(), descriptor,
                bodyBytes<MeshSerializerImpl_v1_41>(), descriptor,
                bodyBytes<MeshSerializerImpl_v1_4>(), descriptor,
                bodyBytes<MeshSerializerImpl_v1_3>(), descriptor,
                bodyBytes<MeshSerializerImpl_v1_2>(), descriptor,
                bodyBytes<MeshSerializerImpl_v1_1>(), descriptor
            };
        }
        static u64 retainedBytes()
        {
            u64 bytes = 0;
            for( auto allocation : allocationBytes() ) bytes += allocation;
            return bytes;
        }
        void check() const
        {
            constexpr MeshVersion versions[] = {
                MESH_VERSION_1_8, MESH_VERSION_1_7, MESH_VERSION_1_4,
                MESH_VERSION_1_0, MESH_VERSION_LEGACY, MESH_VERSION_LEGACY
            };
            const char *labels[] = {
                "[MeshSerializer_v1.8]", "[MeshSerializer_v1.41]", "[MeshSerializer_v1.40]",
                "[MeshSerializer_v1.30]", "[MeshSerializer_v1.20]", "[MeshSerializer_v1.10]"
            };
            require( mVersionData.get_allocator().isSourceBudgeted() &&
                         mVersionData.size() == 6 && mVersionData.capacity() == 6,
                     "serializer registry admits its exact pointer capacity" );
            for( size_t index = 0; index < mVersionData.size(); ++index )
                require( mVersionData[index] && mVersionData[index]->impl &&
                             mVersionData[index]->version == versions[index] &&
                             mVersionData[index]->versionString == labels[index],
                         "serializer versions retain newest-first order and both legacy readers" );
        }
    };

    // Shared by the cooked-mesh fixture: a temporary serializer retains this
    // exact registry footprint while decoded geometry and I/O scratch coexist.
    inline u64 retainedBytes() { return RegistryProbe::retainedBytes(); }

    inline void admission()
    {
        Capacity budget;
        const auto baseline = budget.used();
        const auto resident = retainedBytes();
        u64 prefix = 0;
        for( const auto allocation : RegistryProbe::allocationBytes() )
        {
            budget.set( baseline + prefix + allocation - 1 );
            const auto denials = sourceBudgetDenialCount();
            bool denied = false;
            try { RegistryProbe candidate; } catch( const std::bad_alloc & ) { denied = true; }
            budget.restore();
            require( denied && budget.used() == baseline && sourceBudgetDenialCount() == denials + 1,
                     "each cold registry allocation stage rejects and completely unwinds prior descriptors and implementations" );
            // Retry the complete registry after every rejected stage, then retire
            // it with zero remaining admission capacity.
            std::optional<RegistryProbe> retry;
            budget.set( baseline + resident );
            retry.emplace();
            retry->check();
            require( budget.used() == baseline + resident,
                     "registry bytes equal its pointer buffer and all descriptor/concrete implementation bodies" );
            const auto beforeRetirement = sourceBudgetDenialCount();
            retry.reset();
            require( budget.used() == baseline && sourceBudgetDenialCount() == beforeRetirement,
                     "registry destruction releases physical storage without allocating at capacity" );
            budget.restore();
            prefix += allocation;
        }
        require( prefix == resident, "registry stage totals match its retained physical footprint" );
    }

    inline void roundtrips()
    {
        auto source = mesh_serializer_staging_budget_contracts::triangle();
        auto vertices = source->getSubMesh( 0 )->getVertexBuffer();
        auto positions = vertices->getVertexData();
        std::array<u8, 9 * sizeof( float )> original{};
        std::memcpy( original.data(), positions, original.size() );
        constexpr MeshVersion versions[] = {
            MESH_VERSION_LATEST, MESH_VERSION_1_8, MESH_VERSION_1_7, MESH_VERSION_1_4, MESH_VERSION_1_0
        };
        const char *labels[] = {
            "[MeshSerializer_v1.8]", "[MeshSerializer_v1.8]", "[MeshSerializer_v1.41]",
            "[MeshSerializer_v1.40]", "[MeshSerializer_v1.30]"
        };
        RegistryProbe serializer;
        for( size_t version = 0; version < std::size( versions ); ++version )
            for( const auto endian : { Endian::ENDIAN_NATIVE, Endian::ENDIAN_BIG } )
            {
                std::array<u8, 8192> bytes{};
                auto output = make_ptr<mesh_serializer_staging_budget_contracts::ProbeStream>(
                    bytes.data(), long( bytes.size() ) );
                serializer.exportMesh( source.get(), output, versions[version], endian );
                const auto labelBytes = std::strlen( labels[version] );
                require( std::memcmp( bytes.data() + 2, labels[version], labelBytes ) == 0 &&
                             bytes[2 + labelBytes] == '\n',
                         "explicit format versions select the matching registered writer" );
                auto input = make_ptr<MemoryFile>( bytes.data(), long( output->tell() ), "version", false );
                SmartPtr<workphone::IStream> stream = input;
                auto candidate = make_ptr<Mesh>();
                serializer.importMesh( stream, candidate.get() );
                require( candidate->getNumSubMeshes() == 1, "registered format roundtrip preserves its submesh" );
                auto submesh = candidate->getSubMesh( 0 );
                auto imported = submesh->getVertexBuffer();
                auto indices = submesh->getIndexBuffer();
                require( imported && imported->getVertexData() && imported->getNumVertices() == 3 &&
                             std::memcmp( imported->getVertexData(), original.data(), original.size() ) == 0 &&
                             indices && indices->getNumIndices() == 3 &&
                             static_cast<const u32 *>( indices->getIndexData() )[2] == 2 &&
                             vertices->getVertexData() == positions &&
                             std::memcmp( positions, original.data(), original.size() ) == 0,
                         "current and supported older formats preserve native/endian mesh data and source ownership" );
            }

        // Both historical identifiers still select readers for a valid empty
        // mesh, retaining the existing legacy-export diagnostics and behavior.
        for( const auto label : { "[MeshSerializer_v1.20]", "[MeshSerializer_v1.10]" } )
        {
            std::array<u8, 8192> bytes{};
            size_t offset = 0;
            using mesh_serializer_staging_budget_contracts::append;
            append( bytes, offset, u16( 0x1000 ) );
            const auto labelBytes = std::strlen( label );
            std::memcpy( bytes.data() + offset, label, labelBytes );
            offset += labelBytes;
            append( bytes, offset, char( '\n' ) );
            append( bytes, offset, u16( 0x3000 ) );
            append( bytes, offset, u32( 7 ) );
            append( bytes, offset, bool( false ) );
            auto input = make_ptr<MemoryFile>( bytes.data(), long( offset ), "legacy", false );
            SmartPtr<workphone::IStream> stream = input;
            auto candidate = make_ptr<Mesh>();
            serializer.importMesh( stream, candidate.get() );
            require( candidate->getNumSubMeshes() == 0,
                     "both registered legacy reader labels remain available" );
        }
    }

    inline void run()
    {
        admission();
        roundtrips();
    }
}
