#ifndef CollisionSphere_h__
#define CollisionSphere_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Scene/Components/Collision.hpp>

namespace workphone
{
    namespace scene
    {

        /** Collision sphere. */
        class WPCore_API CollisionSphere : public Collision
        {
        public:
            /** Constructor. */
            CollisionSphere();

            /** Destructor. */
            ~CollisionSphere() override;

            /** @copydoc Collision::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::getBoundingBox */
            AABB3<real_Num> getBoundingBox() const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CollisionSphere_h__
