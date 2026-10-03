#ifndef IVolumeRenderer_h__
#define IVolumeRenderer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API IVolumeRenderer : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IVolumeRenderer() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVolumeRenderer_h__
