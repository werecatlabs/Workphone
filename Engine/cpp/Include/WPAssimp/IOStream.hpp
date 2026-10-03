#ifndef IOStream_h__
#define IOStream_h__

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <assimp/IOStream.hpp>

namespace workphone
{
    class IOStream : public Assimp::IOStream
    {
    public:
        IOStream( SmartPtr<IStream> stream );

        size_t Read( void *pvBuffer, size_t pSize, size_t pCount ) override;
        size_t Tell() const override;
        size_t FileSize() const override;

        size_t Write( const void *pvBuffer, size_t pSize, size_t pCount ) override;
        aiReturn Seek( size_t pOffset, aiOrigin pOrigin ) override;
        void Flush() override;

    protected:
        SmartPtr<IStream> m_stream;
    };
}  // namespace workphone

#endif  // IOStream_h__
