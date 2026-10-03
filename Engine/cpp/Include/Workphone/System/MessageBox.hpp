#ifndef MessageBox_h__
#define MessageBox_h__

#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /** Used to display a message box on supported platforms. */
    class WPCore_API MessageBoxUtil
    {
    public:
        /** Shows a message box with the given text.
        @param text The text to display.
        */
        static void show( const String &text );

        /** Shows a message box with the given text.
        @param text The text to display.
        */
        static void show( const char *text );
    };

}  // namespace workphone

#endif  // MessageBox_h__
