#ifndef __WP_IResource_h__
#define __WP_IResource_h__

#include <Workphone/Interface/IPrototype.hpp>
#include <Workphone/Core/UUID.hpp>

namespace workphone
{

    /**
     * @brief Abstract interface representing a loadable and savable resource.
     *
     * IResource is the common interface for assets used by the engine (for example:
     * textures, meshes, audio clips, configuration data). Concrete resource classes
     * implement serialization, import/conversion, dependency reporting and participate
     * in the engine's resource and state management systems.
     *
     * Implementations are expected to:
     * - Provide serialization via saveToFile/loadFromFile and save.
     * - Support import and reimport operations for converting external source data.
     * - Expose a stable UUID and a file path for VFS identification.
     * - Report dependencies to allow the resource manager to order loading and
     *   track references.
     * - Integrate with IResourceManager and IStateContext for lifecycle and
     *   state-change notifications.
     *
     * @note Thread-safety semantics are implementation defined; callers should
     * consult concrete implementations for concurrency guarantees.
     *
     * @author Zane
     * @version 1.1
     */
    class WPCore_API IResource : public core::IPrototype
    {
    public:
        /**
         * @brief Static identifier string for the resource type.
         *
         * Concrete subclasses should initialize this value to a human-readable
         * name such as "Texture" or "Mesh".
         */
        static const String nameStr;

        IResource();

        IResource( u32 typeId );

        /**
         * @brief Virtual destructor to ensure proper cleanup through the interface.
         */
        ~IResource() override;

        /**
         * @brief Persist the resource to the specified file path.
         * @param filePath Filesystem path to write the resource data to.
         *
         * Implementations should serialize the resource state so that it can be
         * reconstructed by loadFromFile.
         */
        virtual void saveToFile( const String &filePath ) = 0;

        /**
         * @brief Load and deserialize the resource from the specified file path.
         * @param filePath Filesystem path to read the resource data from.
         */
        virtual void loadFromFile( const String &filePath ) = 0;

        /**
         * @brief Save the resource to its currently associated file path.
         *
         * This may be a convenience wrapper around saveToFile(getFilePath()).
         */
        virtual void save() = 0;

        /**
         * @brief Import external source data and convert it into the engine's
         * runtime representation.
         */
        virtual void import() = 0;

        /**
         * @brief Re-run import/processing for the resource, typically used when
         * source data or import settings have changed.
         */
        virtual void reimport() = 0;

        /**
         * @brief Get the persistent UUID that identifies this resource in the
         * virtual file system.
         * @return UUID associated with the resource.
         */
        virtual UUID getFileSystemId() const = 0;

        /**
         * @brief Set the persistent UUID for this resource.
         * @param id UUID to associate with the resource.
         */
        virtual void setFileSystemId( UUID id ) = 0;

        /**
         * @brief Retrieve the file path currently associated with the resource.
         * @return Path to the resource on disk (may be empty for non-file-backed resources).
         */
        virtual String getFilePath() const = 0;

        /**
         * @brief Associate a filesystem path with this resource instance.
         * @param filePath Filesystem path to associate.
         */
        virtual void setFilePath( const String &filePath ) = 0;

        /**
         * @brief Get the UUID of the settings asset associated with this resource.
         * @return UUID of the settings resource.
         */
        virtual UUID getSettingsFileSystemId() const = 0;

        /**
         * @brief Set the UUID of the settings asset associated with this resource.
         * @param id UUID of the settings resource.
         */
        virtual void setSettingsFileSystemId( UUID id ) = 0;

        /**
         * @brief Obtain a raw pointer to the concrete implementation object.
         * @param ppObject Out parameter that will receive the pointer to the
         * concrete object. The concrete type depends on the implementation.
         *
         * Use this method only when the caller needs access to implementation
         * specific APIs not exposed by IResource. Proper casting is the
         * responsibility of the caller.
         */
        virtual void _getObject( void **ppObject ) const = 0;

        /**
         * @brief Return a list of other resources that this resource depends on.
         * @return Array of smart pointers referencing dependent resources.
         */
        virtual Array<SmartPtr<IResource>> getDependencies() const = 0;

        /**
         * @brief Get a raw pointer to the owning resource manager, if any.
         * @return Raw pointer to IResourceManager or nullptr.
         */
        virtual IResourceManager *getResourceManagerPtr() const = 0;

        /**
         * @brief Get a managed pointer to the owning resource manager.
         * @return SmartPtr to IResourceManager or empty pointer.
         */
        virtual SmartPtr<IResourceManager> getResourceManager() const = 0;

        /**
         * @brief Associate this resource with a resource manager.
         * @param resourceManager Smart pointer to the manager handling this resource.
         */
        virtual void setResourceManager( SmartPtr<IResourceManager> resourceManager ) = 0;

        /**
         * @brief Get a raw pointer to the state context used by this resource.
         * @return Raw pointer to IStateContext or nullptr.
         */
        virtual IStateContext *getStateContextPtr() const = 0;

        /**
         * @brief Get a managed pointer to the state context used by this resource.
         * @return SmartPtr to IStateContext or empty pointer.
         */
        virtual SmartPtr<IStateContext> getStateContext() const = 0;

        /**
         * @brief Handle an incoming state message targeted at this resource.
         * @param message Message to handle.
         * @return true if the message was handled, false otherwise.
         */
        virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

        /**
         * @brief Called when an observed state object has changed.
         * @param state The state object that changed.
         * @return true if the resource consumed the state change, false otherwise.
         */
        virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IResource_h__
