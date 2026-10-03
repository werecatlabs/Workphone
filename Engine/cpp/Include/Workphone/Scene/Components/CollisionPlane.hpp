#ifndef CollisionPlane_h__
#define CollisionPlane_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Scene/Components/Collision.hpp>

namespace workphone
{
    namespace scene
    {
        /** Collision plane component. */
        class WPCore_API CollisionPlane : public Collision
        {
        public:
            /** Constructor. */
            CollisionPlane();

            /** Destructor. */
            ~CollisionPlane() override;

            /** @copydoc Collision::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CollisionPlane_h__
