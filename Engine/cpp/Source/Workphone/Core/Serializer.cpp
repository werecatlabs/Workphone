#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Serializer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IO/IStream.hpp>

namespace workphone
{
    /// stream overhead = ID + size
    const u32 STREAM_OVERHEAD_SIZE = sizeof( u16 ) + sizeof( u32 );
    const u16 HEADER_STREAM_ID = 0x1000;
    const u16 OTHER_ENDIAN_HEADER_STREAM_ID = 0x0010;

    Serializer::Serializer()
    {
        // Version number
        m_version = "[Serializer_v1.00]";
        m_flipEndian = false;
    }

    Serializer::~Serializer() = default;

    void Serializer::determineEndianness( SmartPtr<IStream> &stream )
    {
        WP_ASSERT( stream->tell() == 0 );

        if( stream->tell() != 0 )
        {
            WP_LOG_ERROR( "Can only determine the endianness of the input stream if it " );
        }

        u16 dest;
        // read header id manually (no conversion)
        auto actually_read = stream->read( &dest, sizeof( u16 ) );
        // skip back
        stream->seek( 0 );
        if( actually_read != sizeof( u16 ) )
        {
            // end of file?
            WP_LOG_ERROR( "Couldn't read 16 bit header value from input stream." );
        }

        if( dest == HEADER_STREAM_ID )
        {
            m_flipEndian = false;
        }
        else if( dest == OTHER_ENDIAN_HEADER_STREAM_ID )
        {
            m_flipEndian = true;
        }
        else
        {
            WP_LOG_ERROR( "Header chunk didn't match either endian: Corrupted stream?" );
        }
    }

    void Serializer::determineEndianness( Endian requestedEndian )
    {
        switch( requestedEndian )
        {
        case Endian::ENDIAN_NATIVE:
            m_flipEndian = false;
            break;
        case Endian::ENDIAN_BIG:
#if WP_ENDIAN == WP_ENDIAN_BIG
            m_flipEndian = false;
#else
            m_flipEndian = true;
#endif
            break;
        case Endian::ENDIAN_LITTLE:
#if WP_ENDIAN == WP_ENDIAN_BIG
            m_flipEndian = true;
#else
            m_flipEndian = false;
#endif
            break;
        }
    }

    void Serializer::writeFileHeader()
    {
        u16 val = HEADER_STREAM_ID;
        writeShorts( &val, 1 );

        writeString( m_version );
    }

    void Serializer::writeChunkHeader( u16 id, u32 size )
    {
        auto u16id = id;
        writeShorts( &u16id, 1 );

        auto u32size = size;
        writeInts( &u32size, 1 );
    }

    void Serializer::writeFloats( const float *const pFloat, u32 count )
    {
        if( m_flipEndian )
        {
            auto pFloatToWrite = static_cast<float *>( malloc( sizeof( float ) * count ) );
            memcpy( pFloatToWrite, pFloat, sizeof( float ) * count );

            flipToLittleEndian( pFloatToWrite, sizeof( float ), count );
            writeData( pFloatToWrite, sizeof( float ), count );

            free( pFloatToWrite );
        }
        else
        {
            writeData( pFloat, sizeof( float ), count );
        }
    }

    void Serializer::writeFloats( const double *const pDouble, u32 count )
    {
        // Convert to float, then write
        // float* tmp = OGRE_ALLOC_T(float, count, MEMCATEGORY_GENERAL);
        auto tmp = new float[count];
        for( unsigned int i = 0; i < count; ++i )
        {
            tmp[i] = static_cast<float>( pDouble[i] );
        }
        if( m_flipEndian )
        {
            flipToLittleEndian( tmp, sizeof( float ), count );
            writeData( tmp, sizeof( float ), count );
        }
        else
        {
            writeData( tmp, sizeof( float ), count );
        }
        // OGRE_FREE(tmp, MEMCATEGORY_GENERAL);
        delete[] tmp;
    }

