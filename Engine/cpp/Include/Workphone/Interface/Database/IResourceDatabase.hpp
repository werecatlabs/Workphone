#ifndef IResourceDatabase_h__
#define IResourceDatabase_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{

    /** @brief Interface for managing and organizing resources in a database.
     *
     * The IResourceDatabase class provides a comprehensive system for managing resources and their
     * dependencies. It handles the conversion of project files into resources and maintains them in a
     * structured database. The database supports operations such as importing, building, optimizing, and
     * cleaning resources.
     *
     * Key features:
     * - Resource management and dependency tracking
     * - Import and export of assets
     * - Resource caching and optimization
     * - Resource loading and unloading
     * - Database maintenance operations
     */
    class WPCore_API IResourceDatabase : public ISharedObject
    {
    public:
        /** @brief Virtual destructor. */
        ~IResourceDatabase() override;

        /** @brief Imports cache data into the database.
         *
         * This method loads cached resource data into the database, improving load times
         * for previously processed resources.
         */
        virtual void importCache() = 0;

        /** @brief Imports all assets in the project into the database.
         *
         * This method processes all project assets and converts them into resources
         * that can be managed by the database.
         */
        virtual void importAssets() = 0;

        /** @brief Reimports all assets in the project.
         *
         * This method performs a complete reimport of all project assets by:
         * 1. Clearing all existing asset data
         * 2. Reimporting all assets from scratch
         * 3. Rebuilding all resource dependencies
         */
        virtual void reimportAssets() = 0;

        /** @brief Calculates dependencies between resources in the database.
         *
         * Analyzes all resources and establishes their dependency relationships,
         * which is crucial for proper resource loading and unloading.
         */
        virtual void calculateDependencies() = 0;

        /** @brief Builds all resources in the database.
         *
         * Attempts to build all resources without performing a full reimport.
         * This is useful for rebuilding resources after dependency changes.
         */
        virtual void build() = 0;

        /** @brief Refreshes the resource database.
         *
         * Performs a validation check of the database and updates its state.
         * This ensures the database is in a consistent state.
         */
        virtual void refresh() = 0;

        /** @brief Optimizes the database for better performance.
         *
         * Performs optimization operations on the database structure and resources
         * to improve access times and reduce memory usage.
         */
        virtual void optimise() = 0;

        /** @brief Cleans the database.
         *
         * Removes temporary files and performs maintenance operations
         * to keep the database in a clean state.
         */
        virtual void clean() = 0;

        /** @brief Deletes all cache files associated with the database.
         *
         * Removes all cached data, forcing a fresh import on next load.
         */
        virtual void deleteCache() = 0;

        /** @brief Checks if a resource exists in the database.
         * @param resource The resource to check for existence
         * @return true if the resource exists in the database, false otherwise
         */
        virtual bool hasResource( SmartPtr<IResource> resource ) = 0;

        /** @brief Adds a resource to the database.
         * @param resource The resource to add to the database
         */
        virtual void addResource( SmartPtr<IResource> resource ) = 0;

        /** @brief Removes a resource from the database.
         * @param resource The resource to remove from the database
         */
        virtual void removeResource( SmartPtr<IResource> resource ) = 0;

        /** @brief Removes a resource from the database using its path.
         * @param path The path of the resource to remove
         */
        virtual void removeResourceFromPath( const String &path ) = 0;

        /** @brief Finds a resource by its type and path.
         * @param type The type identifier of the resource
         * @param path The path of the resource to find
         * @return A smart pointer to the found resource, or nullptr if not found
         */
        virtual SmartPtr<IResource> findResource( u32 type, const String &path ) = 0;

        /** @brief Clones an existing resource with a new path.
         * @param type The type identifier of the resource
         * @param resource The resource to clone
         * @param path The new path for the cloned resource
         * @return A smart pointer to the cloned resource, or nullptr if cloning failed
         */
        virtual SmartPtr<IResource> cloneResource( u32 type, SmartPtr<IResource> resource,
                                                   const String &path ) = 0;

        /** @brief Gets all resource directors from the database.
         * @return An array of smart pointers to resource directors
         */
        virtual Array<SmartPtr<IBuildDirector>> getResourceData() const = 0;

        /** @brief Gets all resources in the database.
         * @return An array of smart pointers to all resources
         */
        virtual Array<SmartPtr<IResource>> getResources() const = 0;

        /** @brief Imports a folder into the database using a folder explorer.
         * @param folderListing The folder explorer containing the folder to import
         */
        virtual void importFolder( SmartPtr<IFolderExplorer> folderListing, bool overwrite = false ) = 0;

        /** @brief Imports a folder into the database using its path.
         * @param path The path of the folder to import
         */
        virtual void importFolder( const String &path, bool overwrite = false ) = 0;

        /** @brief Imports a single file into the database.
         * @param filePath The path of the file to import
         */
        virtual void importFile( const String &filePath, bool overwrite = false ) = 0;

        /** @brief Loads a resource using its UUID.
         * @param id The UUID of the resource to load
         * @return A smart pointer to the loaded resource, or nullptr if loading failed
         */
        virtual SmartPtr<IResource> loadResource( const UUID &id ) = 0;

        /** @brief Loads a resource using its path.
         * @param path The path of the resource to load
         * @return A smart pointer to the loaded resource, or nullptr if loading failed
         */
        virtual SmartPtr<IResource> loadResource( const String &path ) = 0;

        /** @brief Loads a director using its file path.
         * @param filePath The path of the director to load
         * @return A smart pointer to the loaded director, or nullptr if loading failed
         */
        virtual SmartPtr<IBuildDirector> loadDirector( const String &filePath ) = 0;

        /** @brief Loads a director using a resource path.
         * @param filePath The resource path to load the director from
         * @return A smart pointer to the loaded director, or nullptr if loading failed
         */
        virtual SmartPtr<IBuildDirector> loadDirectorFromResourcePath( const String &filePath ) = 0;

        /** @brief Loads a director using a resource path and type.
         * @param filePath The resource path to load the director from
         * @param type The type of the resource
         * @return A smart pointer to the loaded director, or nullptr if loading failed
         */
        virtual SmartPtr<IBuildDirector> loadDirectorFromResourcePath( const String &filePath,
                                                                       u32 type ) = 0;

        /** @brief Loads a director from a resource.
         * @param resource The resource to load the director from
         * @return A smart pointer to the loaded director, or nullptr if loading failed
         */
        virtual SmartPtr<IBuildDirector> loadDirector( SmartPtr<IResource> resource ) = 0;

        /** @brief Loads a resource using its UUID.
         * @param uuid The UUID of the resource to load
         * @return A smart pointer to the loaded resource, or nullptr if loading failed
         */
        virtual SmartPtr<IResource> loadResourceById( const UUID &uuid ) = 0;

        /** @brief Unloads all resources that are not currently in use.
         *
         * This method helps manage memory by unloading resources that are no longer needed.
         */
        virtual void unloadUnusedResources() = 0;

        /** @brief Removes all unused resources from the database.
         *
         * This method permanently removes resources that are not being used,
         * freeing up database space.
         */
        virtual void removeUnusedResources() = 0;

        /** @brief Gets the database manager instance.
         * @return A smart pointer to the database manager
         */
        virtual SmartPtr<IDatabaseManager> getDatabaseManager() const = 0;

        /** @brief Sets the database manager instance.
         * @param databaseManager The database manager to set
         */
        virtual void setDatabaseManager( SmartPtr<IDatabaseManager> databaseManager ) = 0;

        /** @brief Creates a new resource or retrieves an existing one using a director.
         * @param type The type identifier of the resource
         * @param path The path of the resource
         * @param director The build director to use for resource creation
         * @return A pair containing the resource and a boolean indicating if it was created (true) or
         * retrieved (false)
         */
        virtual Pair<SmartPtr<IResource>, bool> createOrRetrieveFromDirector(
            hash_type type, const String &path, SmartPtr<IBuildDirector> director ) = 0;

        /** @brief Creates a new resource or retrieves an existing one.
         * @param type The type identifier of the resource
         * @param path The path of the resource
         * @return A pair containing the resource and a boolean indicating if it was created (true) or
         * retrieved (false)
         */
        virtual Pair<SmartPtr<IResource>, bool> createOrRetrieve( hash_type type,
                                                                  const String &path ) = 0;

        /** @brief Creates a new resource or retrieves an existing one using only the path.
         * @param path The path of the resource
         * @return A pair containing the resource and a boolean indicating if it was created (true) or
         * retrieved (false)
         */
        virtual Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) = 0;

        /** @brief Gets an object from the database using its UUID.
         * @param uuid The UUID of the object to retrieve
         * @return A smart pointer to the object, or nullptr if not found
         */
        virtual SmartPtr<ISharedObject> getObject( const UUID &uuid ) = 0;

        /** @brief Gets an object from the database using its file ID.
         * @param fileId The file ID of the object to retrieve
         * @return A smart pointer to the object, or nullptr if not found
         */
        virtual SmartPtr<ISharedObject> getObjectByFileId( const String &fileId ) const = 0;

        /** @brief Gets the file path of the database.
         * @return The file path of the database
         */
        virtual String getFilePath() const = 0;

        /** @brief Sets the file path of the database.
         * @param filePath The new file path for the database
         */
        virtual void setFilePath( const String &filePath ) = 0;

        /** @brief Creates a new database instance.
         *
         * Initializes a new database with the current configuration.
         */
        virtual void createDatabase() = 0;

        /** @brief Destroys the database instance.
         *
         * Performs cleanup and releases all resources associated with the database.
         */
        virtual void destroyDatabase() = 0;

        /** @brief Gets an object of a specific type using its file ID.
         * @tparam T The type of object to retrieve
         * @param fileId The file ID of the object
         * @return A smart pointer to the object of type T, or nullptr if not found
         */
        template <class T>
        SmartPtr<T> getObjectTypeByFileId( const String &fileId ) const;

        /** @brief Finds a resource of a specific type using its path.
         * @tparam T The type of resource to find
         * @param path The path of the resource
         * @return A smart pointer to the resource of type T, or nullptr if not found
         */
        template <class T>
        SmartPtr<T> findResourceByType( const String &path );

        /** @brief Clones a resource of a specific type.
         * @tparam T The type of resource to clone
         * @param resource The resource to clone
         * @param path The new path for the cloned resource
         * @return A smart pointer to the cloned resource of type T, or nullptr if cloning failed
         */
        template <class T>
        SmartPtr<T> cloneResourceByType( SmartPtr<IResource> resource, const String &path );

        /** @brief Loads a resource of a specific type using its path.
         * @tparam T The type of resource to load
         * @param path The path of the resource
         * @return A smart pointer to the loaded resource of type T, or nullptr if loading failed
         */
        template <class T>
        SmartPtr<T> loadResourceByType( const String &path );

        /** @brief Gets all resources of a specific type.
         * @tparam T The type of resources to retrieve
         * @return An array of smart pointers to resources of type T
         */
        template <class T>
        Array<SmartPtr<T>> getResourcesByType() const;

        /** @brief Creates or retrieves a resource of a specific type using a director.
         * @tparam T The type of resource to create or retrieve
         * @param path The path of the resource
         * @param director The build director to use
         * @return A pair containing the resource and a boolean indicating if it was created (true) or
         * retrieved (false)
         */
        template <class T>
        Pair<SmartPtr<T>, bool> createOrRetrieveFromDirector( const String &path,
                                                              SmartPtr<IBuildDirector> director );

        /** @brief Creates or retrieves a resource of a specific type.
         * @tparam T The type of resource to create or retrieve
         * @param path The path of the resource
         * @return A pair containing the resource and a boolean indicating if it was created (true) or
         * retrieved (false)
         */
        template <class T>
        Pair<SmartPtr<T>, bool> createOrRetrieveByType( const String &path );

        WP_CLASS_REGISTER_DECL;
    };

    template <class T>
    SmartPtr<T> IResourceDatabase::getObjectTypeByFileId( const String &fileId ) const
    {
        auto obj = getObjectByFileId( fileId );
        return workphone::dynamic_pointer_cast<T>( obj );
    }

    template <class T>
    SmartPtr<T> IResourceDatabase::findResourceByType( const String &path )
    {
        auto type = T::typeInfo();
        if( auto resource = findResource( type, path ) )
        {
            if( resource->template isDerived<T>() )
            {
                return workphone::static_pointer_cast<T>( resource );
            }
        }

        return nullptr;
    }

    template <class T>
    SmartPtr<T> IResourceDatabase::cloneResourceByType( SmartPtr<IResource> resource,
                                                        const String &path )
    {
        auto type = T::typeInfo();
        if( auto newResource = cloneResource( type, resource, path ) )
        {
            if( newResource->template isDerived<T>() )
            {
                return workphone::static_pointer_cast<T>( newResource );
            }
        }

        return nullptr;
    }

    template <class T>
    SmartPtr<T> IResourceDatabase::loadResourceByType( const String &path )
    {
        if( auto resource = loadResource( path ) )
        {
            if( resource->template isDerived<T>() )
            {
                return workphone::static_pointer_cast<T>( resource );
            }
        }

        return nullptr;
    }

    template <class T>
    Array<SmartPtr<T>> IResourceDatabase::getResourcesByType() const
    {
        const auto resources = getResources();

        Array<SmartPtr<T>> resourcesByType;
        resourcesByType.reserve( resources.size() );

        for( auto &r : resources )
        {
            if( r )
            {
                if( r->template isDerived<T>() )
                {
                    WP_ASSERT( workphone::dynamic_pointer_cast<T>( r ) );
                    resourcesByType.push_back( r );
                }
            }
        }

        return resourcesByType;
    }

    template <class T>
    Pair<SmartPtr<T>, bool> IResourceDatabase::createOrRetrieveFromDirector(
        const String &path, SmartPtr<IBuildDirector> director )
    {
        auto type = T::typeInfo();
        auto result = createOrRetrieveFromDirector( type, path, director );
        if( result.first )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( result.first ) );
            return Pair<SmartPtr<T>, bool>( result.first, result.second );
        }

        return {};
    }

    template <class T>
    Pair<SmartPtr<T>, bool> IResourceDatabase::createOrRetrieveByType( const String &path )
    {
        auto type = T::typeInfo();
        auto result = createOrRetrieve( type, path );
        if( result.first )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( result.first ) );
            return Pair<SmartPtr<T>, bool>( result.first, result.second );
        }

        return {};
    }

}  // namespace workphone

#endif  // IResourceDatabase_h__
