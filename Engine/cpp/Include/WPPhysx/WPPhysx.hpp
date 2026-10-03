#ifndef __WPPhysx_h__
#define __WPPhysx_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

/**
 * @file WPPhysx.hpp
 * @brief PhysX plugin entry and shared-object wrapper.
 *
 * This header declares the `WPPhysx` class which implements the
 * `ISharedObject` interface and acts as the module entry point for the
 * PhysX integration. It exposes lifecycle methods as well as global
 * accessors for the plugin instance and the factory manager used by
 * the plugin.
 */

namespace workphone
{
    namespace physics
    {

        /**
         * @brief PhysX plugin shared object.
         *
         * `WPPhysx` represents the PhysX plugin and is responsible for
         * handling load/unload lifecycle events. The class exposes a
         * singleton-style static instance accessor and allows a
         * factory manager to be associated with the plugin.
         */
        class WP_PHYSX_API WPPhysx : public ISharedObject
        {
        public:
            /**
             * @brief Construct a new WPPhysx plugin object.
             *
             * Constructor should perform lightweight initialization; heavy
             * initialization should be deferred to `load`.
             */
            WPPhysx();

            /**
             * @brief Destroy the WPPhysx plugin object.
             *
             * Destructor should ensure resources are released. Plugin
             * teardown should normally happen in `unload`.
             */
            ~WPPhysx() override;

            /**
             * @brief Called when the plugin is loaded by the host.
             *
             * @param data Optional data provided by the loader. The meaning
             * of this data is plugin-specific and may be null.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called when the plugin is unloaded by the host.
             *
             * @param data Optional data provided by the unloader. This may
             * be used to pass context back to the plugin during shutdown.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the global plugin instance.
             *
             * The plugin follows a singleton-style pattern for global
             * access; this returns the currently set instance. The return
             * value may be null if no instance has been set.
             *
             * @return SmartPtr<WPPhysx> Current global plugin instance.
             */
            static SmartPtr<WPPhysx> instance();

            /**
             * @brief Set the global plugin instance.
             *
             * Use this to register the plugin instance that should be
             * returned by `instance()`.
             *
             * @param plugin Plugin instance to set as the global instance.
             */
            static void setInstance( SmartPtr<WPPhysx> plugin );

            /**
             * @brief Get the factory manager associated with the plugin.
             *
             * The factory manager is used to create engine objects and
             * should be set before other subsystems that rely on it are
             * initialized.
             *
             * @return SmartPtr<IFactoryManager> Current factory manager.
             */
            static SmartPtr<IFactoryManager> getFactoryManager();

            /**
             * @brief Set the factory manager to be used by the plugin.
             *
             * @param factoryManager Factory manager to associate with the
             * plugin.
             */
            static void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

        protected:
            /**
             * @brief Global plugin instance (singleton-like).
             *
             * Stored as a SmartPtr to manage lifetime and avoid leaking the
             * plugin when the host is shutting down.
             */
            static SmartPtr<WPPhysx> m_sPlugin;

            /**
             * @brief Factory manager used by the PhysX plugin.
             *
             * This manager is responsible for creating engine-level objects
             * and is shared by subsystems that require object factories.
             */
            static SmartPtr<IFactoryManager> m_factoryManager;
        };

    } // end namespace physics
} // namespace workphone

#endif // WPPhysx_h__
