#pragma once

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::meshy_credentials
{
    /** Saves the current user's Meshy key outside project files and Editor exports. */
    WPCore_API bool saveEditorKey( const String &key, String &error );

    /** Removes the saved key. Subsequent requests may use MESHY_API_KEY. */
    WPCore_API bool clearEditorKey( String &error );

    /** Reports saved-key availability without returning the secret to UI controls. */
    WPCore_API bool hasEditorKey( String &error );

    /** Only Editor requests consult saved credentials; other clients use the environment. */
    WPCore_API bool resolveKey( bool useEditorKey, String &key, String &error );
}
