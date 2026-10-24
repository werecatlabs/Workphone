#pragma once

#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <cstring>
#include <stdexcept>

namespace mesh_serializer_staging_budget_contracts
{
    using namespace workphone;

    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 bytes )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ),
                     "serializer staging capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };

    // A caller-owned fixed buffer keeps the probe itself outside the engine's
    // SourceCpu ownership. Admission is tightened at the actual payload boundary,
    // after unrelated serializer headers and imported geometry have been created.
    class ProbeStream : public MemoryFile
    {
    public:
        ProbeStream( void *bytes, long size ) : MemoryFile( bytes, size, "probe", false ) {}
        bool isWriteable() const override { return true; }

        size_Num write( const void *buffer, size_Num count ) override
        {
            const auto bytes = static_cast<const u8 *>( buffer );
            if( watchGeometry && !armed && count == 2 && bytes[0] == 0x52 && bytes[1] == 0x10 )
                // The primitive chunk-id endian copy is released when this
                // write returns, before the vertex buffer is staged.
                arm( sourceBudgetStorageBytes( sizeof( u16 ), alignof( u16 ) ) );
            if( watchGeometry && armed && count == 9 * sizeof( float ) )
            {
                payloadPosition = tell();
                usedAtPayload = budget->used();
                if( failPayload ) throw std::runtime_error( "injected mesh payload write failure" );
            }
            return MemoryFile::write( buffer, count );
        }

        size_Num read( void *buffer, size_Num count ) override
        {
            if( readGate && tell() == readGate && count == sizeof( u16 ) ) arm();
            if( readGate && tell() == readGate + sizeof( u16 ) && count == 6 * sizeof( float ) )
            {
                payloadPosition = tell();
                usedAtPayload = budget->used();
                if( failPayload ) throw std::runtime_error( "injected mesh extrema read failure" );
            }
            return MemoryFile::read( buffer, count );
        }

        void arm( u64 retiring = 0 )
        {
            armed = true;
            usedAtHeader = budget->used() - retiring;
            if( constrain ) budget->set( usedAtHeader + headroom );
        }

        Capacity *budget = nullptr;
        bool watchGeometry = false, constrain = false, armed = false, failPayload = false;
        u64 headroom = 0, usedAtHeader = 0, usedAtPayload = 0;
        size_Num payloadPosition = 0, readGate = 0;
    };

    inline SmartPtr<Mesh> triangle()
    {
        auto declaration = make_ptr<VertexDeclaration>();
        declaration->addElement( 0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3 );
        auto vertices = make_ptr<VertexBuffer>();
        vertices->setVertexDeclaration( declaration );
        vertices->setNumVertices( 3 );
        const float positions[] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
        std::memcpy( vertices->createVertexData(), positions, sizeof( positions ) );
        auto indices = make_ptr<IndexBuffer>();
        indices->setNumIndices( 3 );
        const u32 faces[] = { 0, 1, 2 };
        std::memcpy( indices->createIndexData(), faces, sizeof( faces ) );
        auto submesh = make_ptr<SubMesh>();
        submesh->setVertexBuffer( vertices );
        submesh->setIndexBuffer( indices );
        submesh->setMaterialName( "probe" );
        auto mesh = make_ptr<Mesh>();
        mesh->addSubMesh( submesh );
        mesh->updateAABB();
        mesh->setBoundingSphereRadius( 1 );
        return mesh;
    }

    template<class T> inline void append( std::array<u8, 8192> &bytes, size_t &offset, const T &value )
    {
        require( offset + sizeof( T ) <= bytes.size(), "mesh staging fixture fits its fixed buffer" );
        std::memcpy( bytes.data() + offset, &value, sizeof( T ) );
        offset += sizeof( T );
    }

    class PrimitiveSerializer : public Serializer
    {
    public:
        using Serializer::readFloats;
        using Serializer::writeFloats;
        using Serializer::writeShorts;
        using Serializer::writeInts;
        void bind( const SmartPtr<workphone::IStream> &stream, bool flip )
        { m_stream = stream; m_flipEndian = flip; }
    };

    class PrimitiveStream : public MemoryFile
    {
    public:
        PrimitiveStream( void *bytes, long count ) : MemoryFile( bytes, count, "primitive", false ) {}
        bool isWriteable() const override { return true; }
        size_Num write( const void *bytes, size_Num count ) override
        {
            ++writes; duringIo = budget->used();
            if( fail ) throw std::runtime_error( "injected primitive write failure" );
            return MemoryFile::write( bytes, count );
        }
        size_Num read( void *bytes, size_Num count ) override
        {
            ++reads; duringIo = budget->used();
            if( fail ) throw std::runtime_error( "injected primitive read failure" );
            return MemoryFile::read( bytes, count );
        }
        Capacity *budget = nullptr;
        u64 duringIo = 0;
        size_t reads = 0, writes = 0;
        bool fail = false;
    };

    inline u16 field16( const u8 *bytes, bool big )
    {
        return big ? u16( u16( bytes[0] ) << 8 | bytes[1] ) :
                     u16( u16( bytes[1] ) << 8 | bytes[0] );
    }

    inline u32 field32( const u8 *bytes, bool big )
    {
        u32 value = 0;
        for( size_t byte = 0; byte < 4; ++byte )
            value |= u32( bytes[byte] ) << ( big ? 24 - byte * 8 : byte * 8 );
        return value;
    }

    class GeometryStream : public MemoryFile
    {
    public:
        GeometryStream( void *bytes, long count, bool big ) :
            MemoryFile( bytes, count, "geometry", false ), big( big ) {}
        bool isWriteable() const override { return true; }
        size_Num write( const void *bytes, size_Num count ) override
        {
            if( count == sizeof( u16 ) )
            {
                const auto field = field16( static_cast<const u8 *>( bytes ), big );
                if( field == 0x5000 ) geometry = tell();
                if( geometry && field == 0x5200 )
                {
                    require( streams < headers.size(), "multistream fixture stream count is bounded" );
                    headers[streams++] = tell();
                }
                if( geometry && field == 0x4010 ) geometryEnd = tell();
            }
            return MemoryFile::write( bytes, count );
        }
        bool big;
        size_Num geometry = 0, geometryEnd = 0;
        size_t streams = 0;
        std::array<size_Num, 4> headers{};
    };

    inline void multistream()
    {
        Capacity budget;
        for( const u16 secondSource : { u16( 1 ), u16( 3 ) } )
        {
            auto mesh = triangle();
            auto vertices = mesh->getSubMesh( 0 )->getVertexBuffer();
            auto declaration = vertices->getVertexDeclaration();
            declaration->addElement( secondSource, 0, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                     VertexElementType::VET_FLOAT2 );
            const float uvs[] = { 0.125f, 0.25f, 0.5f, 0.75f, 1.f, 1.25f };
            std::memcpy( vertices->createVertexData( secondSource ), uvs, sizeof( uvs ) );
            auto positionData = vertices->getVertexData();
            auto uvData = vertices->getVertexData( secondSource );
            std::array<u8, 9 * sizeof( float )> positions{};
            std::memcpy( positions.data(), positionData, positions.size() );
            MeshSerializer serializer;
            // First pass warms factory inventories and serializer/logging strings;
            // the second checks complete scratch retirement around each operation.
            for( int pass = 0; pass < 2; ++pass ) for( bool big : { false, true } )
            {
                std::array<u8, 8192> bytes{};
                auto output = make_ptr<GeometryStream>( bytes.data(), long( bytes.size() ), big );
                const auto resident = budget.used();
                serializer.exportMesh( mesh.get(), output, big ? Endian::ENDIAN_BIG : Endian::ENDIAN_NATIVE );
                if( pass ) require( budget.used() == resident,
                                    "multistream export retires every metadata snapshot and endian scratch buffer" );
                require( vertices->getVertexData() == positionData &&
                             vertices->getVertexData( secondSource ) == uvData &&
                             std::memcmp( positionData, positions.data(), positions.size() ) == 0 &&
                             std::memcmp( uvData, uvs, sizeof( uvs ) ) == 0,
                         "native and endian export preserve each source pointer and its actual payload" );
                require( output->streams == 2 && output->geometry && output->geometryEnd &&
                             output->geometry + field32( bytes.data() + output->geometry + 2, big ) == output->geometryEnd,
                         "geometry parent length covers exactly the present stream chunks" );
                for( size_t streamIndex = 0; streamIndex < 2; ++streamIndex )
                {
                    const auto header = output->headers[streamIndex];
                    const auto source = streamIndex ? secondSource : u16( 0 );
                    const auto stride = streamIndex ? 8u : 12u;
                    const auto payload = 3 * stride;
                    const auto end = streamIndex ? output->geometryEnd : output->headers[1];
                    require( field16( bytes.data() + header + 6, big ) == source &&
                                 field16( bytes.data() + header + 8, big ) == stride &&
                                 field16( bytes.data() + header + 10, big ) == 0x5210 &&
                                 field32( bytes.data() + header + 12, big ) == 6 + payload &&
                                 field32( bytes.data() + header + 2, big ) == 16 + payload &&
                                 header + 16 + payload == end,
                             "stream binding, stride, nested data size and actual extent agree for unequal source lengths" );
                }
                auto input = make_ptr<MemoryFile>( bytes.data(), long( output->tell() ), "roundtrip", false );
                SmartPtr<workphone::IStream> stream = input;
                auto candidate = make_ptr<Mesh>();
                serializer.importMesh( stream, candidate.get() );
                auto imported = candidate->getSubMesh( 0 )->getVertexBuffer();
                require( imported->getVertexData() && imported->getVertexData( secondSource ) &&
                             imported->getNumVertices() == 3 &&
                             imported->getVertexDeclaration()->getSize( 0 ) == 12 &&
                             imported->getVertexDeclaration()->getSize( secondSource ) == 8 &&
                             std::memcmp( imported->getVertexData(), positions.data(), positions.size() ) == 0 &&
                             std::memcmp( imported->getVertexData( secondSource ), uvs, sizeof( uvs ) ) == 0,
                         "native and big endian roundtrips preserve independent position and UV streams" );
                for( u16 hole = 1; hole < secondSource; ++hole )
                    require( !imported->getVertexData( hole ), "sparse source indices preserve absent stream holes" );
                imported = nullptr; candidate = nullptr;
                if( pass ) require( budget.used() == resident,
                                    "multistream import retires its temporary and independently owned geometry storage" );
            }
        }
    }

    inline void primitives()
    {
        Capacity budget;
        std::array<u8, 32> bytes{}, expected{};
        auto io = make_ptr<PrimitiveStream>( bytes.data(), long( bytes.size() ) );
        io->budget = &budget;
        SmartPtr<workphone::IStream> stream = io;
        PrimitiveSerializer serializer;
        const float floats[] = { 1.25f, -2.5f };
        const double doubles[] = { 1.25, -2.5 };
        const u16 shorts[] = { 0x1234, 0x5678 };
        const u32 ints[] = { 0x12345678, 0x87654321 };
        const auto baseline = budget.used();
        auto checkWrite = [&]( auto write, const void *values, size_t valueSize, size_t alignment, bool flip ) {
            const auto count = 2 * valueSize;
            std::memcpy( expected.data(), values, count );
            if( flip ) for( size_t item = 0; item < 2; ++item )
                for( size_t byte = 0; byte < valueSize; ++byte )
                    expected[item * valueSize + byte] = static_cast<const u8 *>(values)[item * valueSize + valueSize - 1 - byte];
            const auto scratch = sourceBudgetStorageBytes( count, alignment );
            serializer.bind( stream, flip );
            io->seek( 0 ); io->writes = 0;
            budget.set( baseline + scratch - 1 );
            bool denied = false;
            try { write(); } catch( const std::bad_alloc & ) { denied = true; }
            budget.restore();
            require( denied && io->writes == 0 && io->tell() == 0 && budget.used() == baseline,
                     "primitive scratch denial occurs before writing or retaining storage" );
            budget.set( baseline + scratch );
            write();
            require( io->duringIo == baseline + scratch && io->tell() == count &&
                         std::memcmp( bytes.data(), expected.data(), count ) == 0 && budget.used() == baseline,
                     "primitive scratch has exact physical admission and preserves converted endian bytes" );
            budget.restore();
            io->seek( 0 ); io->fail = true;
            bool failed = false;
            try { write(); } catch( const std::runtime_error & ) { failed = true; }
            io->fail = false;
            require( failed && budget.used() == baseline,
                     "primitive stream write exceptions release admitted scratch" );
        };
        checkWrite( [&] { serializer.writeFloats( floats, 2 ); }, floats, sizeof( float ), alignof( float ), true );
        checkWrite( [&] { serializer.writeFloats( doubles, 2 ); }, floats, sizeof( float ), alignof( float ), false );
        checkWrite( [&] { serializer.writeShorts( shorts, 2 ); }, shorts, sizeof( u16 ), alignof( u16 ), true );
        checkWrite( [&] { serializer.writeInts( ints, 2 ); }, ints, sizeof( u32 ), alignof( u32 ), true );
        io->seek( 0 ); serializer.bind( stream, false );
        budget.set( baseline );
        serializer.writeFloats( floats, 2 );
        require( io->duringIo == baseline && budget.used() == baseline,
                 "native float writes need no conversion allocation at capacity" );
        budget.restore();
        const auto scratch = sourceBudgetStorageBytes( sizeof( floats ), alignof( float ) );
        double destination[] = { 17, 19 };
        io->seek( 0 ); io->reads = 0;
        budget.set( baseline + scratch - 1 );
        bool denied = false;
        try { serializer.readFloats( stream, destination, 2 ); } catch( const std::bad_alloc & ) { denied = true; }
        budget.restore();
        require( denied && io->reads == 0 && io->tell() == 0 && destination[0] == 17 && destination[1] == 19,
                 "double conversion denial preserves stream position and destination" );
        io->fail = true;
        bool failedRead = false;
        try { serializer.readFloats( stream, destination, 2 ); } catch( const std::runtime_error & ) { failedRead = true; }
        io->fail = false;
        require( failedRead && budget.used() == baseline && destination[0] == 17 && destination[1] == 19,
                 "double conversion read exceptions release scratch without publishing values" );
        budget.set( baseline + scratch );
        serializer.readFloats( stream, destination, 2 );
        require( destination[0] == doubles[0] && destination[1] == doubles[1] &&
                     io->duringIo == baseline + scratch && budget.used() == baseline,
                 "double conversion retries with one exact scratch allocation" );
    }

    inline void run()
    {
        primitives();
        multistream();
        Capacity budget;
        auto mesh = triangle();
        auto vertices = mesh->getSubMesh( 0 )->getVertexBuffer();
        std::array<u8, 9 * sizeof( float )> original{};
        std::memcpy( original.data(), vertices->getVertexData(), original.size() );
        MeshSerializer serializer;
        std::array<u8, 8192> bytes{};
        auto output = make_ptr<ProbeStream>( bytes.data(), long( bytes.size() ) );
        output->budget = &budget;
        output->watchGeometry = true;

        // Warm the same serializer, logging strings and factory inventories before
        // checking exact temporary allocation lifetimes.
        serializer.exportMesh( mesh.get(), output, Endian::ENDIAN_BIG );
        const auto exportedSize = output->tell();
        require( output->payloadPosition != 0, "big endian export reaches vertex staging" );
        const auto scratch = sourceBudgetStorageBytes( original.size(), alignof( u8 ) );
        require( output->usedAtPayload == output->usedAtHeader + scratch,
                 "endian vertex staging admits its physical buffer until the payload write" );
        for( size_t offset = 0; offset < original.size(); ++offset )
            require( bytes[output->payloadPosition + offset] == original[( offset / 4 ) * 4 + 3 - offset % 4],
                     "admitted endian staging preserves the serialized float bytes" );

        const auto baseline = budget.used();
        for( auto headroom : { scratch - 1, scratch } )
        {
            output->seek( 0 ); output->armed = false; output->constrain = true;
            output->headroom = headroom; output->payloadPosition = 0;
            bool denied = false;
            try { serializer.exportMesh( mesh.get(), output, Endian::ENDIAN_BIG ); }
            catch( const std::bad_alloc & ) { denied = true; }
            budget.restore();
            require( denied && output->armed && output->payloadPosition == 0 && budget.used() == baseline,
                     "endian scratch or later declaration-snapshot denial retires all staged storage" );
            require( std::memcmp( original.data(), vertices->getVertexData(), original.size() ) == 0,
                     "failed endian export preserves resident vertex storage" );
        }
        output->seek( 0 ); output->armed = false; output->constrain = false; output->failPayload = true;
        bool failedWrite = false;
        try { serializer.exportMesh( mesh.get(), output, Endian::ENDIAN_BIG ); }
        catch( const std::runtime_error & ) { failedWrite = true; }
        require( failedWrite && budget.used() == baseline,
                 "stream write exceptions release admitted endian staging" );
        output->failPayload = false;
        output->seek( 0 ); output->armed = false;
        serializer.exportMesh( mesh.get(), output, Endian::ENDIAN_BIG );
        require( output->tell() == exportedSize && budget.used() == baseline,
                 "endian export retries without retaining temporary storage" );

        // Extend a native export with the optional extrema chunk. The stream's
        // read gate is its submesh index, immediately before scratch admission.
        output->watchGeometry = false; output->seek( 0 );
        serializer.exportMesh( mesh.get(), output, Endian::ENDIAN_NATIVE );
        size_t length = output->tell();
        const auto extremesOffset = length;
        append( bytes, length, u16( 0xE000 ) );
        append( bytes, length, u32( 6 + sizeof( u16 ) + 6 * sizeof( float ) ) );
        const auto readGate = length;
        append( bytes, length, u16( 0 ) );
        for( const auto value : { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f } ) append( bytes, length, value );
        const auto meshChunkOffset = size_t( 2 + std::strlen( "[MeshSerializer_v1.8]" ) + 1 );
        u32 meshChunkSize = 0;
        std::memcpy( &meshChunkSize, bytes.data() + meshChunkOffset + 2, sizeof( meshChunkSize ) );
        meshChunkSize += u32( length - extremesOffset );
        std::memcpy( bytes.data() + meshChunkOffset + 2, &meshChunkSize, sizeof( meshChunkSize ) );
        auto input = make_ptr<ProbeStream>( bytes.data(), long( length ) );
        input->budget = &budget; input->readGate = readGate;
        SmartPtr<workphone::IStream> stream = input;
        { auto warm = make_ptr<Mesh>(); serializer.importMesh( stream, warm.get() ); }
        const auto importBaseline = budget.used();
        const auto submeshes = sourceBudgetStorageBytes( sizeof( SmartPtr<ISubMesh> ),
                                                         alignof( SmartPtr<ISubMesh> ) );
        const auto floats = sourceBudgetStorageBytes( 6 * sizeof( float ), alignof( float ) );
        const auto vectors = sourceBudgetStorageBytes( 2 * sizeof( Vector3F ), alignof( Vector3F ) );
        // The index read arms the limit before readExtremes retains its submesh
        // snapshot. That owner overlaps both independently admitted payloads.
        for( auto headroom : { submeshes - 1, submeshes + floats - 1,
                              submeshes + floats + vectors - 1 } )
        {
            input->seek( 0 ); input->armed = false; input->constrain = true;
            input->headroom = headroom; input->payloadPosition = 0;
            auto candidate = make_ptr<Mesh>();
            bool denied = false;
            try { serializer.importMesh( stream, candidate.get() ); }
            catch( const std::bad_alloc & ) { denied = true; }
            budget.restore();
            const auto beforePayload = headroom < submeshes + floats;
            require( denied && input->armed && ( beforePayload ? input->payloadPosition == 0 :
                         input->payloadPosition != 0 &&
                         input->usedAtPayload == input->usedAtHeader + submeshes + floats ),
                     "extrema snapshot, float input and vector staging require independent physical admission" );
            candidate = nullptr;
            require( budget.used() == importBaseline,
                     "failed extrema import retires partial mesh and both staging buffers" );
        }
        input->seek( 0 ); input->armed = false; input->constrain = false; input->failPayload = true;
        {
            auto candidate = make_ptr<Mesh>();
            bool failedRead = false;
            try { serializer.importMesh( stream, candidate.get() ); }
            catch( const std::runtime_error & ) { failedRead = true; }
            require( failedRead && input->armed, "extrema stream read failure is exercised after admission" );
        }
        require( budget.used() == importBaseline, "extrema read exceptions retire admitted scratch" );
        input->seek( 0 ); input->armed = false; input->failPayload = false;
        {
            auto candidate = make_ptr<Mesh>();
            serializer.importMesh( stream, candidate.get() );
            require( candidate->getSubMesh( 0 )->getVertexBuffer()->getNumVertices() == 3,
                     "extrema staging can retry a complete mesh import" );
        }
        require( budget.used() == importBaseline, "successful extrema import leaves no temporary storage" );
    }
}
