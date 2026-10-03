#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/StringUtilHelper.hpp>
#include <Workphone/Workphone.hpp>

namespace fb
{

    bool StringUtilHelper::hashCheck( python_Integer hash, const char *str )
    {
        return *reinterpret_cast<u32 *>( &hash ) == StringUtil::getHash( str );
    }

    python_Integer StringUtilHelper::_getStringHash( const char *value )
    {
        hash32 hash = StringUtil::getHash( value );
        python_Integer iHash = *reinterpret_cast<python_Integer *>( &hash );
        return iHash;
    }

    python_Integer StringUtilHelper::_getStringHashLower( const char *value )
    {
        hash32 hash = StringUtil::getHash( value );
        python_Integer iHash = *reinterpret_cast<python_Integer *>( &hash );
        return iHash;
    }

    char *StringUtilHelper::_c_str( const String &str )
    {
        return (c8 *)str.c_str();
    }

    char *StringUtilHelper::_get( const Array<String> &strings, python_Integer idx )
    {
        return (c8 *)strings[idx].c_str();
    }

    String StringUtilHelper::_toStringInt( python_Integer value )
    {
        return StringUtil::toString( value );
    }

    String StringUtilHelper::_toStringNumber( python_Number value )
    {
        return StringUtil::toString( value );
    }

}  // end namespace fb
