#ifndef ___AssetDatabaseManager_h__
#define ___AssetDatabaseManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Database/DatabaseManager.hpp>
#include <Workphone/Core/ConcurrentMap.hpp>

namespace workphone
{

    /** @class AssetDatabaseManager
     *  @brief Manages the asset database system for storing and retrieving game assets.
     *
     *  This class extends DatabaseManager to provide specialized functionality for managing
     *  game assets in the database. It handles resource entries, components, and provides
     *  methods for CRUD operations on game assets.
     */
    class WPCore_API AssetDatabaseManager : public DatabaseManager
    {
    public:
        /** @brief Default constructor.
         *  Initializes a new instance of the AssetDatabaseManager.
         */
        AssetDatabaseManager();

        /** @brief Virtual destructor.
         *  Ensures proper cleanup of resources.
         */
        ~AssetDatabaseManager() override;

        /** @brief Loads the database manager with the specified data.
         *  @param data Shared object containing initialization data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @brief Unloads the database manager and its associated data.
         *  @param data Shared object containing cleanup data.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @brief Loads the asset database and ensures required asset tables exist.
         *  @param filePath The path to the database file.
         */
        void loadFromFile( const String &filePath ) override;

        /** @brief Loads the asset database and ensures required asset tables exist.
         *  @param filePath The wide string path to the database file.
         */
        void loadFromFile( const StringW &filePath ) override;

        /** @brief Creates the database and necessary tables.
         *  Initializes the database structure for asset management.
         */
        void create() override;

        /** @brief Destroys the database and cleans up resources.
         *  Performs cleanup operations on the database.
         */
        void destroy() override;

        /** @brief Clears all data from the database.
         *  Removes all entries from the resources and components tables.
         */
        void clearDatabase();

        /** @brief Checks if a resource entry exists for the given object.
         *  @param object The shared object to check.
         *  @return true if the resource entry exists, false otherwise.
         */
        bool hasResourceEntry( SmartPtr<ISharedObject> object );

        /** @brief Adds a new resource entry to the database.
         *  @param object The shared object to add as a resource.
         */
        void addResourceEntry( SmartPtr<ISharedObject> object );

        /** @brief Updates an existing resource entry in the database.
         *  @param object The shared object containing updated resource data.
         */
        void updateResourceEntry( SmartPtr<ISharedObject> object );

        /** @brief Removes a resource entry from the database.
         *  @param object The shared object to remove.
         */
        void removeResourceEntry( SmartPtr<ISharedObject> object );

        /** @brief Removes a resource entry based on its file path.
         *  @param path The file path of the resource to remove.
         */
        void removeResourceEntryFromPath( const String &path );

        /** @brief Retrieves a resource entry by its UUID.
         *  @param uuid The unique identifier of the resource.
         *  @return A smart pointer to the resource director if found, nullptr otherwise.
         */
        SmartPtr<IBuildDirector> getResourceEntry( const String &uuid );

        /** @brief Retrieves a resource entry by its file path.
         *  @param path The file path of the resource.
         *  @return A smart pointer to the resource director if found, nullptr otherwise.
         */
        SmartPtr<IBuildDirector> getResourceEntryFromPath( const String &path );

        /** @brief Checks if a resource exists by its UUID.
         *  @param uuid The unique identifier to check.
         *  @return true if the resource exists, false otherwise.
         */
        bool hasResourceById( const String &uuid );

        /** @brief Gets the name of the resources table.
         *  @return The name of the resources table.
         */
        String getResourcesTableName() const;

        /** @brief Sets the name of the resources table.
         *  @param resourcesTableName The new name for the resources table.
         */
        void sestResourcesTableName( const String &resourcesTableName );

        /** @brief Gets the name of the static components table.
         *  @return The name of the static components table.
         */
        String getStaticComponentsTableName() const;

        /** @brief Sets the name of the static components table.
         *  @param staticComponentsTableName The new name for the static components table.
         */
        void setStaticComponentsTableName( const String &staticComponentsTableName );

        /** @brief Gets the name of the components table.
         *  @return The name of the components table.
         */
        String getComponentsTableName() const;

        /** @brief Sets the name of the components table.
         *  @param componentsTableName The new name for the components table.
         */
        void setComponentsTableName( const String &componentsTableName );

        WP_CLASS_REGISTER_DECL;

    protected:
        void clearResourceEntryCache();

        AtomicObject<FixedString<128>>
            m_resourcesTableName;  ///< Name of the resources table in the database
        AtomicObject<FixedString<128>>
            m_staticComponentsTableName;  ///< Name of the static components table in the database
        AtomicObject<FixedString<128>>
            m_componentsTableName;  ///< Name of the components table in the database

        ConcurrentMap<FixedString<256>, SmartPtr<IBuildDirector>> m_resourceEntriesByUUID;
        ConcurrentMap<FixedString<1024>, SmartPtr<IBuildDirector>> m_resourceEntriesByPath;
    };
}  // namespace workphone

#endif  // ___AssetDatabaseManager_h__
