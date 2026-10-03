#ifndef _CCamera_H
#define _CCamera_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class CCameraOgreNext
         * @brief Represents a camera in the scene, providing functionality for rendering and managing camera properties.
         *
         * This class is a wrapper around Ogre::Camera and extends the functionality of CGraphicsObjectOgreNext.
         * It provides methods to manage the camera's state, properties, and associated compositor.
         */
        class CCameraOgreNext : public CGraphicsObjectOgreNext<GraphicsCamera>
        {
        public:
            /**
             * @brief Default constructor.
             * Initializes the camera object.
             */
            CCameraOgreNext();

            /**
             * @brief Constructor with a scene creator.
             * @param creator Pointer to the scene creator.
             */
            CCameraOgreNext( CGraphicsSceneOgreNext *creator );

            /**
             * @brief Destructor.
             * Cleans up resources associated with the camera.
             */
            ~CCameraOgreNext() override;

            /**
             * @brief Loads the camera with the specified data.
             * @param data Shared pointer to the data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the camera and releases associated resources.
             * @param data Shared pointer to the data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Clones the camera object.
             * @param name Optional name for the cloned object.
             * @return A smart pointer to the cloned graphics object.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Retrieves the properties of the camera.
             * @return A smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the camera.
             * @param properties Shared pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Retrieves the child objects of the camera.
             * @return An array of shared pointers to the child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the associated compositor.
             * @return A smart pointer to the compositor.
             */
            SmartPtr<Compositor> getCompositor() const;

            /**
             * @brief Sets the associated compositor.
             * @param compositor Shared pointer to the compositor.
             */
            void setCompositor( SmartPtr<Compositor> compositor );

            /**
             * @brief Handles state messages for the camera.
             * @param message Shared pointer to the state message.
             * @return True if the message was handled successfully, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handles state changes for the camera.
             * @param state Shared pointer to the new state.
             * @return True if the state change was handled successfully, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Sets up the state object for the camera.
             */
            void setupStateObject() override;

            /**
             * @brief Creates the compositor for the camera.
             */
            void createCompositor();

            /**
             * @brief Destroys the compositor associated with the camera.
             */
            void destroyCompositor();

            /// The associated compositor.
            AtomicSmartPtr<Compositor> m_compositor;
        };
    }  // end namespace render
}  // namespace workphone

#endif
