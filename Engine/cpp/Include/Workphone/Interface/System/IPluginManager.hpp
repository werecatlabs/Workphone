#ifndef __IPluginManager_H__
#define __IPluginManager_H__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @brief Interface for managing dynamically loaded plugins (shared libraries).
     *
     * The IPluginManager interface provides methods for loading, unloading, and managing plugins at
     * runtime. Plugins are typically shared libraries (DLLs or .so files) that extend the application's
     * functionality.
     *
     * This interface allows for loading plugins by filename, loading pre-instantiated plugin objects,
     * unloading plugins, and managing the collection of loaded plugins. All plugins are managed as
     * shared objects for lifetime and memory safety.
     *
     * @see IPlugin, ISharedObject
     */
    class WPCore_API IPluginManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe cleanup of derived plugin manager objects.
         */
        ~IPluginManager() override;

        /**
         * @brief Loads a plugin from the specified library filename.
         *
         * This method loads a shared library (plugin) from the given filename and returns a smart
         * pointer to the loaded plugin interface.
         *
         * @param filename The name or path of the plugin library to load (e.g., "myplugin.dll" or
         * "libmyplugin.so").
         * @return SmartPtr<IPlugin> A smart pointer to the loaded plugin interface, or nullptr if
         * loading fails.
         *
         * @see IPlugin
         */
        virtual SmartPtr<IPlugin> loadPlugin( const String &filename ) = 0;

        /**
         * @brief Registers and loads a plugin from an existing plugin object.
         *
         * This method allows the manager to take ownership of a pre-instantiated plugin object,
         * typically used for plugins that are created or configured outside the manager's standard
         * loading mechanism.
         *
         * @param plugin A smart pointer to the plugin object to register and load.
         */
        virtual void loadPlugin( SmartPtr<IPlugin> plugin ) = 0;

        /**
         * @brief Unloads the specified plugin and releases its resources.
         *
         * This method unloads the given plugin, releasing any resources and detaching it from the
         * manager.
         *
         * @param plugin A smart pointer to the plugin to unload.
         */
        virtual void unloadPlugin( SmartPtr<IPlugin> plugin ) = 0;

        /**
         * @brief Gets the list of currently loaded plugins.
         *
         * @return Array<SmartPtr<ISharedObject>> An array of smart pointers to the loaded plugin shared
         * objects.
         */
        virtual Array<SmartPtr<ISharedObject>> getPlugins() const = 0;

        /**
         * @brief Sets the list of managed plugins.
         *
         * This method replaces the current list of managed plugins with the provided array.
         *
         * @param plugins An array of smart pointers to plugin shared objects to manage.
         */
        virtual void setPlugins( Array<SmartPtr<ISharedObject>> plugins ) = 0;

        /**
         * @brief Macro or declaration for class registration and reflection (implementation-specific).
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
