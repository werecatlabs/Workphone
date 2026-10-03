#ifndef IVehicleComponent_h__
#define IVehicleComponent_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for a vehicle component.
         */
        class WPCore_API IVehicleComponent : public ISharedObject
        {
        public:
            /**
             * @brief Represents the state of a vehicle component.
             */
            enum class State
            {
                AWAKE,      ///< The component is awake.
                DESTROYED,  ///< The component is destroyed.
                EDIT,       ///< The component is being edited.
                PLAY,       ///< The component is playing.
                RESET,      ///< The component is resetting.

                COUNT  ///< The number of states.
            };

            /** Virtual destructor. */
            ~IVehicleComponent() override;

            /**
             * @brief Resets the component.
             */
            virtual void reset() = 0;

            /**
             * @brief Returns a pointer to the owner of the component.
             */
            virtual IVehicle *getOwnerPtr() const = 0;

            /**
             * @brief Returns a smart pointer to the owner of the component.
             */
            virtual SmartPtr<IVehicle> getOwner() const = 0;

            /**
             * @brief Sets the owner of the component.
             */
            virtual void setOwner( SmartPtr<IVehicle> vehicle ) = 0;

            /**
             * @brief Updates the transform of the component in the world space.
             */
            virtual void updateTransform() = 0;

            /**
             * @brief Updates the transform of the body of the component in the world space.
             */
            virtual void updateBodyTransform() = 0;

            /**
             * @brief Updates the geometry of the component.
             */
            virtual void updateGeometry() = 0;

            /**
             * @brief Returns the world transform of the component.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Sets the world transform of the component.
             */
            virtual void setWorldTransform( Transform3<real_Num> worldTransform ) = 0;

            /**
             * @brief Returns the local transform of the component.
             */
            virtual Transform3<real_Num> getLocalTransform() const = 0;

            /**
             * @brief Sets the local transform of the component.
             */
            virtual void setLocalTransform( Transform3<real_Num> localTransform ) = 0;

            /**
             * @brief Sets the state of the component.
             */
            virtual void setState( State state ) = 0;

            /**
             * @brief Returns the state of the component.
             */
            virtual State getState() const = 0;

            /**
             * @brief Returns a pointer to the data of the component.
             */
            virtual void *getData() const = 0;

            /**
             * @brief Sets the data of the component.
             */
            virtual void setData( void *data ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#if !WP_FINAL
#    define DEBUG_DRAW_LINE( start, end ) getOwner()->displayLocalVector( 0, start, end )
#    define DEBUG_DRAW_LINE_BY_ID( id, start, end, colour ) \
        getOwner()->displayVector( 0, id, start, end, colour )
#    define DEBUG_DRAW_POINT_BY_ID( id, start, end, colour ) \
        getOwner()->drawPoint( 0, id, start, end, colour )
#    define DEBUG_DRAW_LOCAL_LINE_BY_ID( id, start, end, colour ) \
        getOwner()->displayLocalVector( 0, id, start, end, colour )
#    define DEBUG_DRAW_LOCAL_POINT_BY_ID( id, start, end, colour ) \
        getOwner()->drawLocalPoint( 0, id, start, end, colour )
#else
#    define DEBUG_DRAW_LINE( start, end )
#    define DEBUG_DRAW_LINE_BY_ID( id, start, end )
#    define DEBUG_DRAW_POINT_BY_ID( id, start, end, colour )
#    define DEBUG_DRAW_LOCAL_LINE_BY_ID( id, start, end )
#    define DEBUG_DRAW_LOCAL_POINT_BY_ID( id, start, end, colour )
#endif

#endif  // IVehicleComponent_h__
