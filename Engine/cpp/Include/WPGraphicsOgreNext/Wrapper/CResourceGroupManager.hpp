#ifndef _CResourceGroupManager_H
#define _CResourceGroupManager_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/System/Job.hpp>
#include <OgreArchive.h>

namespace workphone
{
    namespace render
    {

        /**
         * @class CResourceGroupManager
         * @brief Implementation of IResourceGroupManager that wraps OgreNext's resource management system.
         * 
         * This class handles the loading, unloading, and initialization of resource groups,
         * integrating OgreNext's resource system with the Workphone engine.
         */
        class CResourceGroupManager : public SharedGraphicsObject<IResourceGroupManager>
        {
        public:
            /**
             * @class ResourceLoadJob
             * @brief A job responsible for loading resources asynchronously.
             */
            class ResourceLoadJob : public Job
            {
            public:
                ResourceLoadJob();
                ~ResourceLoadJob() override;

                /** @brief Executes the resource loading logic. */
                void execute() override;
                /** @brief Step-based execution for coroutine-like loading. */
                void coroutine_execute_step( SmartPtr<ICoroutineData> &rYield ) override;

                /** @brief Gets the associated ResourceGroupManager. */
                CResourceGroupManager *getResourceGroupManager() const;
                /** @brief Sets the associated ResourceGroupManager. */
                void setResourceGroupManager( CResourceGroupManager *resourceGroupManager );

                WP_CLASS_REGISTER_DECL;

            private:
                CResourceGroupManager *m_resourceGroupManager = nullptr; ///< Pointer to the managing resource group manager.
            };

            static const u32 RGMID_UNLOADRESOURCEGROUP;

            CResourceGroupManager();
            ~CResourceGroupManager() override;

            /** @copydoc IResourceGroupManager::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IResourceGroupManager::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IResourceGroupManager::initialiseAllResourceGroups */
            void initialiseAllResourceGroups() override;

            /** @copydoc IResourceGroupManager::initialiseResourceGroup */
            void initialiseResourceGroup( const String &groupName ) override;

            /** @copydoc IResourceGroupManager::unloadResourceGroup */
            void unloadResourceGroup( const String &groupName ) override;

            /** @copydoc IResourceGroupManager::clearResourceGroup */
            void clearResourceGroup( const String &groupName ) override;

            /** @copydoc IResourceGroupManager::destroyResourceGroup */
            void destroyResourceGroup( const String &groupName ) override;

            /** @copydoc IResourceGroupManager::_getObject */
            void _getObject( void **ppObject ) const override;

            /** @copydoc IResourceGroupManager::reloadResources */
            void reloadResources( const String &groupName ) override;

            /** @copydoc IResourceGroupManager::parseScripts */
            void parseScripts( const Array<String> &scripts ) override;

            /** @copydoc IResourceGroupManager::parseScripts */
            void parseScripts( const Set<String> &scripts );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Parses scripts for a specific extension and group. */
            void parseScripts( const String &extension, const String &group );
            /** @brief Parses scripts using a specific Ogre ResourceManager. */
            void parseScripts( Ogre::ResourceManager *resMgr, const String &extension,
                               const String &group );
            /** @brief Parses scripts using Ogre's ParticleSystemManager. */
            void parseScripts( Ogre::ParticleSystemManager *resMgr, const String &extension,
                               const String &group );
            /** @brief Parses scripts using an Ogre ScriptLoader. */
            void parseScripts( Ogre::ScriptLoader *loader, const String &extension,
                               const String &group );

            /** @brief Parses a single script file. */
            void parseScript( Ogre::ScriptLoader *loader, const String &fileName, const String &group );

            /** @brief Loads the resource configuration file. */
            void loadResourceFile();

            /** @brief Registers base HLMS components. */
            void baseRegisterHlms();
            /** @brief Registers HLMS components. */
            void registerHlms( void );

            /** @brief Sets up the initial resource state. */
            void setupResources();

            /** @brief Loads the texture cache from disk. */
            void loadTextureCache( void );
            /** @brief Saves the texture cache to disk. */
            void saveTextureCache( void );
            /** @brief Loads the HLMS disk cache. */
            void loadHlmsDiskCache( void );
            /** @brief Saves the HLMS disk cache. */
            void saveHlmsDiskCache( void );

            /** @brief Parses an HLMS script file. */
            void parseHlmsScript( const String &filePath );
            /** @brief Initializes all defined resource groups. */
            void initialiseResourceGroups();

            FixedString<WP_MAX_PATH> m_pluginsFolder; ///< Path to the plugins directory.
            FixedString<WP_MAX_PATH> m_writeAccessFolder; ///< Path to the folder where write access is permitted.
            FixedString<WP_MAX_PATH> m_resourcePath; ///< Root path for resources.

            bool m_alwaysAskForConfig = false; ///< If true, always prompt for configuration.
            bool m_useHlmsDiskCache = false; ///< Whether to use the HLMS disk cache.
            bool m_useMicrocodeCache = false; ///< Whether to use the microcode cache.

            SharedPtr<Ogre::Archive> m_fileSystemArchive; ///< The file system archive used for resource loading.
        };
    }  // end namespace render
}  // namespace workphone

#endif
