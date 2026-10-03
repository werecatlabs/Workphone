#ifndef ICharacterController3_h__
#define ICharacterController3_h__

#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D character controller.
         *
         * This class represents a character controller in the 3D physics simulation.
         * Character controllers are specialized physics bodies designed for player
         * or AI character movement, providing features like ground detection,
         * jumping, and smooth movement control.
         *
         * The interface provides functionality for:
         * - Managing character position and orientation
         * - Controlling movement speed and direction
         * - Handling jumping and ground detection
         * - Managing character state and behavior
         *
         * @see IPhysicsBody3
         * @see IPhysicsManager
         */
        class WPCore_API ICharacterController3 : public IPhysicsBody3
        {
        public:
            /** Destructor */
            ~ICharacterController3() override;

            /**
             * @brief Sets the position of the character.
             * @param position The new position to set.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Gets the current position of the character.
             * @return The current position of the character.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Sets the orientation of the character.
             * @param orientation The new orientation to set.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Gets the current orientation of the character.
             * @return The current orientation of the character.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Gets the current movement speed of the character.
             * @return The current movement speed.
             */
            virtual f32 getMoveSpeed() const = 0;

            /**
             * @brief Sets the movement speed of the character.
             * @param moveSpeed The new movement speed to set.
             */
            virtual void setMoveSpeed( f32 moveSpeed ) = 0;

            /**
             * @brief Gets whether the character is currently jumping.
             * @return True if the character is jumping, false otherwise.
             */
            virtual bool getJump() const = 0;

            /**
             * @brief Sets whether the character should jump.
             * @param jump True to make the character jump, false otherwise.
             */
            virtual void setJump( bool jump ) = 0;

            /**
             * @brief Checks if the character is currently on the ground.
             * @return True if the character is grounded, false otherwise.
             */
            virtual bool isGrounded() const = 0;

            /**
             * @brief Gets the underlying physics object.
             * @param ppObject Pointer to store the physics object.
             */
            virtual void _getObject( void **ppObject ) = 0;

            /**
             * @brief Sets the walking direction vector for the character.
             * @param vector The direction vector for walking.
             */
            virtual void setWalkVector( const Vector3<real_Num> &vector ) = 0;

            /**
             * @brief Stops the character's movement.
             */
            virtual void stop() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // ICharacterController3_h__
