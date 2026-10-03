#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/LogStream.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{

    void LogStream::write( const char *message )
    {
        auto msg = message ? String( message ) : String();
        StringUtil::trim( msg );

        WP_LOG( "Assimp: " + msg );
    }

}  // namespace workphone
