#ifndef IFileListener_h__
#define IFileListener_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** An interface for a file listener. */
    class WPCore_API IFileListener : public ISharedObject
    {
    public:
        static const u32 FILE_LISTENER_ACTION_ADD;       ///< Constant for adding a file.
        static const u32 FILE_LISTENER_ACTION_DELETE;    ///< Constant for deleting a file.
        static const u32 FILE_LISTENER_ACTION_MODIFIED;  ///< Constant for modifying a file.

        /** Virtual destructor. */
        ~IFileListener() override;

        /** Handles a file action.
        @param action An unsigned 32-bit integer representing the file action to be handled.
        @param folderName A string object containing the folder name.
        @param fileName A string object containing the file name.
        */
        virtual void fileAction( u32 action, const String &folderName, const String &fileName ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IFileListener_h__
