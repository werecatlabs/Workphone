#ifndef FileSelection_h__
#define FileSelection_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/System/RttiClass.hpp>

namespace workphone
{
    namespace editor
    {

        /**
         * @class FileSelection
         * @brief Represents a file selection in the editor, encapsulating file path and file info.
         *
         * This class is used to store and manage information about a selected file, including its path
         * and additional file metadata. It inherits from ISharedObject to support shared ownership
         * semantics.
         */
        class WPCore_API FileSelection : public ISharedObject
        {
        public:
            /**
             * @brief Default constructor.
             */
            FileSelection();

            /**
             * @brief Destructor.
             */
            ~FileSelection() override;

            /**
             * @brief Gets the file path of the selected file.
             * @return The file path as a String.
             */
            String getFilePath() const;

            /**
             * @brief Sets the file path of the selected file.
             * @param filePath The file path to set.
             */
            void setFilePath( const String &filePath );

            /**
             * @brief Gets the FileInfo object containing metadata about the selected file.
             * @return The FileInfo object.
             */
            FileInfo getFileInfo() const;

            /**
             * @brief Sets the FileInfo object containing metadata about the selected file.
             * @param fileInfo The FileInfo object to set.
             */
            void setFileInfo( const FileInfo &fileInfo );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// The file path of the selected file.
            FixedString<WP_MAX_PATH> m_filePath;

            /// The FileInfo object containing metadata about the selected file.
            FileInfo m_fileInfo;
        };

    }  // end namespace editor
}  // namespace workphone

#endif  // FileSelection_h__
