#ifndef KeyFrameTransform3_h__
#define KeyFrameTransform3_h__

#include <Workphone/Animation/AnimationKeyFrame.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /**
     * @class KeyFrameTransform3
     * @brief Represents a keyframe containing 3D transformation data (position, orientation, and scale).
     */
    class WPCore_API KeyFrameTransform3 : public AnimationKeyFrame
    {
    public:
        /** @brief Default constructor. */
        KeyFrameTransform3();

        /**
         * @brief Constructs a transformation keyframe with specified values.
         * @param time The timestamp of the keyframe.
         * @param position The 3D position vector.
         * @param orientation The rotation quaternion.
         * @param scale The scale vector.
         */
        KeyFrameTransform3( f32 time, const Vector3<real_Num> &position,
                            const Quaternion<real_Num> &orientation, const Vector3<real_Num> &scale );

        ~KeyFrameTransform3() override;

        /** @brief Gets the position of the keyframe. */
        Vector3<real_Num> getPosition() const;

        /** @brief Sets the position of the keyframe. */
        void setPosition( const Vector3<real_Num> &position );

        /** @brief Gets the orientation of the keyframe. */
        Quaternion<real_Num> getOrientation() const;

        /** @brief Sets the orientation of the keyframe. */
        void setOrientation( const Quaternion<real_Num> &orientation );

        /** @brief Gets the scale of the keyframe. */
        Vector3<real_Num> getScale() const;

        /** @brief Sets the scale of the keyframe. */
        void setScale( const Vector3<real_Num> &scale );

        WP_CLASS_REGISTER_DECL;

    private:
        Quaternion<real_Num> m_orientation =
            Quaternion<real_Num>::identity();                      ///< Orientation of the keyframe
        Vector3<real_Num> m_position = Vector3<real_Num>::zero();  ///< Position of the keyframe
        Vector3<real_Num> m_scale = Vector3<real_Num>::unit();     ///< Scale of the keyframe
    };
}  // namespace workphone

#endif  // KeyFrameTransform3_h__
