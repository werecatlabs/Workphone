#ifndef _IResourceManager_H
#define _IResourceManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    /** Interface for a resource manager. */
    class WPCore_API IResourceManager : public ISharedObject
    {
    public:
        IResourceManager();

        IResourceManager( u32 poolTypeId );

        /** Virtual destructor. */
        ~IResourceManager() override;

        /** Creates a resource.
         * @param name The name of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> create( const String &name ) = 0;

        /** Creates a resource.
         * @param name The name of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> create( const String &uuid, const String &name ) = 0;

        /** Creates a resource or retrieves an existing a resource.
         * @param uuid The uuid of the resource.
         * @param path The path of the resource.
         * @param type The type of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                                  const String &type ) = 0;

        /** Creates a resource or retrieves an existing a resource.
         * @param path The path of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) = 0;

        /** Creates a resource or retrieves an existing a resource by type.
         * @tparam T The type of the resource.
         * @param uuid The uuid of the resource.
         * @param path The path of the resource.
         * @param type The type of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        template <typename T>
        Pair<SmartPtr<T>, bool> createOrRetrieveByType( const String &uuid, const String &path,
                                                        const String &type );

        /** Creates a resource or retrieves an existing a resource by type.
         * @tparam T The type of the resource.
         * @param path The path of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        template <typename T>
        Pair<SmartPtr<T>, bool> createOrRetrieveByType( const String &path );

        /**
         * @brief Destroy a resource in the manager.
         * @param resource The resource to remove.
         */
        virtual void destroyResource( SmartPtr<IResource> resource ) = 0;

        /**
         * @brief Destroys all the resources in the manager.
         */
        virtual void destroyAll() = 0;

        /** Saves a resource to file.
        @param filePath The filePath of the resource.
        @param resource The resource instance.
        */
        virtual void saveToFile( const String &filePath, SmartPtr<IResource> resource ) = 0;

        /** Loads a resource from a file.
        @param filePath The file path of the resource.
        @returns The resource instance. Can be null if the resource does not exist.
        */
        virtual SmartPtr<IResource> loadFromFile( const String &filePath ) = 0;

        /** Loads a resource.
        @param name The name of the resource.
        @returns The resource instance. Can be null if the resource does not exist.
        */
        virtual SmartPtr<IResource> loadResource( const String &name ) = 0;

        /**
         * @brief Handles incoming state messages.
         * @param message The state message to handle
         * @return True if the message was handled, false otherwise
         */
        virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

        /**
         * @brief Handles state transitions.
         * Called when the material's state changes, allowing the listener
         * to respond to loading state transitions and resource changes.
         * @param state Reference to the new state
         * @return True if the state change was handled successfully
         */
        virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

        /** Loads a resource.
         * @tparam T The type of the resource.
         * @param name The name of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        template <typename T>
        SmartPtr<T> loadResourceByType( const String &name );

        /** Gets an existing resource.
         * @param name The name of the resource.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> getByName( const String &name ) = 0;

        /** Gets an existing resource by passing the hash id.
         * @param uuid The name of the resource as a hash value.
         * @returns The resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> getById( const String &uuid ) = 0;

        /**
         * @brief Get the raw pointer to the attached IStateContext.
         * Useful when C-style pointer access is required. The returned pointer is not
         * accompanied by ownership guarantees; prefer getStateContext() for ownership.
         * @return Raw IStateContext pointer or nullptr if none set.
         */
        virtual IStateContext *getStateContextPtr() const = 0;

        /**
         * @brief Get the SmartPtr to the attached IStateContext.
         * Returns the internal smart pointer that owns the state context.
         * @return SmartPtr<IStateContext> reference (may be nullptr).
         */
        virtual SmartPtr<IStateContext> getStateContext() const = 0;

        /**
         * @brief Attach a state context to this object.
         * The provided SmartPtr will be stored and later unloaded/removed by
         * destroyStateContext() during object unload.
         * @param stateContext Smart pointer to the state context to attach.
         */
        virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

        /** Clone a resource.
         * @param resource The resource to clone.
         * @param clonedResourceName The name of the cloned resource.
         * @returns The cloned resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                                   const String &clonedResourceName ) = 0;

        /** Clone a resource.
         * @param name The name of the resource.
         * @param clonedResourceName The name of the cloned resource.
         * @returns The cloned resource instance. Can be null if the resource does not exist.
         */
        virtual SmartPtr<IResource> cloneResource( const String &name,
                                                   const String &clonedResourceName ) = 0;

        /** Clone a resource by type.
         * @tparam T The type of the resource.
         * @param resource The resource to clone.
         * @param clonedResourceName The name of the cloned resource.
         * @returns The cloned resource instance. Can be null if the resource does not exist.
         */
        template <typename T>
        SmartPtr<T> cloneResourceByType( SmartPtr<IResource> resource,
                                         const String &clonedResourceName );

        /** Gets a pointer to the underlying object.
         * @param ppObject A pointer to store the object.
         */
        virtual void _getObject( void **ppObject ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

    template <typename T>
    Pair<SmartPtr<T>, bool> IResourceManager::createOrRetrieveByType( const String &uuid,
                                                                      const String &path,
                                                                      const String &type )
    {
        auto pair = createOrRetrieve( uuid, path, type );
        auto resource = workphone::static_pointer_cast<T>( pair.first );
        return Pair<SmartPtr<T>, bool>( resource, pair.second );
    }

    template <typename T>
    Pair<SmartPtr<T>, bool> IResourceManager::createOrRetrieveByType( const String &path )
    {
        auto pair = createOrRetrieve( path );
        auto resource = workphone::static_pointer_cast<T>( pair.first );
        return Pair<SmartPtr<T>, bool>( resource, pair.second );
    }

    template <typename T>
    SmartPtr<T> IResourceManager::loadResourceByType( const String &name )
    {
        auto resource = loadResource( name );
        return workphone::static_pointer_cast<T>( resource );
    }

    template <typename T>
    SmartPtr<T> IResourceManager::cloneResourceByType( SmartPtr<IResource> resource,
                                                       const String &clonedTextureName )
    {
        auto clonedResource = cloneResource( resource, clonedTextureName );
        return workphone::static_pointer_cast<T>( clonedResource );
    }

}  // namespace workphone

#endif
