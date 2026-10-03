#ifndef KeyFrameTransform2_h__
#define KeyFrameTransform2_h__

#include <Workphone/Animation/AnimationKeyFrame.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @class KeyFrameTransform2
     * @brief Represents a keyframe containing 2D transformation data (position, rotation, and scale).
     */
    class KeyFrameTransform2 : public AnimationKeyFrame
    {
    public:
        /** @brief Default constructor. */
        KeyFrameTransform2();

        ~KeyFrameTransform2() override;

        /** @brief Gets the position of the keyframe. */
        Vector2<real_Num> getPosition() const;

        /** @brief Sets the position of the keyframe. */
        void setPosition( const Vector2<real_Num> &position );

        /** @brief Gets the scale of the keyframe. */
        Vector2<real_Num> getScale() const;

        /** @brief Sets the scale of the keyframe. */
        void setScale( const Vector2<real_Num> &scale );

        /** @brief Gets the rotation angle of the keyframe. */
        f32 getRotation() const;

        /** @brief Sets the rotation angle of the keyframe. */
        void setRotation( f32 rotation );

        WP_CLASS_REGISTER_DECL;

    private:
        Vector2<real_Num> m_position = Vector2<real_Num>::zero();  ///< Position of the keyframe
        Vector2<real_Num> m_scale = Vector2<real_Num>::unit();     ///< Scale of the keyframe
        f32 m_rotation = 0.0f;                                     ///< Rotation angle of the keyframe
    };
}  // namespace workphone

#endif  // KeyFrameTransform2_h__
