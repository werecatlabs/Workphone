#ifndef _ISoundListener3D_H
#define _ISoundListener3D_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /** Interface for a 3D sound listener. */
    class WPCore_API ISoundListener3 : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~ISoundListener3() override;

        /** Set the new position of the listener.
         * @param position The new position.
         */
        virtual void setPosition( const Vector3<real_Num> &position ) = 0;

        /** Retrieves the current position of the listener.
         * @return The position.
         */
        virtual Vector3<real_Num> getPosition() const = 0;

        /** Sets the direction the listener is facing.
         * @param vector The new forward vector.
         */
        virtual void setForwardVector( const Vector3<real_Num> &vector ) = 0;

        /** Gets the direction the listener is facing.
         * @return The forward vector.
         */
        virtual Vector3<real_Num> getForwardVector() const = 0;

        /** Set the velocity of the listener.
         * @param velocity The new velocity.
         */
        virtual void setVelocity( const Vector3<real_Num> &velocity ) = 0;

        /** Retrieves the velocity of the listener.
         * @return The velocity.
         */
        virtual Vector3<real_Num> getVelocity() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
