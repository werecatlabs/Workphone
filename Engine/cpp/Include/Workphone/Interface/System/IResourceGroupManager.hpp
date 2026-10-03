#ifndef _IResourceGroupManager_H
#define _IResourceGroupManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * An interface for managing resource groups.
     */
    class WPCore_API IResourceGroupManager : public ISharedObject
    {
    public:
        static const String DEFAULT_RESOURCE_GROUP_NAME;

        /** Destructor. */
        ~IResourceGroupManager() override;

        /**
         * Initializes all resource groups.
         */
        virtual void initialiseAllResourceGroups() = 0;

        /**
         * Initializes a resource group.
         *
         * @param groupName The name of the resource group.
         */
        virtual void initialiseResourceGroup( const String &groupName ) = 0;

        /**
         * Unloads a resource group.
         *
         * @param groupName The name of the resource group.
         */
        virtual void unloadResourceGroup( const String &groupName ) = 0;

        /**
         * Clears a resource group.
         *
         * @param groupName The name of the resource group.
         */
        virtual void clearResourceGroup( const String &groupName ) = 0;

        /**
         * Destroys a resource group.
         *
         * @param groupName The name of the resource group.
         */
        virtual void destroyResourceGroup( const String &groupName ) = 0;

        /**
         * Gets a pointer to the underlying object. This is dependent on the graphics library used.
         *
         * @param ppObject A pointer to the underlying object.
         */
        virtual void _getObject( void **ppObject ) const = 0;

        /**
         * Reloads resources in a resource group.
         *
         * @param groupName The name of the resource group.
         */
        virtual void reloadResources( const String &groupName ) = 0;

        /**
         * Parses scripts.
         *
         * @param scripts An array of script filenames to parse.
         */
        virtual void parseScripts( const Array<String> &scripts ) = 0;

        /**
         * Gets the state object associated with the resource group manager.
         *
         * @return The state object.
         */
        virtual SmartPtr<IStateContext> getStateContext() const = 0;

        /**
         * Sets the state object associated with the resource group manager.
         *
         * @param stateContext The state object to set.
         */
        virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
