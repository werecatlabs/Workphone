#ifndef LogStream_h__
#define LogStream_h__

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <assimp/DefaultLogger.hpp>

namespace workphone
{

    class LogStream : public Assimp::LogStream
    {
    public:
        LogStream() = default;

        void write( const char *message );
    };

}  // namespace workphone

#endif  // LogStream_h__
