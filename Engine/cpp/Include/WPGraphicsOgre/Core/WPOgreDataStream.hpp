#ifndef WPOgreDataStream_h__
#define WPOgreDataStream_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <OgreDataStream.h>

namespace workphone
{

    class WPOgreDataStream : public Ogre::DataStream
    {
    public:
        WPOgreDataStream();
        WPOgreDataStream( SmartPtr<IStream> stream );
        ~WPOgreDataStream() override;

        size_t read( void *buf, size_t count ) override;
        Ogre::String getLine( bool trimAfter = true ) override;

        void skip( long count ) override;
        void seek( size_t pos ) override;
        size_t tell( void ) const override;

        bool eof( void ) const override;
        void close( void ) override;

        SmartPtr<IStream> getStream() const;
        void setStream( SmartPtr<IStream> stream );

    protected:
        SmartPtr<IStream> m_stream;
    };

}  // namespace workphone

#endif  // WPOgreDataStream_h__
