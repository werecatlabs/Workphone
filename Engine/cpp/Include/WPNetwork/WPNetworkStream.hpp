#ifndef WPNetworkStream_h__
#define WPNetworkStream_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/Net/INetworkStream.hpp>

namespace workphone
{
    class WPNetwork_API WPNetworkStream : public INetworkStream
    {
    public:
        explicit WPNetworkStream( bool writing = true );
        ~WPNetworkStream() override;

        bool isWriting() const override;
        bool isReading() const override;
        size_t read( void *buffer, size_t size ) override;
        size_t write( const void *buffer, size_t size ) override;
        void read( s8 &value ) override;
        void read( u8 &value ) override;
        void read( s16 &value ) override;
        void read( u16 &value ) override;
        void read( s32 &value ) override;
        void read( u32 &value ) override;
        void read( f32 &value ) override;
        void read( bool &value ) override;
        void read( String &value ) override;
        void read( Vector2I &value ) override;
        void read( Vector2<real_Num> &value ) override;
        void read( Vector3I &value ) override;
        void read( Vector3<real_Num> &value ) override;
        void write( s8 value ) override;
        void write( u8 value ) override;
        void write( s16 value ) override;
        void write( u16 value ) override;
        void write( s32 value ) override;
        void write( u32 value ) override;
        void write( f32 value ) override;
        void write( bool value ) override;
        void write( const String &value ) override;
        void write( const Vector2I &value ) override;
        void write( const Vector2<real_Num> &value ) override;
        void write( const Vector3I &value ) override;
        void write( const Vector3<real_Num> &value ) override;
        size_t getSize() const override;
        size_t getPosition() const override;
        void reset() override;
        const u8 *getData() const override;

        void setData( const void *data, size_t size );

        WP_CLASS_REGISTER_DECL;

    private:
        void appendBytes( const void *data, size_t size );
        void extractBytes( void *data, size_t size );

        Array<u8> m_buffer;
        size_t m_position = 0;
        bool m_writing = true;
    };
}  // namespace workphone

#endif  // WPNetworkStream_h__
