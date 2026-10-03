#ifndef __IPrefab_h__
#define __IPrefab_h__

#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * Interface for a prefab. Used to construct actors using pre-defined data.
         */
        class WPCore_API IGamePrefab : public IResource
        {
        public:
            IGamePrefab() : IResource( IGamePrefab::typeInfo() )
            {
            }

            IGamePrefab( u32 poolTypeId ) : IResource( poolTypeId )
            {
            }

            /**
             * Virtual destructor.
             */
            ~IGamePrefab() override;

            /**
             * Creates an actor instance.
             * @return The created actor instance.
             */
            virtual SmartPtr<IGameActor> createActor() = 0;

            /**
             * Gets the data associated with this prefab.
             * @return The data.
             */
            virtual SmartPtr<ISharedObject> getData() const = 0;

            /**
             * Sets the data associated with this prefab.
             * @param data The data to set.
             */
            virtual void setData( SmartPtr<ISharedObject> data ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // IResource_h__
