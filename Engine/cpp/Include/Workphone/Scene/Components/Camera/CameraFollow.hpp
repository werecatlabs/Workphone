#ifndef CameraFollow_h__
#define CameraFollow_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /** A component to follow an object.
         */
        class WPCore_API CameraFollow : public Component
        {
        public:
            static const String targetStr;
            static const String followObjectStr;

            /** Constructor. */
            CameraFollow();

            /** Destructor. */
            ~CameraFollow() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Gets the camera target.
             * @return The camera target.
             */
            SmartPtr<CameraTarget> getTarget() const;

            /** Sets the camera target.
             * @param target The camera target.
             */
            void setTarget( SmartPtr<CameraTarget> target );

            /** Gets the follow object.
             * @return The follow object.
             */
            SmartPtr<IGameActor> getFollowObject() const;

            /** Sets the follow object.
             * @param followObject The follow object.
             */
            void setFollowObject( SmartPtr<IGameActor> followObject );

            WP_CLASS_REGISTER_DECL;

        protected:
            // The camera target.
            SmartPtr<CameraTarget> m_target;

            // The follow object.
            SmartPtr<IGameActor> m_followObject;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CameraFollow_h__
