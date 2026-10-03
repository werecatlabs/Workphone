#include <WPSQLite/SQLiteObfuscateCodec.hpp>
#include "WPSQLite/SQLiteObfuscateCodecInterface.hpp"
#include <Workphone/Workphone.hpp>
#include <cstring>

namespace workphone
{
    Codec::Codec( const Codec *other, void *db )
    {
        m_hash = 1 << 1;
        m_db = db;
        m_pageSize = 65536;
    }

    Codec::Codec( void *db )
    {
        m_hash = 1 << 1;
        m_db = db;
        m_pageSize = 65536;
    }

    void Codec::GenerateWriteKey( const char *userPassword, int passwordLength )
    {
        try
        {
            //m_writeKey = SymmetricKey(masterKey.bits_of(), KEY_SIZE);
            //m_ivWriteKey = SymmetricKey(masterKey.bits_of() + KEY_SIZE, IV_DERIVATION_KEY_SIZE);

            m_hasWriteKey = true;
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Codec::DropWriteKey()
    {
        m_hasWriteKey = false;
    }

    void Codec::SetWriteIsRead()
    {
        m_writeKey = m_readKey;
        m_ivWriteKey = m_ivReadKey;
        m_hasWriteKey = m_hasReadKey;
    }

    void Codec::SetReadIsWrite()
    {
        m_readKey = m_writeKey;
        m_ivReadKey = m_ivWriteKey;
        m_hasReadKey = m_hasWriteKey;
    }

    unsigned char *Codec::Encrypt( int page, unsigned char *data, bool useWriteKey )
    {
        memcpy( m_page.data(), data, m_pageSize );

        auto pch = (char *)m_page.data();
        for( size_t i = 0; i < m_pageSize; ++i )
        {
            pch[i] ^= m_hash;
        }

        return m_page.data();  //return location of newly ciphered data
    }

    void Codec::Decrypt( int page, unsigned char *data )
    {
        auto pch = (char *)data;
        for( size_t i = 0; i < m_pageSize; ++i )
        {
            pch[i] ^= m_hash;
        }
    }

    void Codec::SetPageSize( int pageSize )
    {
        m_pageSize = pageSize;
    }

    bool Codec::HasReadKey()
    {
        return m_hasReadKey;
    }

    bool Codec::HasWriteKey()
    {
        return m_hasWriteKey;
    }

    void *Codec::GetDB()
    {
        return m_db;
    }

    const char *Codec::GetAndResetError()
    {
        return nullptr;
    }
}  // namespace workphone

using namespace workphone;

void InitializeBotan()
{
    //LibraryInitializer::initialize();
}

void *InitializeNewCodec( void *db )
{
    return new Codec( db );
}

void *InitializeFromOtherCodec( const void *otherCodec, void *db )
{
    return new Codec( (Codec *)otherCodec, db );
}

void GenerateWriteKey( void *codec, const char *userPassword, int passwordLength )
{
    static_cast<Codec *>( codec )->GenerateWriteKey( userPassword, passwordLength );
}

void DropWriteKey( void *codec )
{
    static_cast<Codec *>( codec )->DropWriteKey();
}

void SetWriteIsRead( void *codec )
{
    static_cast<Codec *>( codec )->SetWriteIsRead();
}

void SetReadIsWrite( void *codec )
{
    static_cast<Codec *>( codec )->SetReadIsWrite();
}

unsigned char *Encrypt( void *codec, int page, unsigned char *data, unsigned char useWriteKey )
{
    return static_cast<Codec *>( codec )->Encrypt( page, data, useWriteKey );
}

void Decrypt( void *codec, int page, unsigned char *data )
{
    static_cast<Codec *>( codec )->Decrypt( page, data );
}

void SetPageSize( void *codec, int pageSize )
{
    static_cast<Codec *>( codec )->SetPageSize( pageSize );
}

unsigned char HasReadKey( void *codec )
{
    return static_cast<Codec *>( codec )->HasReadKey();
}

unsigned char HasWriteKey( void *codec )
{
    return static_cast<Codec *>( codec )->HasWriteKey();
}

void *GetDB( void *codec )
{
    return static_cast<Codec *>( codec )->GetDB();
}

const char *GetAndResetError( void *codec )
{
    return static_cast<Codec *>( codec )->GetAndResetError();
}

void DeleteCodec( void *codec )
{
    auto deleteThisCodec = static_cast<Codec *>( codec );
    delete deleteThisCodec;
}
