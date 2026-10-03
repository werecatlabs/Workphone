#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Mesh/MeshFileFormat.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <fstream>

namespace workphone
{
    const unsigned short HEADER_CHUNK_ID = 0x1000;

    MeshSerializer::MeshSerializer() : mListener( nullptr )
    {
        // Init implementations
        // String identifiers have not always been 100% unified with OGRE version

        // Note MUST be added in reverse order so latest is first in the list
        mVersionData.push_back(
            new MeshVersionData( MESH_VERSION_1_8, "[MeshSerializer_v1.8]", new MeshSerializerImpl() ) );

        mVersionData.push_back( new MeshVersionData( MESH_VERSION_1_7, "[MeshSerializer_v1.41]",
                                                     new MeshSerializerImpl_v1_41() ) );

        mVersionData.push_back( new MeshVersionData( MESH_VERSION_1_4, "[MeshSerializer_v1.40]",
                                                     new MeshSerializerImpl_v1_4() ) );

        mVersionData.push_back( new MeshVersionData( MESH_VERSION_1_0, "[MeshSerializer_v1.30]",
                                                     new MeshSerializerImpl_v1_3() ) );
        mVersionData.push_back( new MeshVersionData( MESH_VERSION_LEGACY, "[MeshSerializer_v1.20]",
                                                     new MeshSerializerImpl_v1_2() ) );

        mVersionData.push_back( new MeshVersionData( MESH_VERSION_LEGACY, "[MeshSerializer_v1.10]",
                                                     new MeshSerializerImpl_v1_1() ) );
    }

    MeshSerializer::~MeshSerializer()
    {
        // delete map
        for( auto &i : mVersionData )
        {
            delete i;
        }
        mVersionData.clear();
    }

    void MeshSerializer::exportMesh( const Mesh *pMesh, const String &filename, Endian endianMode )
    {
        auto f = new std::fstream;
        f->open( filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc );

        auto stream = workphone::make_ptr<FileDataStream>( filename, f, 0, true );
        exportMesh( pMesh, stream, endianMode );
        stream->close();
    }

    void MeshSerializer::exportMesh( const Mesh *pMesh, const String &filename, MeshVersion version,
                                     Endian endianMode )
    {
        auto f = new std::fstream;
        f->open( filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc );

        auto stream = workphone::make_ptr<FileDataStream>( filename, f, 0, true );
        exportMesh( pMesh, stream, version, endianMode );
        stream->close();
    }

    void MeshSerializer::exportMesh( const Mesh *pMesh, SmartPtr<IStream> stream, Endian endianMode )
    {
        exportMesh( pMesh, stream, MESH_VERSION_LATEST, endianMode );
    }

    void MeshSerializer::exportMesh( const Mesh *pMesh, SmartPtr<IStream> stream, MeshVersion version,
                                     Endian endianMode )
    {
        if( version == MESH_VERSION_LEGACY )
        {
            WP_LOG_ERROR(
                "MeshSerializer::exportMesh: You may not supply a legacy version number (pre v1.0) for "
                "writing meshes." );
        }

        MeshSerializerImpl *impl = nullptr;
        if( version == MESH_VERSION_LATEST )
        {
            impl = mVersionData[0]->impl;
        }
        else
        {
            for( auto &i : mVersionData )
            {
                if( version == i->version )
                {
                    impl = i->impl;
                    break;
                }
            }
        }

        if( !impl )
        {
            WP_LOG_ERROR( "MeshSerializer::exportMesh: Cannot find serializer implementation" );
        }

        impl->exportMesh( pMesh, stream, endianMode );
    }

    void MeshSerializer::importMesh( SmartPtr<IStream> &stream, Mesh *pDest )
    {
        determineEndianness( stream );

        // Jump back to start
        stream->seek( 0 );

        // Read header and determine the version
        unsigned short headerID;

        // Read header ID
        readShorts( stream, &headerID, 1 );

        if( headerID != HEADER_CHUNK_ID )
        {
            WP_EXCEPTION( "File header not found" );
        }

        // Read version
        String ver = readString( stream );
        // Jump back to start
        stream->seek( 0 );

        // Find the implementation to use
        MeshSerializerImpl *impl = nullptr;
        for( auto &i : mVersionData )
        {
            if( i->versionString == ver )
            {
                impl = i->impl;
                break;
            }
        }
        if( !impl )
        {
            WP_EXCEPTION(
                ( String( "Cannot find serializer implementation for mesh version " ) + ver ).c_str() );
        }

        // Call implementation
        impl->importMesh( stream, pDest, mListener );
        // Warn on old version of mesh
        if( ver != mVersionData[0]->versionString )
        {
            WP_LOG_INFO( String( "WARNING: " ) + pDest->getName() + String( " is an older format (" ) +
                         ver + String( "); you should upgrade it as soon as possible" ) +
                         String( " using the OgreMeshUpgrade tool." ) );
        }
    }

    auto MeshSerializer::loadMesh( SmartPtr<IStream> &stream ) -> SmartPtr<IMesh>
    {
        auto pDest = workphone::make_ptr<Mesh>();

        // Read header and determine the version
        unsigned short headerID;

        // Read header ID
        readShorts( stream, &headerID, 1 );

        if( headerID != HEADER_CHUNK_ID )
        {
            WP_EXCEPTION( "File header not found" );
        }

        // Read version
        String ver = readString( stream );
        // Jump back to start
        stream->seek( 0 );

        // Find the implementation to use
        MeshSerializerImpl *impl = nullptr;
        for( auto &i : mVersionData )
        {
            if( i->versionString == ver )
            {
                impl = i->impl;
                break;
            }
        }
        if( !impl )
        {
            WP_EXCEPTION(
                ( String( "Cannot find serializer implementation for mesh version " ) + ver ).c_str() );
        }

        // Call implementation
        impl->importMesh( stream, static_cast<Mesh *>( pDest.get() ), mListener );
        // Warn on old version of mesh
        if( ver != mVersionData[0]->versionString )
        {
            WP_LOG_INFO( String( "WARNING: " ) + pDest->getName() + String( " is an older format (" ) +
                         ver + String( "); you should upgrade it as soon as possible" ) +
                         String( " using the OgreMeshUpgrade tool." ) );
        }

        return pDest;
    }

    void MeshSerializer::setListener( MeshSerializerListener *listener )

    {
        mListener = listener;
    }

    //----------------------------------------------
    auto MeshSerializer::getListener() -> MeshSerializerListener *
    {
        return mListener;
    }

    MeshSerializer::MeshVersionData::~MeshVersionData()
    {
        delete impl;
    }

    MeshSerializer::MeshVersionData::MeshVersionData( MeshVersion _ver, const String &_string,
                                                      MeshSerializerImpl *_impl ) :
        version( _ver ),
        versionString( _string ),
        impl( _impl )
    {
    }

}  // namespace workphone
