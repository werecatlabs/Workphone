#ifndef CResourceDatabase_h__
#define CResourceDatabase_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/UnorderedMap.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /**
     * @class ResourceDatabase
     * @brief Central storage and manager for engine resources.
     *
     * The ResourceDatabase owns and indexes resources discovered or created by
     * the engine. It provides facilities to import assets from disk,
     * serialize/deserialize resource metadata via the database manager,
     * instantiate resources on demand, and perform maintenance operations
     * (build, refresh, optimise, clean, cache management, etc.).
     *
     * Thread-safety: many public API methods lock an internal recursive mutex
     * where necessary to protect the resource maps and instance lists.
     */
    class WPCore_API ResourceDatabase : public IResourceDatabase
    {
    public:
        /**
         * @class ImportFileJob
         * @brief Background job used to import a single file into the database.
         *
         * This nested Job class is scheduled on the engine's job system when
         * an import operation must run asynchronously. It stores the file path
         * to import and an overwrite flag indicating whether an existing
         * resource should be replaced.
         */
        class WPCore_API ImportFileJob : public Job
        {
        public:
            /** Constructor. */
            ImportFileJob();

            /** Destructor. */
            ~ImportFileJob() override;

            /**
             * @copydoc Job::execute
             *
             * The implementation performs the file import using the path set
             * via `setFilePath` and applies the overwrite behaviour if
             * `setOverwrite(true)` was called.
             */
            void execute() override;

            /**
             * @brief Set the file path to import by this job.
             * @param[in] filePath Path on disk to the asset to import.
             */
            void setFilePath( const String &filePath );

            /**
             * @brief Get the currently configured import file path.
             * @return The file path that will be imported when the job runs.
             */
            String getFilePath() const;

            /**
             * @brief Query whether the job will overwrite existing resources.
             * @return True if existing resources should be replaced.
             */
            bool getOverwrite() const;

            /**
             * @brief Configure whether import should overwrite existing resources.
             * @param[in] overwrite If true, existing resources will be replaced.
             */
            void setOverwrite( bool overwrite );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** File path to import when the job executes. */
            String m_filePath;

            /** If true, the import will overwrite any existing resources at the
             * same path/identifier. */
            bool m_overwrite = false;
        };

        static const String defaultDatabaseFileName;

        /**
         * @brief Construct a new ResourceDatabase.
         *
         * The database is created in an unloaded state; call `load` to
         * initialise it with a database manager or other startup data.
         */
        ResourceDatabase();

        /**
         * @brief Destructor; cleans up in-memory resource lists.
         */
        ~ResourceDatabase() override;

        /**
         * @copydoc ISharedObject::load
         *
         * The `data` parameter typically contains configuration or a
         * database manager instance required to initialise the resource
         * database.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc ISharedObject::unload
         *
         * Releases any runtime resources and clears internal containers.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IResourceDatabase::build */
        void build() override;

        /** @copydoc IResourceDatabase::refresh */
        void refresh() override;

        /** @copydoc IResourceDatabase::optimise */
        void optimise() override;

        /** @copydoc IResourceDatabase::clean */
        void clean() override;

        /** @copydoc IResourceDatabase::deleteCache */
        void deleteCache() override;

        /** @copydoc IResourceDatabase::hasResource */
        bool hasResource( SmartPtr<IResource> resource ) override;

        /** @copydoc IResourceDatabase::addResource */
        void addResource( SmartPtr<IResource> resource ) override;

        /** @copydoc IResourceDatabase::removeResource */
        void removeResource( SmartPtr<IResource> resource ) override;

        /** @copydoc IResourceDatabase::removeResourceFromPath */
        void removeResourceFromPath( const String &path ) override;

        /**
         * @copydoc IResourceDatabase::findResource
         *
         * Returns a smart pointer to the resource of the given `type` at
         * `path`, or a null pointer if none exists.
         */
        SmartPtr<IResource> findResource( u32 type, const String &path ) override;

        /**
         * @copydoc IResourceDatabase::cloneResource
         *
         * Creates a duplicate of `resource` with a new path. The `type`
         * parameter ensures the cloned instance matches the expected resource
         * type identifier.
         */
        SmartPtr<IResource> cloneResource( u32 type, SmartPtr<IResource> resource,
                                           const String &path ) override;

        /** @copydoc IResourceDatabase::importFolder */
        void importFolder( SmartPtr<IFolderExplorer> folderListing, bool overwrite = false ) override;

        /**
         * @copydoc IResourceDatabase::importFolder
         *
         * Convenience overload that takes a filesystem path and walks the
         * folder to import contained assets.
         */
        void importFolder( const String &path, bool overwrite = false ) override;

        /**
         * @copydoc IResourceDatabase::importFile
         *
         * Import a single file into the resource database. If `overwrite` is
         * true an existing resource at the same path will be replaced.
         */
        void importFile( const String &filePath, bool overwrite = false ) override;

        /** @copydoc IResourceDatabase::importCache */
        void importCache() override;

        /** @copydoc IResourceDatabase::importAssets */
        void importAssets() override;

        /** @copydoc IResourceDatabase::reimportAssets */
        void reimportAssets() override;

        /** @copydoc IResourceDatabase::calculateDependencies */
        void calculateDependencies() override;

        /** @copydoc IResourceDatabase::getResourceData */
        Array<SmartPtr<IBuildDirector>> getResourceData() const override;

        /** @copydoc IResourceDatabase::getResources */
        Array<SmartPtr<IResource>> getResources() const override;

        /**
         * @brief Create a new actor from the provided properties and attach it
         * to `parent` in the in-memory scene representation.
         *
         * This helper is used when constructing scenes or previewing assets
         * loaded via the resource database.
         */
        void createActor( SmartPtr<scene::IGameActor> parent, SmartPtr<Properties> properties );

        /** @copydoc IResourceDatabase::loadResource */
        SmartPtr<IResource> loadResource( const UUID &id ) override;

        /** @copydoc IResourceDatabase::loadResource */
        SmartPtr<IResource> loadResource( const String &path ) override;

        /**
         * @copydoc IResourceDatabase::loadDirector
         *
         * Load a scene director from a file path on disk.
         */
        SmartPtr<IBuildDirector> loadDirector( const String &filePath ) override;

        /** @copydoc IResourceDatabase::loadDirectorFromResourcePath */
        SmartPtr<IBuildDirector> loadDirectorFromResourcePath( const String &filePath ) override;

        /** @copydoc IResourceDatabase::loadDirectorFromResourcePath */
        SmartPtr<IBuildDirector> loadDirectorFromResourcePath( const String &filePath,
                                                               u32 type ) override;

        /** @copydoc IResourceDatabase::loadDirector */
        SmartPtr<IBuildDirector> loadDirector( SmartPtr<IResource> resource ) override;

        /** @copydoc IResourceDatabase::loadResourceById */
        SmartPtr<IResource> loadResourceById( const UUID &uuid ) override;

        /** @copydoc IResourceDatabase::unloadUnusedResources */
        void unloadUnusedResources() override;

        /** @copydoc IResourceDatabase::removeUnusedResources */
        void removeUnusedResources() override;

        /** @copydoc IResourceDatabase::getDatabaseManager */
        SmartPtr<IDatabaseManager> getDatabaseManager() const override;

        /** @copydoc IResourceDatabase::setDatabaseManager */
        void setDatabaseManager( SmartPtr<IDatabaseManager> databaseManager ) override;

        /** @copydoc IResourceDatabase::createOrRetrieveFromDirector */
        Pair<SmartPtr<IResource>, bool> createOrRetrieveFromDirector(
            hash_type type, const String &path, SmartPtr<IBuildDirector> director ) override;

        /** @copydoc IResourceDatabase::createOrRetrieve */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( hash_type type, const String &path ) override;

        /** @copydoc IResourceDatabase::createOrRetrieve */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

        /**
         * @brief Retrieve a shared object by UUID from the database's in-memory
         * object list.
         */
        SmartPtr<ISharedObject> getObject( const UUID &uuid ) override;

        /**
         * @brief Find a shared object by its file identifier string.
         * @return Shared object or null if not found.
         */
        SmartPtr<ISharedObject> getObjectByFileId( const String &fileId ) const override;

        /** Get a pointer to the internal resource instance array. */
        SharedPtr<Array<SmartPtr<IResource>>> getInstancesPtr() const;
        void setInstancesPtr( SharedPtr<Array<SmartPtr<IResource>>> instances );

        /** @copydoc IResourceDatabase::getFilePath */
        String getFilePath() const override;

        /** @copydoc IResourceDatabase::setFilePath */
        void setFilePath( const String &filePath ) override;

        /** @copydoc IResourceDatabase::createDatabase */
        void createDatabase() override;

        /** @copydoc IResourceDatabase::destroyDatabase */
        void destroyDatabase() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Load asset properties for a node from the database.
         * @param[in] db Database manager used to query asset data.
         * @param[in] parent Optional parent properties used when constructing
         * the in-memory properties hierarchy.
         * @param[in] id UUID of the asset to load.
         * @return Constructed Properties for the asset, or null on error.
         */
        SmartPtr<Properties> loadAssetNode( SmartPtr<IDatabaseManager> db, SmartPtr<Properties> parent,
                                            const UUID &id );

        /**
         * @brief Retrieve all objects that represent scene contents.
         * @return Array of shared objects that make up scene root objects.
         */
        Array<SmartPtr<ISharedObject>> getSceneObjects() const;

        /**
         * @brief Retrieve referenced objects associated with resources.
         * @return Array of shared objects referenced by resources.
         */
        Array<SmartPtr<ISharedObject>> getReferenceObjects() const;

        /**
         * @brief Create or retrieve a resource using a prepared database query.
         * @param[in] query Database query describing the resource to load or create.
         * @param[in] bLoadResource If true, the resource implementation will be loaded.
         * @return Smart pointer to the resource instance.
         */
        SmartPtr<IResource> createOrRetrieveResource( SmartPtr<IDatabaseQuery> query,
                                                      bool bLoadResource );

        /**
         * @brief Load asset properties by UUID from the database.
         * @param[in] id UUID of the asset.
         * @return Properties describing the asset.
         */
        SmartPtr<Properties> loadAssetProperties( const UUID &id );

        /**
         * @brief Recursively collect scene objects from an actor into `objects`.
         * @param[in] actor Actor to traverse.
         * @param[out] objects Accumulator for collected shared objects.
         */
        void getSceneObjects( SmartPtr<scene::IGameActor> actor,
                              Array<SmartPtr<ISharedObject>> &objects ) const;

        /**
         * @brief Map of resource type hash to a map of resource path -> weak pointer.
         *
         * This structure provides fast lookup of resources by type and path.
         */
        std::unordered_map<hash_type, std::unordered_map<String, WeakPtr<IResource>>> m_resourceMap;

        /** Shared pointer to the array containing all resource instances. */
        SharedPtr<Array<SmartPtr<IResource>>> m_instances;

        /** In-memory list of shared objects (scene nodes, references, etc.). */
        Array<SmartPtr<ISharedObject>> m_objects;

        /** Database manager used for persistence and queries. */
        SmartPtr<IDatabaseManager> m_databaseManager;

        /** File path to the resource database on disk. */
        String m_filePath;

        /** Recursive mutex protecting internal containers for multi-threaded use. */
        mutable RecursiveMutex m_mutex;

        /** Stores the number of import jobs currently running. */
        static atomic_s32 m_numJobs;
    };
}  // namespace workphone

#endif  // CResourceDatabase_h__
