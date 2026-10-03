#ifndef ClawResourceGroupManager_h__
#define ClawResourceGroupManager_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/System/IResourceGroupManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawResourceGroupManager
         * @brief Minimal resource group manager for the ClawHammer backend.
         *
         * The Claw software renderer does not drive resource group initialisation
         * through an external library (unlike the Ogre backends). This implementation
         * satisfies the IResourceGroupManager contract with safe no-op behaviour so the
         * engine's graphics lifecycle (which calls getResourceGroupManager()/load()) can
         * complete without a backend-specific resource system in place.
         */
        class WPGraphics_API ClawResourceGroupManager : public IResourceGroupManager
        {
        public:
            /** @brief Default constructor. */
            ClawResourceGroupManager();

            /** @brief Destructor. */
            ~ClawResourceGroupManager() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
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

            /** @copydoc IResourceGroupManager::getStateContext */
            SmartPtr<IStateContext> getStateContext() const override;

            /** @copydoc IResourceGroupManager::setStateContext */
            void setStateContext( SmartPtr<IStateContext> stateContext ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Optional state context associated with the resource group manager. */
            SmartPtr<IStateContext> m_stateContext;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawResourceGroupManager_h__
