#ifndef __FBException_H_
#define __FBException_H_

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

#ifdef _MSC_VER
#    pragma warning( push )
#    pragma warning( disable : 4251 )
#endif

    /** Exception class. */
    class WPCore_API Exception : public std::exception
    {
    public:
        /** Default constructor. */
        Exception();

        /** Constructor. */
        explicit Exception( const String &description, const char *file = __FILE__, s32 line = __LINE__,
                            const char *func = nullptr );

        /** Destructor. */
        ~Exception() override;

        /** Gets the error message. */
        char const *what() const noexcept override;

        /** Gets the error message. */
        String getDescription() const;

        /** Sets the error message. */
        void setDescription( const String &description );

    protected:
        s32 m_line = 0;
        String m_typeName;
        String m_description;
        String m_source;
        String m_file;
        String m_fullDesc;
    };

    class WPCore_API RuntimeException : public Exception
    {
    public:
        explicit RuntimeException( const String &description, const char *file = __FILE__,
                                   s32 line = __LINE__, const char *func = nullptr );
    };

    class WPCore_API FatalException : public Exception
    {
    public:
        explicit FatalException( const String &description, const char *file = __FILE__,
                                 s32 line = __LINE__, const char *func = nullptr );
    };

    class WPCore_API ScriptException : public Exception
    {
    public:
        explicit ScriptException( const String &description, const char *file = __FILE__,
                                  s32 line = __LINE__, const char *func = nullptr );
    };

#ifdef _MSC_VER
#    pragma warning( pop )
#endif

}  // namespace workphone

#if WP_EXCEPTIONS
#    define WP_EXCEPTION( desc ) \
        throw workphone::RuntimeException( String( desc ), __FILE__, __LINE__, __FUNCTION__ )
#    define WP_FATAL_EXCEPTION( desc ) \
        throw workphone::FatalException( String( desc ), __FILE__, __LINE__, __FUNCTION__ )
#    define WP_SCRIPT_EXCEPTION( desc ) \
        throw workphone::ScriptException( String( desc ), __FILE__, __LINE__, __FUNCTION__ )
#else
#    define WP_EXCEPTION( desc )
#    define WP_FATAL_EXCEPTION( desc )
#    define WP_SCRIPT_EXCEPTION( desc )
#endif

#endif
