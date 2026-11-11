#ifndef Workphone_OpenAICredentials_hpp
#define Workphone_OpenAICredentials_hpp

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::openai_credentials
{
    /** Saves the current user's Editor key in the operating system credential store.
     * The key is not written to project files or Editor preferences. */
    WPCore_API bool saveEditorKey( const String &key, String &error );

    /** Removes the saved Editor key. Subsequent requests may use OPENAI_API_KEY. */
    WPCore_API bool clearEditorKey( String &error );

    /** Reports whether a usable key is saved, without exposing it to settings controls. */
    WPCore_API bool hasEditorKey( String &error );

    /** Resolves a key for one request. Only Editor requests consult the saved credential;
     * runtime and headless requests continue to use OPENAI_API_KEY. */
    WPCore_API bool resolveKey( bool useEditorKey, String &key, String &error );
}

#endif
