#ifndef _FBFactoryManager_H_
#define _FBFactoryManager_H_

#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /**
     * @class FactoryManager
     * @brief Manages a collection of factories for object creation and pooling.
     *
     * This class implements the IFactoryManager interface and is responsible for
     * registering, retrieving, and managing factories. It provides thread-safe
     * operations for adding, removing, and querying factories, as well as creating
     * objects by type or name. It also supports tagging factories for advanced
     * categorization and filtering.
     */
    class WPCore_API FactoryManager : public IFactoryManager
    {
    public:
        /**
         * @brief Default constructor.
         * Initializes the FactoryManager and its internal data structures.
         */
        FactoryManager();

        /**
         * @brief Deleted copy constructor.
         * Copying FactoryManager is not allowed.
         * @param other The other FactoryManager instance (unused).
         */
        FactoryManager( FactoryManager &other ) = delete;

        /**
         * @brief Destructor.
         * Cleans up resources and registered factories.
         */
        ~FactoryManager() override;

        /**
         * @copydoc ISharedObject::load
         * @brief Loads the factory manager with the provided data.
         * @param data Shared object containing initialization data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc ISharedObject::unload
         * @brief Unloads the factory manager and releases resources.
         * @param data Shared object containing unload data.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IFactoryManager::allocateData
         * @brief Allocates internal data structures for factory management.
         */
        void allocateData() override;

        /**
         * @copydoc IFactoryManager::freeData
         * @brief Frees internal data structures and clears factories.
         */
        void freeData() override;

        /**
         * @copydoc IFactoryManager::addFactory
         * @brief Registers a new factory with the manager.
         * @param factory The factory to add.
         */
        void addFactory( SmartPtr<IFactory> factory ) override;

        /**
         * @copydoc IFactoryManager::removeFactory
         * @brief Removes a factory from the manager.
         * @param factory The factory to remove.
         * @return True if the factory was removed, false otherwise.
         */
        bool removeFactory( SmartPtr<IFactory> factory ) override;

        /**
         * @copydoc IFactoryManager::removeAllFactories
         * @brief Removes all registered factories from the manager.
         */
        void removeAllFactories() override;

        /**
         * @copydoc IFactoryManager::getFactoryByName
         * @brief Retrieves a factory by its name.
         * @param name The name of the factory.
         * @return The factory with the specified name, or nullptr if not found.
         */
        SmartPtr<IFactory> getFactoryByName( const String &name ) const override;

        /**
         * @copydoc IFactoryManager::getFactoryByHash
         * @brief Retrieves a factory by its hash value.
         * @param hash The hash value of the factory.
         * @return The factory with the specified hash, or nullptr if not found.
         */
        SmartPtr<IFactory> getFactoryByHash( hash64 hash ) const override;

        /**
         * @copydoc IFactoryManager::getFactoryById
         * @brief Retrieves a factory by its ID.
         * @param id The ID of the factory.
         * @return The factory with the specified ID, or nullptr if not found.
         */
        SmartPtr<IFactory> getFactoryById( u32 id ) const override;

        /**
         * @copydoc IFactoryManager::findFactoryByName
         * @brief Finds a factory by its name without throwing an error if not found.
         * @param name The name of the factory to find.
         * @return The factory with the specified name, or nullptr if not found.
         */
        SmartPtr<IFactory> findFactoryById( u32 id ) const override;

        /**
         * @copydoc IFactoryManager::hasFactoryByName
         * @brief Checks if a factory with the given name exists.
         * @param typeName The name to check.
         * @return True if the factory exists, false otherwise.
         */
        bool hasFactoryByName( const String &typeName ) const override;

        /**
         * @copydoc IFactoryManager::hasFactoryByType
         * @brief Checks if a factory with the given type exists.
         * @param type The type to check.
         * @return True if the factory exists, false otherwise.
         */
        bool hasFactoryByType( const String &type ) const override;

        /**
         * @copydoc IFactoryManager::hasFactoryById
         * @brief Checks if a factory with the given ID exists.
         * @param typeId The ID to check.
         * @return True if the factory exists, false otherwise.
         */
        bool hasFactoryById( u32 typeId ) const override;

        /**
         * @copydoc IFactoryManager::getFactories
         * @brief Returns an array of all registered factories.
         * @return Array of all factories.
         */
        Array<SmartPtr<IFactory>> getFactories() const override;

        /**
         * @copydoc IFactoryManager::createById
         * @brief Creates an object using the factory with the specified type ID.
         * @param typeId The type ID of the factory.
         * @return A shared pointer to the created object, or nullptr if creation failed.
         */
        SmartPtr<ISharedObject> createById( u32 typeId ) const override;

        /**
         * @copydoc IFactoryManager::createById
         * @brief Creates an object using the factory with the specified type ID and hint.
         * @param typeId The type ID of the factory.
         * @param hint A string hint to assist in object creation.
         * @return A shared pointer to the created object, or nullptr if creation failed.
         */
        SmartPtr<ISharedObject> createById( u32 typeId, const String &hint ) const override;

        /**
         * @copydoc IFactoryManager::setPoolSize
         * @brief Sets the pool size for a specific factory type.
         * @param typeId The type ID of the factory.
         * @param size The desired pool size.
         */
        void setPoolSize( u32 typeId, size_t size ) override;

        /**
         * @copydoc IFactoryManager::compareTags
         * @brief Compares the tags of two factories for equality.
         * @param factory1 The first factory.
         * @param factory2 The second factory.
         * @return True if the tags are equal, false otherwise.
         */
        bool compareTags( const SmartPtr<IFactory> &factory1,
                          const SmartPtr<IFactory> &factory2 ) override;

        /**
         * @copydoc IFactoryManager::hasTag
         * @brief Checks if a factory has a specific tag.
         * @param factory The factory to check.
         * @param tag The tag to look for.
         * @return True if the factory has the tag, false otherwise.
         */
        bool hasTag( const SmartPtr<IFactory> &factory, const String &tag ) override;

        /**
         * @copydoc IFactoryManager::addTag
         * @brief Adds a tag to a factory.
         * @param factory The factory to tag.
         * @param tag The tag to add.
         */
        void addTag( SmartPtr<IFactory> &factory, const String &tag ) override;

        /**
         * @copydoc IFactoryManager::removeTag
         * @brief Removes a tag from a factory.
         * @param factory The factory to untag.
         * @param tag The tag to remove.
         */
        void removeTag( SmartPtr<IFactory> &factory, const String &tag ) override;

        /**
         * @copydoc IFactoryManager::getFactoriesWithTag
         * @brief Retrieves all factories that have a specific tag.
         * @param tag The tag to filter factories by.
         * @return Array of factories with the specified tag.
         */
        Array<SmartPtr<IFactory>> getFactoriesWithTag( const String &tag ) override;

        /**
         * @copydoc IFactoryManager::lock
         */
        void lock() override;

        /**
         * @copydoc IFactoryManager::try_lock
         */
        bool try_lock() override;

        /**
         * @copydoc IFactoryManager::unlock
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Gets the unload priority for a factory.
         * @param factory The factory to query.
         * @return The unload priority as an integer.
         */
        s32 getFactoryUnloadPriority( SmartPtr<IFactory> factory );

        /**
         * @brief Thread-safe array of registered factories.
         */
        ConcurrentArray<SmartPtr<IFactory>> m_factories;

        /**
         * @brief If true, factories are auto-created when requested and not found.
         */
        atomic_bool m_autoCreateFactory = false;

        /**
         * @brief Enables or disables debug mode for the factory manager.
         */
        atomic_bool m_bEnableDebug = false;

        /**
         * @brief Recursive mutex to protect access to the factory manager's internal data structures.
         */
        mutable RecursiveSpinMutex m_mutex;
    };
}  // namespace workphone

#endif
