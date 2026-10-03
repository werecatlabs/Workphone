#ifndef __PluginManager_h__
#define __PluginManager_h__

#include <Workphone/Interface/System/IPluginManager.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace core
    {
        /** Implementation for a plugin manager. */
        class WPCore_API PluginManager : public IPluginManager
        {
        public:
            /** Constructor. */
            PluginManager();

            /** Destructor. */
            ~PluginManager() override;

            /** @copydoc IPluginManager::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPluginManager::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPluginManager::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IPluginManager::loadPlugin */
            SmartPtr<IPlugin> loadPlugin( const String &filename ) override;

            /** @copydoc IPluginManager::loadPlugin */
            void loadPlugin( SmartPtr<IPlugin> plugin ) override;

            /** @copydoc IPluginManager::unloadPlugin */
            void unloadPlugin( SmartPtr<IPlugin> plugin ) override;

            /** @copydoc IPluginManager::getPlugins */
            Array<SmartPtr<ISharedObject>> getPlugins() const override;

            /** @copydoc IPluginManager::setPlugins */
            void setPlugins( Array<SmartPtr<ISharedObject>> plugins ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Array of loaded plugins. */
            Array<SmartPtr<ISharedObject>> m_plugins;

            /** Mutex for thread safety. */
            mutable RecursiveMutex m_mutex;
        };
    }  // namespace core
}  // namespace workphone

#endif  // PluginManager_h__
