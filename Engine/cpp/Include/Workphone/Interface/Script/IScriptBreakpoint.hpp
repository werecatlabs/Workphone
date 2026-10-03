#ifndef IScriptBreakpoint_h__
#define IScriptBreakpoint_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for a script breakpoint.
     */
    class WPCore_API IScriptBreakpoint : public ISharedObject
    {
    public:
        /** Destructor. */
        ~IScriptBreakpoint() override;

        /** Gets the line number where the breakpoint is set.
         * @return The line number where the breakpoint is set.
         */
        virtual s32 getLineNumber() const = 0;

        /** Sets the line number where the breakpoint is set.
         * @param lineNumber The line number where the breakpoint is set.
         */
        virtual void setLineNumber( s32 lineNumber ) = 0;

        /** Gets the file path where the breakpoint is set.
         * @return The file path where the breakpoint is set.
         */
        virtual String getFilePath() const = 0;

        /** Sets the file path where the breakpoint is set.
         * @param filePath The file path where the breakpoint is set.
         */
        virtual void setFilePath( const String &filePath ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IScriptBreakpoint_h__
