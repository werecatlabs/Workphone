#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/DebugUtil.hpp>
#include <sstream>

#if WP_ENABLE_TRACE
#    include <boost/stacktrace.hpp>
#endif

namespace workphone
{

    auto DebugUtil::getStackTrace() -> String
    {
        std::stringstream strStream;

#if WP_ENABLE_TRACE
        strStream << boost::stacktrace::stacktrace();
#endif

        return strStream.str().c_str();
    }

    auto DebugUtil::getStackTraceForException( std::exception &e ) -> String
    {
        std::stringstream strStream;

#if WP_ENABLE_TRACE
        strStream << boost::stacktrace::stacktrace();
#endif

        return strStream.str().c_str();
    }

}  // namespace workphone
