#ifndef IResourceReference_h__
#define IResourceReference_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for managing resource references in the system.
     *
     * This interface provides functionality to track and manage references to resources
     * by maintaining UUIDs for both the resource and its owner. It inherits from
     * ISharedObject to support shared ownership semantics.
     */
    class WPCore_API IResourceReference : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IResourceReference() override;

        /**
         * @brief Gets the UUID of the owner of this resource reference.
         * @return The UUID string of the owner.
         */
        virtual String getOwnerUUID() const = 0;

        /**
         * @brief Sets the UUID of the owner for this resource reference.
         * @param ownerUUID The UUID string to set as the owner.
         */
        virtual void setOwnerUUID( const String &ownerUUID ) = 0;

        /**
         * @brief Gets the UUID of the referenced resource.
         * @return The UUID string of the resource.
         */
        virtual String getResourceUUID() const = 0;

        /**
         * @brief Sets the UUID of the resource being referenced.
         * @param resourceUUID The UUID string of the resource to reference.
         */
        virtual void setResourceUUID( const String &resourceUUID ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IResourceReference_h__
