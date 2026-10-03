#ifndef IVideoMaterial_h__
#define IVideoMaterial_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Interface for video material management in the rendering system.
         *
         * This interface provides functionality for handling video materials in the rendering pipeline.
         * It inherits from ISharedObject to support shared memory management and object lifecycle
         * control. Video materials are used to apply video textures to 3D objects in the scene.
         */
        class WPCore_API IVideoMaterial : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived class resources.
             */
            ~IVideoMaterial() override;

            /**
             * @brief Retrieves the underlying video material object.
             *
             * @param object Pointer to store the retrieved video material object.
             *              The caller is responsible for managing the lifetime of the returned object.
             */
            virtual void _getObject( void **object ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVideoMaterial_h__
