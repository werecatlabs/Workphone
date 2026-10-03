#ifndef CollisionBox_h__
#define CollisionBox_h__

#include <Workphone/Scene/Components/Collision.hpp>

namespace workphone
{
    namespace scene
    {
        /** Collision box component. */
        class WPCore_API CollisionBox : public Collision
        {
        public:
            /** Constructor. */
            CollisionBox();

            /** Destructor. */
            ~CollisionBox() override;

            /** @copydoc Collision::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Collision::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Collision::setExtents */
            void setExtents( const Vector3<real_Num> &extents ) override;

            /** @copydoc Collision::isValid */
            bool isValid() const override;

            /** @copydoc Component::updateTransform */
            void updateTransform() override;

            /** @copydoc Collision::getBoundingBox */
            AABB3<real_Num> getBoundingBox() const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CollisionBox_h__
