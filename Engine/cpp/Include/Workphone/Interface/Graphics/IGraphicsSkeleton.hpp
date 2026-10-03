#ifndef IGraphicsSkeleton_h__
#define IGraphicsSkeleton_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a Skeleton. */
        class WPCore_API IGraphicsSkeleton : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IGraphicsSkeleton() override;

            /** Creates a brand new Bone owned by this Skeleton. */
            virtual SmartPtr<IGraphicsBone> createBone() = 0;

            /** Creates a brand new Bone owned by this Skeleton. */
            virtual SmartPtr<IGraphicsBone> createBone( u32 handle ) = 0;

            /** Creates a brand new Bone owned by this Skeleton. */
            virtual SmartPtr<IGraphicsBone> createBone( const String &name ) = 0;

            /** Creates a brand new Bone owned by this Skeleton. */
            virtual SmartPtr<IGraphicsBone> createBone( const String &name, u32 handle ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IGraphicsSkeleton_h__
