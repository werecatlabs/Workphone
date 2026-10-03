#ifndef SQLiteObfuscateCodec_h__
#define SQLiteObfuscateCodec_h__

#include <Workphone/WorkphoneConfig.hpp>
#include <sqlite3.h>
#include <array>

namespace workphone
{
    class Codec
    {
    public:
        Codec( void *db );
        Codec( const Codec *other, void *db );

        void GenerateWriteKey( const char *userPassword, int passwordLength );
        void DropWriteKey();
        void SetWriteIsRead();
        void SetReadIsWrite();

        unsigned char *Encrypt( int page, unsigned char *data, bool useWriteKey );
        void Decrypt( int page, unsigned char *data );

        void SetPageSize( int pageSize );

        bool HasReadKey();
        bool HasWriteKey();
        void *GetDB();
        const char *GetAndResetError();

    private:
        int m_pageSize = 0;
        std::array<unsigned char, 65536> m_page;
        void *m_db = nullptr;

        bool m_hasReadKey = false;
        bool m_hasWriteKey = false;

        size_t m_readKey;
        size_t m_writeKey;
        size_t m_ivReadKey;
        size_t m_ivWriteKey;

        size_t m_hash;
    };
}  // namespace workphone

#endif  // SQLiteObfuscateCodec_h__
