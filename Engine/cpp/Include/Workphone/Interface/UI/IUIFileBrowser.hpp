#ifndef __IUIFileBrowser_H
#define __IUIFileBrowser_H

#include <Workphone/Interface/UI/IUIDialogBox.hpp>

/**
 * @file IUIFileBrowser.hpp
 * @brief Interface definition for a platform-agnostic file browser dialog.
 *
 * This header declares the `IUIFileBrowser` interface which extends
 * `IUIDialogBox` and exposes configuration and accessors for common file
 * browser behaviors such as dialog mode, filtering and selected path/extension.
 */

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Abstract interface for a file browser dialog.
         *
         * Implementations of this interface provide a native or custom file
         * selection dialog. Consumers can query and set the currently selected
         * path, filter mode and dialog behavior (open/save/select directory).
         */
        class WPCore_API IUIFileBrowser : public IUIDialogBox
        {
        public:
            /**
             * @brief Controls the overall behaviour of the dialog.
             *
             * - `Select`  : Allow selecting a directory (select folder mode).
             * - `Open`    : Open an existing file (open file mode).
             * - `Save`    : Save to a file (save file mode).
             */
            enum class DialogMode
            {
                Select,  ///< Select a directory
                Open,    ///< Open an existing file
                Save     ///< Save to a file
            };

            /**
             * @brief Bitmask describing what should be shown by the browser.
             *
             * These values can be combined if the implementation supports it
             * (e.g. show files and directories together).
             */
            enum class FilterMode
            {
                FilterMode_Files = 0x01,  ///< Show files
                FilterMode_Dirs = 0x02    ///< Show directories
            };

            IUIFileBrowser() : IUIDialogBox( IUIFileBrowser::typeInfo() )
            {
            }

            IUIFileBrowser( u32 poolTypeId ) : IUIDialogBox( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor.
             */
            ~IUIFileBrowser() override;

            /**
             * @brief Get the currently selected file or directory path.
             *
             * When the dialog is used in `Open` or `Save` modes this will
             * typically be a full file path. In `Select` (directory) mode this
             * will refer to the selected folder path.
             *
             * @return The selected file or directory path as a `String`.
             */
            virtual String getFilePath() const = 0;

            /**
             * @brief Set the initial file or directory path to display.
             *
             * Implementations may use this as the starting directory or the
             * default filename in save mode.
             *
             * @param filePath The file or directory path to set.
             */
            virtual void setFilePath( const String &filePath ) = 0;

            /**
             * @brief Get the current file extension filter.
             *
             * This returns the extension or filter string used by the dialog
             * (for example ".txt" or "*.png;*.jpg").
             *
             * @return The file extension or filter string.
             */
            virtual String getFileExtension() const = 0;

            /**
             * @brief Set the file extension or filter used by the dialog.
             *
             * Use this to restrict visible files in `Open`/`Save` modes. The
             * format is implementation-defined (common forms include ".ext"
             * or patterns like "*.png;*.jpg").
             *
             * @param fileExtension The extension or filter string to apply.
             */
            virtual void setFileExtension( const String &fileExtension ) = 0;

            /**
             * @brief Get the dialog mode.
             *
             * @return The current DialogMode value.
             */
            virtual DialogMode getDialogMode() const = 0;

            /**
             * @brief Set the dialog mode.
             *
             * @param mode The DialogMode to use for this dialog instance.
             */
            virtual void setDialogMode( DialogMode mode ) = 0;

            /**
             * @brief Get the current filter mode bitmask.
             *
             * @return The FilterMode value describing what the browser shows.
             */
            virtual FilterMode getFilterMode() const = 0;

            /**
             * @brief Set the filter mode bitmask.
             *
             * @param mode The FilterMode value (or-combined flags) to apply.
             */
            virtual void setFilterMode( FilterMode mode ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // __IUIFileBrowser_H