    void Serializer::writeShorts( const u16 *const pShort, u32 count = 1 )
    {
        if( m_flipEndian )
        {
            auto pShortToWrite =
                static_cast<unsigned short *>( malloc( sizeof( unsigned short ) * count ) );
            memcpy( pShortToWrite, pShort, sizeof( unsigned short ) * count );

            flipToLittleEndian( pShortToWrite, sizeof( unsigned short ), count );
            writeData( pShortToWrite, sizeof( unsigned short ), count );

            free( pShortToWrite );
        }
        else
        {
            writeData( pShort, sizeof( unsigned short ), count );
        }
    }

    void Serializer::writeInts( const u32 *const pInt, u32 count = 1 )
    {
        if( m_flipEndian )
        {
            auto pIntToWrite = static_cast<unsigned *>( malloc( sizeof( unsigned int ) * count ) );
            memcpy( pIntToWrite, pInt, sizeof( unsigned int ) * count );

            flipToLittleEndian( pIntToWrite, sizeof( unsigned int ), count );
            writeData( pIntToWrite, sizeof( unsigned int ), count );

            free( pIntToWrite );
        }
        else
        {
            writeData( pInt, sizeof( unsigned int ), count );
        }
    }

    void Serializer::writeBools( const bool *const pBool, u32 count = 1 )
    {
        // no endian flipping for 1-byte bools
        // XXX Nasty Hack to convert to 1-byte bools
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        auto pCharToWrite = static_cast<char *>( malloc( sizeof( char ) * count ) );
        for( unsigned int i = 0; i < count; i++ )
        {
            *( pCharToWrite + i ) = *const_cast<bool *>( pBool + i );
        }

        writeData( pCharToWrite, sizeof( char ), count );

        free( pCharToWrite );
#else
        writeData( pBool, sizeof( bool ), count );
#endif
    }

    void Serializer::writeData( const void *const buf, u32 size, u32 count )
    {
        m_stream->write( buf, size * count );
    }

    void Serializer::writeString( const String &string )
    {
        // Old, backwards compatible way - \n terminated
        m_stream->write( string.c_str(), string.length() );
        // Write terminating newline char
        char terminator = '\n';
        m_stream->write( &terminator, 1 );
    }

    void Serializer::readFileHeader( SmartPtr<IStream> &stream )
    {
        unsigned short headerID;

        // Read header ID
        readShorts( stream, &headerID, 1 );

        if( headerID == HEADER_STREAM_ID )
        {
            // Read version
            String ver = readString( stream );
            if( ver != m_version )
            {
                WP_LOG_ERROR(
                    "Serializer::readFileHeader:Invalid file: version incompatible, file reports " +
                    String( ver ) + " Serializer is version " + m_version );
            }
        }
        else
        {
            WP_LOG_ERROR( "Serializer::readFileHeader: Invalid file: no header" );
        }
    }

    auto Serializer::readChunk( SmartPtr<IStream> &stream ) -> unsigned short
    {
        unsigned short id;
        readShorts( stream, &id, 1 );

        readInts( stream, &m_currentStreamLen, 1 );
        return id;
    }

    void Serializer::readBools( SmartPtr<IStream> &stream, bool *pDest, u32 count )
    {
        // XXX Nasty Hack to convert 1 byte bools to 4 byte bools
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        auto pTemp = static_cast<char *>( malloc( 1 * count ) );  // to hold 1-byte bools
        stream->read( pTemp, 1 * count );
        for( unsigned int i = 0; i < count; i++ )
        {
            *( pDest + i ) = *( pTemp + i );
        }

        free( pTemp );
#else
        stream->read( pDest, sizeof( bool ) * count );
#endif
        // no flipping on 1-byte datatypes
    }

    void Serializer::readFloats( SmartPtr<IStream> &stream, float *pDest, u32 count )
    {
        stream->read( pDest, sizeof( float ) * count );
        flipFromLittleEndian( pDest, sizeof( float ), count );
    }

