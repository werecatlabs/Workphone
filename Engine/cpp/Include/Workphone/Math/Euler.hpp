/**
@file Euler.h
@brief %Euler class for %Ogre
@details License: Do whatever you want with it.
@version 2.3
@author Kojack
@author Transporter
@author Klaim

Extracted From: http://www.ogre3d.org/tikiwiki/tiki-ind ... e=Cookbook
*/
#ifndef HGUARD_OGRE_MATHS_EULER_H
#define HGUARD_OGRE_MATHS_EULER_H

// #include "Workphone/Math/Math.hpp"
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Matrix3.hpp>

namespace workphone
{
    /**
    @class Euler
    @brief Class for %Euler rotations

    <table><tr><td>Yaw is a rotation around the Y axis.</td><td>Pitch is a rotation around the X
    axis.</td><td>Roll is a rotation around the Z axis.</td></tr> <tr><td><img
    src="http://www.ogre3d.org/tikiwiki/tiki-download_file.php?fileId=2112" /></td><td><img
    src="http://www.ogre3d.org/tikiwiki/tiki-download_file.php?fileId=2113" /></td><td><img
    src="http://www.ogre3d.org/tikiwiki/tiki-download_file.php?fileId=2114" /></td></tr></table>
    */
    template <class T>
    class WPCore_API Euler
    {
    public:
        /// Default constructor.
        Euler();

        /**
        @brief Constructor which takes yaw, pitch and roll values.
        @param y Starting value for yaw
        @param p Starting value for pitch
        @param r Starting value for roll
        */
        explicit Euler( const T &y, const T &p = T( 0.0 ), const T &r = T( 0.0 ) );

        explicit Euler( const Vector3<T> &rads );

        /**
        @brief Default constructor with presets.
        @param quaternion Calculate starting values from this quaternion
        */
        explicit Euler( const Quaternion<T> &quaternion );

        explicit Euler( const Matrix3<T> &matrix );

        /// Get the Yaw angle.
        T yaw() const;

        /// Get the Pitch angle.
        T pitch() const;

        /// Get the Roll angle.
        T roll() const;

        /**
        @brief Set the yaw.
        @param y New value for yaw
        */
        Euler &setYaw( T y );

        /**
        @brief Set the pitch.
        @param p New value for pitch
        */
        Euler &setPitch( T p );

        /**
        @brief Set the roll.
        @param r New value for roll
        */
        Euler &setRoll( T r );

        /**
        @brief Set all rotations at once.
        @param y New value for yaw
        @param p New value for pitch
        @param r New value for roll
        */
        Euler &orientation( const T &y, const T &p, const T &r );

        /**
        @brief Apply a relative yaw.
        @param y Angle to add on current yaw
        */
        Euler &yaw( const T &y );

        /**
        @brief Apply a relative pitch.
        @param p Angle to add on current pitch
        */
        Euler &pitch( const T &p );

        /**
        @brief Apply a relative roll.
        @param r Angle to add on current roll
        */
        Euler &roll( const T &r );

        /**
        @brief Apply all relative rotations at once.
        @param y Angle to add on current yaw
        @param p Angle to add on current pitch
        @param r Angle to add on current roll
        */
        Euler &rotate( const T &y, const T &p, const T &r );

        /// Get a vector pointing forwards.
        Vector3<T> forward() const;

        /// Get a vector pointing to the right.
        Vector3<T> right() const;

        /// Get a vector pointing up.
        Vector3<T> up() const;

        Vector3<T> toRadians() const;

        Vector3<T> toDegrees() const;

        /**
        @brief Calculate the quaternion of the euler object.
        @details The result is cached, it is only recalculated when the component euler angles are
        changed.
        */
        Quaternion<T> toQuaternion() const;

        /// Casting operator. This allows any ogre function that wants a Quaternion to accept a Euler
        /// instead.
        explicit operator Quaternion<T>() const;

        /**
        @brief Calculate the current euler angles of a given quaternion object.
        @param quaternion Quaternion which is used to calculate current euler angles.
        */
        Euler &fromQuaternion( const Quaternion<T> &quaternion );

        /**
        @brief Calculate the current euler angles of a given matrix object.
        @param matrix Matrix3 which is used to calculate current euler angles.
        */
        Euler &fromMatrix3( const Matrix3<T> &matrix );

        /**
        @brief Set the yaw and pitch to face in the given direction.
        @details The direction doesn't need to be normalised. Roll is always unaffected.
        @param setYaw If false, the yaw isn't changed.
        @param setPitch If false, the pitch isn't changed.
        */
        Euler &direction( const Vector3<T> &v, bool setYaw = true, bool setPitch = true );

