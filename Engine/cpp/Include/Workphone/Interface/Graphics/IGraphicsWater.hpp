#ifndef IWater_h__
#define IWater_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Interface for a water object.
         */
        class WPCore_API IGraphicsWater : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~IGraphicsWater() override;

            /**
             * Gets the scene manager associated with this water object.
             *
             * @return A shared pointer to the scene manager.
             */
            virtual SmartPtr<IGraphicsScene> getSceneManager() const = 0;

            /**
             * Sets the scene manager associated with this water object.
             *
             * @param sceneMgr A shared pointer to the scene manager.
             */
            virtual void setSceneManager( SmartPtr<IGraphicsScene> sceneMgr ) = 0;

            /**
             * Gets the camera associated with this water object.
             *
             * @return A shared pointer to the camera.
             */
            virtual SmartPtr<IGraphicsCamera> getCamera() const = 0;

            /**
             * Sets the camera associated with this water object.
             *
             * @param camera A shared pointer to the camera.
             */
            virtual void setCamera( SmartPtr<IGraphicsCamera> camera ) = 0;

            /**
             * Gets the viewport associated with this water object.
             *
             * @return A shared pointer to the viewport.
             */
            virtual SmartPtr<IViewport> getViewport() const = 0;

            /**
             * Sets the viewport associated with this water object.
             *
             * @param viewport A shared pointer to the viewport.
             */
            virtual void setViewport( SmartPtr<IViewport> viewport ) = 0;

            /**
             * Gets the position of the node relative to its parent.
             *
             * @return The position vector.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * Sets the position of the node relative to its parent.
             *
             * @param position The position vector.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IWater_h__