    void Serializer::readFloats( SmartPtr<IStream> &stream, double *pDest, u32 count )
    {
        // Read from float, convert to double
        // float* tmp = OGRE_ALLOC_T(float, count, MEMCATEGORY_GENERAL);
        auto tmp = new float[count];
        float *ptmp = tmp;
        stream->read( tmp, sizeof( float ) * count );
        flipFromLittleEndian( tmp, sizeof( float ), count );
        // Convert to doubles (no cast required)
        while( count-- )
        {
            *pDest++ = *ptmp++;
        }
        // OGRE_FREE(tmp, MEMCATEGORY_GENERAL);
        delete[] tmp;
    }

    void Serializer::readShorts( SmartPtr<IStream> &stream, unsigned short *pDest, u32 count )
    {
        stream->read( pDest, sizeof( unsigned short ) * count );
        flipFromLittleEndian( pDest, sizeof( unsigned short ), count );
    }

    void Serializer::readInts( SmartPtr<IStream> &stream, unsigned int *pDest, u32 count )
    {
        stream->read( pDest, sizeof( unsigned int ) * count );
        flipFromLittleEndian( pDest, sizeof( unsigned int ), count );
    }

    auto Serializer::readString( SmartPtr<IStream> &stream, u32 numChars ) -> String
    {
        WP_ASSERT( numChars <= 255 );
        char str[255];
        stream->read( str, numChars );
        str[numChars] = '\0';
        return str;
    }

    auto Serializer::readString( SmartPtr<IStream> &stream ) -> String
    {
        return stream->getLine( false );
    }

    void Serializer::writeObject( const Vector3<real_Num> &vec )
    {
        float tmp[3] = { vec.X(), vec.Y(), vec.Z() };
        writeFloats( tmp, 3 );
    }

    void Serializer::writeObject( const Quaternion<real_Num> &q )
    {
        float tmp[4] = { q.W(), q.Y(), q.Z(), q.W() };
        writeFloats( tmp, 4 );
    }

    void Serializer::readObject( SmartPtr<IStream> &stream, Vector3<real_Num> &pDest )
    {
        float tmp[3];
        readFloats( stream, tmp, 3 );
        pDest = Vector3<real_Num>( tmp[0], tmp[1], tmp[2] );
    }

    void Serializer::readObject( SmartPtr<IStream> &stream, Quaternion<real_Num> &pDest )
    {
        float tmp[4];
        readFloats( stream, tmp, 4 );
        pDest.X() = tmp[0];
        pDest.Y() = tmp[1];
        pDest.Z() = tmp[2];
        pDest.W() = tmp[3];
    }

    void Serializer::flipToLittleEndian( void *pData, u32 size, u32 count )
    {
        if( m_flipEndian )
        {
            flipEndian( pData, size, count );
        }
    }

    void Serializer::flipFromLittleEndian( void *pData, u32 size, u32 count )
    {
        if( m_flipEndian )
        {
            flipEndian( pData, size, count );
        }
    }

    void Serializer::flipEndian( void *pData, u32 size, u32 count )
    {
        for( unsigned int index = 0; index < count; index++ )
        {
            flipEndian( (void *)( reinterpret_cast<size_t>( pData ) + ( index * size ) ), size );
        }
    }

    void Serializer::flipEndian( void *pData, u32 size )
    {
        char swapByte;
        for( unsigned int byteIndex = 0; byteIndex < size / 2; byteIndex++ )
        {
            swapByte = *(char *)( reinterpret_cast<size_t>( pData ) + byteIndex );
            *(char *)( reinterpret_cast<size_t>( pData ) + byteIndex ) =
                *(char *)( reinterpret_cast<size_t>( pData ) + size - byteIndex - 1 );
            *(char *)( reinterpret_cast<size_t>( pData ) + size - byteIndex - 1 ) = swapByte;
        }
    }
}  // namespace workphone