        /**
        @brief Normalise the selected rotations to be within the +/-180 degree range.
        @details The normalise uses a wrap around, so for example a yaw of 360 degrees becomes 0 degrees,
        and -190 degrees becomes 170.
        @param normYaw If false, the yaw isn't normalized.
        @param normPitch If false, the pitch isn't normalized.
        @param normRoll If false, the roll isn't normalized.
        */
        Euler &normalise( bool normYaw = true, bool normPitch = true, bool normRoll = true );

        /**
        @brief Return the relative euler angles required to rotate from the current forward direction to
        the specified dir vector.
        @details The result euler can then be added to the current euler to immediately face dir.
        The rotation won't flip upside down then roll instead of a 180 degree yaw.
        @param setYaw If false, the angle is set to 0. If true, the angle is calculated.
        @param setPitch If false, the angle is set to 0. If true, the angle is calculated.
        @param shortest If false, the full value of each angle is used. If true, the angles are
        normalised and the shortest rotation is found to face the correct direction. For example, when
        false a yaw of 1000 degrees and a dir of (0,0,-1) will return a -1000 degree yaw. When true, the
        same yaw and dir would give 80 degrees (1080 degrees faces the same way as (0,0,-1).
        */
        Euler rotationTo( const Vector3<T> &dir, bool setYaw = true, bool setPitch = true,
                          bool shortest = true ) const;

        /// Clamp the yaw angle to a range of +/-limit.
        Euler &limitYaw( const T &limit );

        /// Clamp the pitch angle to a range of +/-limit.
        Euler &limitPitch( const T &limit );

        /// Clamp the roll angle to a range of +/-limit.
        Euler &limitRoll( const T &limit );

        /// Stream operator, for printing the euler component angles to a stream
        friend std::ostream &operator<<( std::ostream &o, const Euler &e )
        {
            o << e.mYaw << ", " << e.mPitch << ", " << e.mRoll;
            return o;
        }

        /// Add two euler objects.
        Euler operator+( const Euler &rhs ) const;

        /**
        @brief Subtract two euler objects.
        @details This finds the difference as relative angles.
        */
        Euler operator-( const Euler &rhs ) const;

        /// Interpolate the euler angles by rhs.
        Euler operator*( T rhs ) const;

        /// Interpolate the euler angle by lhs.
        friend Euler operator*( T lhs, const Euler &rhs )
        {
            return Euler( lhs * rhs.mYaw, lhs * rhs.mPitch, lhs * rhs.mRoll );
        }

        /**
        @brief Multiply two eulers.
        @details This has the same effect as multiplying quaternions.
        @returns The result is a quaternion.
        */
        Quaternion<T> operator*( Euler rhs ) const;

        /// Apply the euler rotation to the vector rhs.
        Vector3<T> operator*( const Vector3<T> &rhs ) const;

        /// Copy assignment operator (Euler)
        Euler &operator=( const Euler &src );

        /// Copy assignment operator (Quaternion)
        Euler &operator=( const Quaternion<T> &quaternion );

        /// Copy assignment operator (Matrix3)
        Euler &operator=( const Matrix3<T> &matrix );

        friend bool operator==( const Euler &left, const Euler &right );

        friend bool operator!=( const Euler &left, const Euler &right );

        friend bool sameOrientation( const Euler &left, const Euler &right );

    protected:
        void wrapAngle( T &angle );

        void limitAngle( T &angle, const T &limit );

        //!< Rotation around the Y axis.
        T mYaw;

        //!< Rotation around the X axis.
        T mPitch;

        //!< Rotation around the Z axis.
        T mRoll;

        //!< Cached quaternion equivalent of this euler object.
        mutable Quaternion<T> mCachedQuaternion;

        //!< Is the cached quaternion out of date?
        mutable bool mChanged = true;
    };

    template <class T>
    bool sameOrientation( const Euler<T> &left, const Euler<T> &right )
    {
        // I'm comparing resulting vectors to avoid having to compare angles that are the same but in
        // different values. Only the resulting oriented vectors really have any meaning in the end.
        return left.forward().positionEquals( right.forward() ) &&
               left.up().positionEquals( right.up() );
    }

    template <class T>
    bool operator!=( const Euler<T> &left, const Euler<T> &right )
    {
        return !( left == right );
    }

    template <class T>
    bool operator==( const Euler<T> &left, const Euler<T> &right )
    {
        return left.mYaw == right.mYaw && left.mPitch == right.mPitch && left.mRoll == right.mRoll;
    }

}  // namespace workphone

#endif
