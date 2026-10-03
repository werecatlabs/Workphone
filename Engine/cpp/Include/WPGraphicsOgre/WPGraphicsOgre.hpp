
#ifndef __WPOgreGFX__H
#define __WPOgreGFX__H

#include <WPGraphicsOgre/WPGraphicsOgreConfig.hpp>
#include <WPGraphicsOgre/WPGraphicsOgreAutoLink.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Main plugin class for the Ogre graphics system integration.
         *
         * WPGraphicsOgre serves as the primary entry point and factory for the Ogre-based
         * rendering system. It manages the lifecycle of the graphics system, provides
         * singleton access to the plugin instance, and handles factory management for
         * creating graphics-related objects.
         *
         * This class follows the singleton pattern and provides static factory methods
         * for creating and accessing the graphics system components.
         *
         * @see IGraphicsSystem
         * @see IFactoryManager
         */
        class WPGraphicsOgre_API WPGraphicsOgre : public ISharedObject
        {
        public:
            /**
             * @brief Constructs the WPGraphicsOgre plugin instance.
             */
            WPGraphicsOgre();

            /**
             * @brief Destructor that cleans up the graphics plugin resources.
             */
            ~WPGraphicsOgre() override;

            /**
             * @brief Loads the graphics plugin with the provided data.
             *
             * Initializes the Ogre graphics system and prepares it for use.
             *
             * @param data Shared pointer to initialization data for loading the plugin.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the graphics plugin and releases resources.
             *
             * Performs cleanup and shutdown of the Ogre graphics system.
             *
             * @param data Shared pointer to data required for unloading the plugin.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Factory method to create an Ogre-based graphics system instance.
             *
             * Creates and returns a new instance of the IGraphicsSystem implementation
             * using the Ogre rendering engine.
             *
             * @return Smart pointer to the newly created graphics system instance.
             */
            static SmartPtr<IGraphicsSystem> createGraphicsOgre();

            /**
             * @brief Gets the singleton instance of the WPGraphicsOgre plugin.
             *
             * @return Smart pointer to the plugin singleton instance, or nullptr if not set.
             */
            static SmartPtr<WPGraphicsOgre> instance();

            /**
             * @brief Sets the singleton instance of the WPGraphicsOgre plugin.
             *
             * @param plugin Smart pointer to the plugin instance to be set as singleton.
             */
            static void setInstance( SmartPtr<WPGraphicsOgre> plugin );

            /**
             * @brief Gets the factory manager for creating graphics objects.
             *
             * The factory manager is responsible for creating various graphics-related
             * objects such as materials, meshes, textures, and other rendering components.
             *
             * @return Smart pointer to the factory manager instance.
             */
            static SmartPtr<IFactoryManager> getFactoryManager();

            /**
             * @brief Sets the factory manager for creating graphics objects.
             *
             * @param factoryManager Smart pointer to the factory manager to be used
             *                       for object creation.
             */
            static void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

        protected:
            /** @brief Singleton instance of the WPGraphicsOgre plugin. */
            static SmartPtr<WPGraphicsOgre> m_sPlugin;

            /** @brief Factory manager for creating and managing graphics objects. */
            static SmartPtr<IFactoryManager> m_factoryManager;
        };
    }  // end namespace render
}  // namespace workphone

#endif
