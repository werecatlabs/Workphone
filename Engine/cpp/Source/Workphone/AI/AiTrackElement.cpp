#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/AiTrackElement.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AiTrackElement, IAiTrackElement );

    AiTrackElement::AiTrackElement() = default;

    AiTrackElement::~AiTrackElement() = default;

    auto AiTrackElement::getCenter() const -> Vector3<real_Num>
    {
        return m_center;
    }

    void AiTrackElement::setCenter( const Vector3<real_Num> &center )
    {
        m_center = center;
    }

    auto AiTrackElement::getStart() const -> Vector3<real_Num>
    {
        return m_start;
    }

    void AiTrackElement::setStart( const Vector3<real_Num> &start )
    {
        m_start = start;
    }

    auto AiTrackElement::getEnd() const -> Vector3<real_Num>
    {
        return m_end;
    }

    void AiTrackElement::setEnd( const Vector3<real_Num> &end )
    {
        m_end = end;
    }

    auto AiTrackElement::getDirection() const -> Vector3<real_Num>
    {
        return m_direction;
    }

    void AiTrackElement::setDirection( const Vector3<real_Num> &direction )
    {
        m_direction = direction;
    }

    auto AiTrackElement::getExtents() const -> Vector3<real_Num>
    {
        return m_extents;
    }

    void AiTrackElement::setExtents( const Vector3<real_Num> &extents )
    {
        m_extents = extents;
    }
}  // namespace workphone
