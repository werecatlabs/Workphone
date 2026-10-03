#ifndef _CResourceGroupManager_H
#define _CResourceGroupManager_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/Core/Set.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CResourceGroupManager
         * @brief Wrapper that adapts Ogre's resource group management to the engine's IResourceGroupManager interface.
         *
         * This class provides methods to load, initialise, unload, clear and destroy named resource groups,
         * parse resource scripts and reload resources. It holds pointers to Ogre-specific helpers and the
         * underlying Ogre::ResourceGroupManager and exposes the wrapped native object through `_getObject`.
         *
         * @note This is a thin wrapper � most heavy lifting is delegated to Ogre and the ResourceGroupHelper.
         */
        class CResourceGroupManager : public IResourceGroupManager
        {
        public:
            /** Notification ID used when a resource group is unloaded. */
            static const u32 RGMID_UNLOADRESOURCEGROUP;

            /** Default constructor. */
            CResourceGroupManager();

            /** Virtual destructor. Releases owned resources and detaches listeners. */
            ~CResourceGroupManager() override;

            /**
             * @copydoc IResourceGroupManager::load
             * @param data Smart pointer to an ISharedObject that contains data required for loading.
             *             Typically this will contain configuration or a description of resources to
             * load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IResourceGroupManager::unload
             * @param data Smart pointer to an ISharedObject that contains data required for unloading.
             *             Typically will identify which group(s) or resources to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Initialise all registered resource groups.
             *
             * Calls into Ogre to initialise every resource group that has been registered but not yet initialised.
             */
            void initialiseAllResourceGroups() override;

            /**
             * @brief Initialise a specific resource group by name.
             * @param groupName Name of the resource group to initialise.
             */
            void initialiseResourceGroup( const String &groupName ) override;

            /**
             * @brief Unload all resources in the specified resource group.
             * @param groupName Name of the resource group to unload.
             *
             * @note Unloading keeps the group itself registered but frees resource memory.
             */
            void unloadResourceGroup( const String &groupName ) override;

            /**
             * @brief Clear resource group contents (remove resource entries created at runtime).
             * @param groupName Name of the resource group to clear.
             *
             * @note This removes resources from the group but does not necessarily destroy the group.
             */
            void clearResourceGroup( const String &groupName ) override;

            /**
             * @brief Destroy a resource group completely.
             * @param groupName Name of the resource group to destroy.
             *
             * @note After destruction the group must be re-registered before use.
             */
            void destroyResourceGroup( const String &groupName ) override;

            /**
             * @brief Obtain the underlying native object.
             * @param ppObject Pointer to a void* which will receive the native object pointer (Ogre object).
             *
             * @note Caller must pass a valid pointer to a `void*`. The function does not transfer ownership.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Reload resources belonging to the named group.
             * @param groupName Name of the resource group to reload.
             *
             * This typically re-reads resource data (e.g. textures, materials, shaders) so that
             * changes on disk or from editing tools become visible at runtime.
             */
            void reloadResources( const String &groupName ) override;

            /**
             * @brief Parse a collection of script file names and load definitions into their groups.
             * @param scripts Array of script file paths or names to parse.
             */
            void parseScripts( const Array<String> &scripts ) override;

            /**
             * @brief Parse a set of script file names and load definitions into their groups.
             * @param scripts Set of script file paths or names to parse.
             */
            void parseScripts( const Set<String> &scripts );

            /**
             * @brief Get the engine state context associated with this manager.
             * @return Smart pointer to the IStateContext instance, or null if none set.
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Set the engine state context to use for notifications and state tracking.
             * @param stateContext Smart pointer to an IStateContext instance.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

        protected:
            /**
             * @brief Parse scripts belonging to a resource manager for a given extension and group.
             * @param resMgr Pointer to an Ogre::ResourceManager that owns the scripts.
             * @param extension File extension (for example "material", "particle").
             * @param group Resource group name these scripts belong to.
             *
             * This overload is used for resource managers that provide script enumeration or loading APIs.
             */
            void parseScripts( Ogre::ResourceManager *resMgr, const String &extension,
                               const String &group );

            /**
             * @brief Parse particle system scripts from the provided particle system manager.
             * @param resMgr Pointer to an Ogre::ParticleSystemManager.
             * @param extension File extension for particle scripts.
             * @param group Resource group name.
             */
            void parseScripts( Ogre::ParticleSystemManager *resMgr, const String &extension,
                               const String &group );

            /**
             * @brief Parse scripts using a generic Ogre::ScriptLoader.
             * @param loader Script loader instance used to read/interpret script contents.
             * @param extension File extension to filter by.
             * @param group Resource group name.
             */
            void parseScripts( Ogre::ScriptLoader *loader, const String &extension,
                               const String &group );

            /**
             * @brief Parse a single script file.
             * @param loader Script loader used to parse the file.
             * @param fileName Script file name or path.
             * @param group Resource group name the script belongs to.
             */
            void parseScript( Ogre::ScriptLoader *loader, const String &fileName, const String &group );

            /** Helper object that contains convenience functions for resource group operations. */
            ResourceGroupHelper *m_resourceGroupHelper = nullptr;

            /** Pointer to the underlying Ogre ResourceGroupManager instance. */
            Ogre::ResourceGroupManager *m_resourceGroupManager = nullptr;

            /** Optional state context used for integration with the engine's state system. */
            SmartPtr<IStateContext> m_stateContext;

            /** Optional listener for state change events associated with this manager. */
            SmartPtr<IStateListener> m_stateListener;
        };
    }  // end namespace render
}  // namespace workphone

#endif
