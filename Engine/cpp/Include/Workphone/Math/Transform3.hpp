#ifndef Transformation3_h__
#define Transformation3_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Euler.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    /**
     * Class used to store a 3d transformation.
     */
    template <class T>
    class WPCore_API Transform3
    {
    public:
        /** Default constructor. */
        Transform3();

        /**
         * Constructor with position and orientation parameters.
         * @param position The position of the transformation.
         * @param orientation The orientation of the transformation.
         */
        Transform3( const Vector3<T> &position, const Quaternion<T> &orientation );

        /**
         * Constructor with position, orientation, and scale parameters.
         * @param position The position of the transformation.
         * @param orientation The orientation of the transformation.
         * @param scale The scale of the transformation.
         */
        Transform3( const Vector3<T> &position, const Quaternion<T> &orientation,
                    const Vector3<T> &scale );

        /**
         * Constructor with an orientation parameter.
         * @param orientation The orientation of the transformation.
         */
        explicit Transform3( const Quaternion<T> &orientation );

        /** Copy constructor. */
        Transform3( const Transform3 &t );

        /** Destructor. */
        ~Transform3();

        /**
         * Returns the position of the transformation.
         * @return The position of the transformation.
         */
        Vector3<T> &getPosition();

        /**
         * Returns the position of the transformation.
         * @return The position of the transformation.
         */
        const Vector3<T> &getPosition() const;

        /**
         * Sets the position of the transformation.
         * @param position The new position of the transformation.
         */
        void setPosition( const Vector3<T> &position );

        /**
         * Returns the scale of the transformation.
         * @return The scale of the transformation.
         */
        Vector3<T> &getScale();

        /**
         * Returns the scale of the transformation.
         * @return The scale of the transformation.
         */
        const Vector3<T> &getScale() const;

        /**
         * Sets the scale of the transformation.
         * @param scale The new scale of the transformation.
         */
        void setScale( const Vector3<T> &scale );

        /**
         * Returns the orientation of the transformation.
         * @return The orientation of the transformation.
         */
        Quaternion<T> &getOrientation();

        /**
         * Returns the orientation of the transformation.
         * @return The orientation of the transformation.
         */
        const Quaternion<T> &getOrientation() const;

        /**
         * Sets the orientation of the transformation.
         * @param orientation The new orientation of the transformation.
         */
        void setOrientation( const Quaternion<T> &orientation );

        /**
         * Returns the rotation of the transformation.
         * @return The rotation of the transformation.
         */
        Vector3<T> getRotation() const;

        /**
         * Sets the rotation of the transformation.
         * @param rotation The new rotation of the transformation.
         */
        void setRotation( const Vector3<T> &rotation );

        /**
         * Returns the transformation matrix of the transformation.
         * @return The transformation matrix of the transformation.
         */
        Matrix4<T> getTransformationMatrix() const;

        /**
         * Copy assignment operator.
         * \param transform The transformation to copy.
         */
        void operator=( const Transform3<T> &transform );

        /**
         * Equality operator.
         * \param other The other transformation to compare with.
         * \return True if the transformations are equal, false otherwise.
         */
        bool operator==( const Transform3<T> &other ) const;

        /**
         * Inequality operator.
         * \param other The other transformation to compare with.
         * \return True if the transformations are not equal, false otherwise.
         */
        bool operator!=( const Transform3<T> &other ) const;

        /**
         * Transforms a point by this transformation.
         * \param p The point to transform.
         * \return The transformed point.
         */
        Vector3<T> transformPoint( const Vector3<T> &p ) const;

        /**
         * Transforms a vector by this transformation.
         * \param dir The vector to transform.
         * \return The transformed vector.
         */
        Vector3<T> transformVector( const Vector3<T> &dir ) const;

        /** Transforms a point from local space to world space. */
        Vector3<T> inverseTransformPoint( const Vector3<T> &p ) const;

        /** Transforms a vector from local space to world space. */
        Vector3<T> inverseTransformVector( const Vector3<T> &dir ) const;

        /** Rotates a point from local space to world space. */
        Vector3<T> inverseRotate( const Vector3<T> &p ) const;

        /** Returns the up vector of the transform in world space. */
        Vector3<T> up() const;

        /** Returns the forward vector of the transform in world space. */
        Vector3<T> forward() const;

        /** Returns the right vector of the transform in world space. */
        Vector3<T> right() const;

        /** Sets the transform using the given position, rotation and scale. */
        void setTransform( const Vector3<T> &pos, const Quaternion<T> &rot, const Vector3<T> &scale );

        /** Sets the transform using another Transform3 instance. */
        void setTransform( const Transform3 &t );

        /** Gets the transform values (position, rotation and scale) into the provided arguments. */
        void getTransform( Vector3<T> &pos, Quaternion<T> &rot, Vector3<T> &scale );

        /** Transforms the local transform to the world transform given the parent transform and the
         * local transform. */
        void transformFromParent( const Transform3 &parent, const Transform3 &local );

        /** Transforms the local transform to the world transform given the parent transform and the
         * local transform. */
        void transformFromParent( const SmartPtr<Transform3<T>> &parent,
                                  const SmartPtr<Transform3<T>> &local );

        /**
         * Transforms a point from world space to local space.
         * @param parentTransform The world transform of the parent object.
         * @param worldTransform The world transform of the object being transformed.
         */
        void fromWorldToLocal( const Transform3<T> &parentTransform,
                               const Transform3<T> &worldTransform );

        /**
         * Converts a position from world space to local space.
         * @param worldPos The position in world space.
         * @return The position in local space.
         */
        Vector3<T> convertWorldToLocalPosition( const Vector3<T> &worldPos ) const;

        /**
         * Converts an orientation from world space to local space.
         * @param worldOrientation The orientation in world space.
         * @return The orientation in local space.
         */
        Quaternion<T> convertWorldToLocalOrientation( const Quaternion<T> &worldOrientation ) const;

        /**
         * Converts a position from local space to world space.
         * @param localPos The position in local space.
         * @return The position in world space.
         */
        Vector3<T> convertLocalToWorldPosition( const Vector3<T> &localPos ) const;

        /**
         * Converts an orientation from local space to world space.
         * @param localOrientation The orientation in local space.
         * @return The orientation in world space.
         */
        Quaternion<T> convertLocalToWorldOrientation( const Quaternion<T> &localOrientation ) const;

        /**
         * Transforms an AABB from local space to world space.
         * @param aabb The AABB in local space.
         * @return The AABB in world space.
         */
        AABB3<T> transformAABB( const AABB3<T> &aabb ) const;

        /**
         * Returns true if the transform is valid.
         * @return True if the transform is valid, false otherwise.
         */
        bool isValid() const;

        /**
         * Returns true if the transform is sane (i.e. not NaN or Inf).
         * @return True if the transform is sane, false otherwise.
         */
        bool isSane() const;

        /**
         * Returns true if the transform is finite.
         * @return True if the transform is finite, false otherwise.
         */
        bool isFinite() const;

        /**
         * Returns the inverse transform.
         * @return The inverse transform.
         */
        Transform3<T> getInverse() const;

        /**
         * Returns the object properties.
         * @return The object properties.
         */
        SmartPtr<Properties> getProperties() const;

        /**
         * Sets the object properties.
         * @param properties The object properties to set.
         */
        void setProperties( SmartPtr<Properties> properties );

        static Transform3<T> identity();

    private:
        /**
         * The position of the transformation in world space.
         */
        Vector3<T> m_position = Vector3<T>::zero();

        /**
         * The scale of the transformation in world space.
         */
        Vector3<T> m_scale = Vector3<T>::unit();

        /**
         * The orientation of the transformation in world space.
         */
        Quaternion<T> m_orientation = Quaternion<T>::identity();
    };

    /// Typedef for a f32 transform.
    using Transform3I = Transform3<s32>;

    /// Typedef for a f32 transform.
    using Transform3F = Transform3<f32>;

    /// Typedef for a f64 transform.
    using Transform3D = Transform3<f64>;

}  // namespace workphone

#endif  // Transformation3_h__
