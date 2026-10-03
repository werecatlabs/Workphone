#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawResourceGroupManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawResourceGroupManager, IResourceGroupManager );

        ClawResourceGroupManager::ClawResourceGroupManager()
        {
            static const auto name = String( "ClawResourceGroupManager" );
            setName( name );
        }

        ClawResourceGroupManager::~ClawResourceGroupManager()
        {
        }

        void ClawResourceGroupManager::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );
            IResourceGroupManager::load( data );
            setLoadingState( LoadingState::Loaded );
        }

        void ClawResourceGroupManager::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );
            m_stateContext = nullptr;
            IResourceGroupManager::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        void ClawResourceGroupManager::initialiseAllResourceGroups()
        {
            // No backend resource system to initialise for the Claw renderer.
        }

        void ClawResourceGroupManager::initialiseResourceGroup( const String &groupName )
        {
            (void)groupName;
        }

        void ClawResourceGroupManager::unloadResourceGroup( const String &groupName )
        {
            (void)groupName;
        }

        void ClawResourceGroupManager::clearResourceGroup( const String &groupName )
        {
            (void)groupName;
        }

        void ClawResourceGroupManager::destroyResourceGroup( const String &groupName )
        {
            (void)groupName;
        }

        void ClawResourceGroupManager::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = nullptr;
            }
        }

        void ClawResourceGroupManager::reloadResources( const String &groupName )
        {
            (void)groupName;
        }

        void ClawResourceGroupManager::parseScripts( const Array<String> &scripts )
        {
            (void)scripts;
        }

        SmartPtr<IStateContext> ClawResourceGroupManager::getStateContext() const
        {
            return m_stateContext;
        }

        void ClawResourceGroupManager::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }
    }  // namespace render
}  // namespace workphone
