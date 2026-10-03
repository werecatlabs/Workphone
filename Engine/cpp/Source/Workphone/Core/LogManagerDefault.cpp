#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <iostream>

#if WP_ENABLE_TRACE
#    include <boost/stacktrace.hpp>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, LogManagerDefault, ILogManager );

    SmartPtr<LogManagerDefault> LogManagerDefault::m_instance;

    LogManagerDefault::LogManagerDefault() = default;

    LogManagerDefault::~LogManagerDefault() = default;

    void LogManagerDefault::logMessage( const String &message, [[maybe_unused]] Type type )
    {
        try
        {
            std::cout << message << std::endl;

#if WP_ENABLE_TRACE
            static const auto newLineStr = String( "\n" );

            std::stringstream strStream;
            strStream << boost::stacktrace::stacktrace();
            auto trace = strStream.str();

            auto currentTimeStr = StringUtil::getCurrentTime() + String( ": " );
            auto logEntryStr = currentTimeStr + message + newLineStr + trace;
#else
            auto currentTimeStr = StringUtil::getCurrentTime() + String( ": " );
            auto logEntryStr = currentTimeStr + message;
#endif

            if( getEnableQueue() || !m_log.is_open() )
            {
                m_messages.push( logEntryStr );
            }
            else
            {
                String curMessage;
                while( m_messages.try_pop( curMessage ) )
                {
                    m_log << curMessage.c_str() << std::endl;
                }

                m_log << logEntryStr.c_str() << std::endl;
                m_log.flush();
            }
        }
        catch( std::exception &e )
        {
            std::cout << e.what() << std::endl;
        }
    }

    void LogManagerDefault::logMessage( const StringW &message, Type type )
    {
        try
        {
            StringW currentTimeStr = StringUtilW::getCurrentTime() + StringW( L": " );
            StringW logEntryStr = currentTimeStr + message;

            if( getEnableQueue() || !m_log.is_open() )
            {
                m_messages.push( StringUtil::toStringC( logEntryStr ) );
            }
            else
            {
                String message;
                while( m_messages.try_pop( message ) )
                {
                    m_log << message.c_str() << std::endl;
                }

                m_log << StringUtil::toUTF16to8( logEntryStr ) << std::endl;
                m_log.flush();
            }
        }
        catch( std::exception &e )
        {
            std::cout << e.what() << std::endl;
        }
    }

    void LogManagerDefault::setEnableQueue( bool queue )
    {
        m_queue = queue;
    }

    auto LogManagerDefault::getEnableQueue() const -> bool
    {
        return m_queue;
    }

    void LogManagerDefault::close()
    {
        m_log.close();
    }

    void LogManagerDefault::open( const String &filePath )
    {
        if( !m_log.is_open() )
        {
            m_log.open( filePath.c_str() );
        }
    }

    void LogManagerDefault::open( const StringW &filePath )
    {
#if defined WP_PLATFORM_WIN32
        if( !m_log.is_open() )
        {
            m_log.open( filePath.c_str() );
        }
#else
        auto filePathUTF8 = StringUtil::toUTF16to8( filePath );
        if( !m_log.is_open() )
        {
            m_log.open( filePathUTF8 );
        }
#endif
    }

    void LogManagerDefault::flush()
    {
        String message;
        while( m_messages.try_pop( message ) )
        {
            m_log << message.c_str() << std::endl;
        }

        m_log.flush();
    }
}  // namespace workphone
