#ifndef __FolderListing_h__
#define __FolderListing_h__

#include <Workphone/Interface/IO/IFolderExplorer.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @file FolderListing.hpp
     * @brief In-memory representation of a folder listing used by the Workphone IO subsystem.
     */

    /**
     * @class FolderListing
     * @brief Concrete implementation of `IFolderExplorer` that stores folder metadata.
     *
     * `FolderListing` is a lightweight container that holds the folder name, a list of
     * file names contained in the folder and a list of child folder explorers. It is
     * intended for use where an in-memory snapshot of a directory structure is required.
     */
    class FolderListing : public IFolderExplorer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes an empty folder listing. Member containers are empty and
         * `m_folderName` is default-constructed.
         */
        FolderListing();

        /**
         * @brief Virtual destructor.
         */
        ~FolderListing() override;

        /**
         * @brief Get the stored folder name.
         * @return The folder name as a `String`.
         */
        String getFolderName() const override;

        /**
         * @brief Set the folder name.
         * @param folderName The folder name to store (copied).
         */
        void setFolderName( const String &folderName ) override;

        /**
         * @brief Get a modifiable reference to the list of file names in the folder.
         * @return Reference to an `Array<String>` containing file names.
         */
        Array<String> &getFiles() override;

        /**
         * @brief Get a const reference to the list of file names in the folder.
         * @return Const reference to an `Array<String>` containing file names.
         */
        const Array<String> &getFiles() const override;

        /**
         * @brief Replace the stored file list with the provided list.
         * @param files Array of file names to store (copied).
         */
        void setFiles( const Array<String> &files ) override;

        /**
         * @brief Get the list of sub-folder explorers.
         * @note This returns a copy of the internal container. Use `addSubFolder` to
         * add a single child without copying the whole array.
         * @return An `Array` of `SmartPtr<IFolderExplorer>` representing sub-folders.
         */
        Array<SmartPtr<IFolderExplorer>> getSubFolders() const override;

        /**
         * @brief Replace the stored sub-folder list with the provided list.
         * @param subFolders Array of smart pointers to `IFolderExplorer` implementations (copied).
         */
        void setSubFolders( const Array<SmartPtr<IFolderExplorer>> &subFolders ) override;

        /**
         * @brief Append a single sub-folder to the internal list.
         * @param subFolder Smart pointer to an `IFolderExplorer` representing the child folder.
         */
        void addSubFolder( SmartPtr<IFolderExplorer> subFolder ) override;

    protected:
        /**
         * @brief Stored folder name.
         */
        FixedString<WP_MAX_PATH> m_folderName;

        /**
         * @brief Stored file names contained in the folder.
         */
        Array<String> m_files;

        /**
         * @brief Stored sub-folder explorers.
         */
        Array<SmartPtr<IFolderExplorer>> m_subFolders;
    };

}  // namespace workphone

#endif  // __FolderListing_h__
