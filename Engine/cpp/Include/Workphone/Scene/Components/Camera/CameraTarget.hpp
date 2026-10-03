#ifndef CameraTarget_h__
#define CameraTarget_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /** A camera target component.
         */
        class WPCore_API CameraTarget : public Component
        {
        public:
            static const String offsetPositionStr;
            static const String offsetRotationStr;

            /** Constructor. */
            CameraTarget();

            /** Destructor. */
            ~CameraTarget() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            Vector3<real_Num> getOffsetPosition() const;

            void setOffsetPosition( const Vector3<real_Num> &offsetPosition );

            Quaternion<real_Num> getOffsetRotation() const;

            void setOffsetRotation( const Quaternion<real_Num> &offsetRotation );

            WP_CLASS_REGISTER_DECL;

        protected:
            Vector3<real_Num> m_offsetPosition;
            Quaternion<real_Num> m_offsetRotation;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CameraTarget_h__
