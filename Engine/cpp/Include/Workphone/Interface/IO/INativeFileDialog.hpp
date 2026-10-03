#ifndef INativeFileDialog_h__
#define INativeFileDialog_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Interface for a native file dialog.
     *
     * This interface provides an abstraction for native file dialogs, allowing the user to select files
     * or directories using the platform's standard dialog windows. It supports different dialog modes
     * (select, open, save), file and directory filtering, and provides methods to get and set the
     * selected file path and extension.
     */
    class WPCore_API INativeFileDialog : public ISharedObject
    {
    public:
        /**
         * @brief The mode of the file dialog.
         */
        enum class DialogMode
        {
            Select, /**< Select a file or directory without opening or saving. */
            Open,   /**< Open an existing file. */
            Save    /**< Save a file (prompt for file name and location). */
        };

        /**
         * @brief The mode for filtering files and directories.
         */
        enum class FilterMode
        {
            FilterMode_Files = 0x01, /**< Show only files in the dialog. */
            FilterMode_Dirs = 0x02   /**< Show only directories in the dialog. */
        };

        /**
         * @brief The result of the file dialog operation.
         */
        enum class Result
        {
            Dialog_Error,  /**< An error occurred while opening or interacting with the dialog. */
            Dialog_Okay,   /**< The user successfully selected a file or directory. */
            Dialog_Cancel, /**< The user cancelled the dialog operation. */
            Count          /**< Number of result types (not used as a result). */
        };

        /**
         * @brief Virtual destructor.
         */
        ~INativeFileDialog() override;

        /**
         * @brief Opens the native file dialog.
         *
         * This method displays the native file dialog to the user according to the current dialog and
         * filter modes.
         *
         * @return The result of the file dialog operation (success, cancel, or error).
         */
        virtual Result openDialog() = 0;

        /**
         * @brief Gets the currently selected file path.
         *
         * @return A string containing the selected file or directory path. Returns an empty string if no
         * selection was made.
         */
        virtual String getFilePath() const = 0;

        /**
         * @brief Sets the file path to be initially selected or shown in the dialog.
         *
         * @param filePath A string containing the file or directory path to preselect in the dialog.
         */
        virtual void setFilePath( const String &filePath ) = 0;

        /**
         * @brief Gets the file extension filter for the dialog.
         *
         * @return A string containing the file extension filter (e.g., ".txt").
         */
        virtual String getFileExtension() const = 0;

        /**
         * @brief Sets the file extension filter for the dialog.
         *
         * @param fileExtension A string containing the file extension filter (e.g., ".png").
         */
        virtual void setFileExtension( const String &fileExtension ) = 0;

        /**
         * @brief Gets the current dialog mode (select, open, or save).
         *
         * @return The current dialog mode.
         */
        virtual DialogMode getDialogMode() const = 0;

        /**
         * @brief Sets the dialog mode (select, open, or save).
         *
         * @param mode The dialog mode to set.
         */
        virtual void setDialogMode( DialogMode mode ) = 0;

        /**
         * @brief Gets the current filter mode for files and directories.
         *
         * @return The current filter mode (files, directories, or both).
         */
        virtual FilterMode getFilterMode() const = 0;

        /**
         * @brief Sets the filter mode for files and directories.
         *
         * @param mode The filter mode to set (files, directories, or both).
         */
        virtual void setFilterMode( FilterMode mode ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // INativeFileDialog_h__
