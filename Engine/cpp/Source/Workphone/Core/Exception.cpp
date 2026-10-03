#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <iostream>
#include <sstream>

#if WP_ENABLE_TRACE
#    include <boost/stacktrace.hpp>
#endif

namespace workphone
{

    Exception::Exception() = default;

#if defined WP_PLATFORM_WIN32
    Exception::Exception( const String &description, const char *file, s32 line, const char *func )
#else
    Exception::Exception( const String &description, const char *file, s32 line, const char *func )
#endif
    {
        m_description = description + " file: " + file + " line: " + StringUtil::toString( line );
        m_file = file ? file : "";
        m_line = line;
        m_source = func ? func : "";

        std::stringstream strStream;
        strStream << description.c_str() << std::endl;

        std::cout << strStream.str() << std::endl;

#if WP_ENABLE_TRACE
        strStream << boost::stacktrace::stacktrace();

        m_description = strStream.str() + String( " " ) + m_file + String( " " ) +
                        StringUtil::toString( (s32)m_line ) + String( " " ) + m_source;

        WP_LOG_ERROR( m_description );
#endif
    }

    Exception::~Exception() = default;

    char const *Exception::what() const noexcept
    {
        return m_description.c_str();
    }

    String Exception::getDescription() const
    {
        return m_description;
    }

    void Exception::setDescription( const String &description )
    {
        m_description = description;
    }

    RuntimeException::RuntimeException( const String &description, const char *file /*= __FILE__*/,
                                        s32 line /*= __LINE__*/, const char *func /*= 0 */ ) :
        Exception( description, file, line, func )
    {
    }

    FatalException::FatalException( const String &description, const char *file /*= __FILE__*/,
                                    s32 line /*= __LINE__*/, const char *func /*= 0 */ ) :
        Exception( description, file, line, func )
    {
    }

    ScriptException::ScriptException( const String &description, const char *file /*= __FILE__*/,
                                      s32 line /*= __LINE__*/, const char *func /*= 0 */ ) :
        Exception( description, file, line, func )
    {
    }
}  // namespace workphone
