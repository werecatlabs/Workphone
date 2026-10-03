#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/KeyFrameTransform2.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, KeyFrameTransform2, AnimationKeyFrame );

    KeyFrameTransform2::KeyFrameTransform2() = default;

    KeyFrameTransform2::~KeyFrameTransform2() = default;

    Vector2<real_Num> KeyFrameTransform2::getPosition() const
    {
        return m_position;
    }

    void KeyFrameTransform2::setPosition( const Vector2<real_Num> &position )
    {
        m_position = position;
    }

    Vector2<real_Num> KeyFrameTransform2::getScale() const
    {
        return m_scale;
    }

    void KeyFrameTransform2::setScale( const Vector2<real_Num> &scale )
    {
        m_scale = scale;
    }

    f32 KeyFrameTransform2::getRotation() const
    {
        return m_rotation;
    }

    void KeyFrameTransform2::setRotation( f32 rotation )
    {
        m_rotation = rotation;
    }

}  // namespace workphone
