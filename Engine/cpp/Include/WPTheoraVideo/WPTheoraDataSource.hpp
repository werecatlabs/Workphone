#ifndef WPTheoraDataSource_h__
#define WPTheoraDataSource_h__

#if __has_include( "TheoraDataSource.hpp" )
#    include "TheoraDataSource.hpp"
#    define WP_THEORA_HAS_THEORADATA_SOURCE 1
#else
#    define WP_THEORA_HAS_THEORADATA_SOURCE 0
class TheoraDataSource
{
public:
    virtual ~TheoraDataSource() = default;
};
#endif

#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/IO/IStream.hpp>

namespace workphone
{
    class WPTheoraDataSource : public TheoraDataSource
    {
    public:
        explicit WPTheoraDataSource( const String &filename );
        ~WPTheoraDataSource() override;

        int read( void *output, int nBytes );
        void seek( unsigned long byteIndex );
        std::string repr();
        unsigned long size();
        unsigned long tell();

        String getFileName() const;
        void setFileName( const String &fileName );

        SmartPtr<IStream> getStream() const;
        void setStream( SmartPtr<IStream> stream );

    protected:
        String m_fileName;
        SmartPtr<IStream> m_stream;
    };
}  // namespace workphone

#endif  // WPTheoraDataSource_h__
