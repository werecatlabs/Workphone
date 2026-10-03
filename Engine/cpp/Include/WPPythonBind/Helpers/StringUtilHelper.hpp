#ifndef StringUtilHelper_h__
#define StringUtilHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Base/StringTypes.hpp>
#include <Workphone/Base/Array.hpp>

namespace fb
{

    class StringUtilHelper
    {
    public:
        static bool hashCheck( python_Integer hash, const char *str );

        static python_Integer _getStringHash( const char *value );

        static python_Integer _getStringHashLower( const char *value );

        static char *_c_str( const String &str );

        static char *_get( const Array<String> &strings, python_Integer idx );

        static String _toStringInt( python_Integer value );

        static String _toStringNumber( python_Number value );

        template <class T>
        static String _toStringObject( T *value )
        {
            return "";
        }
    };

}  // end namespace fb

#endif  // StringUtilHelper_h__
