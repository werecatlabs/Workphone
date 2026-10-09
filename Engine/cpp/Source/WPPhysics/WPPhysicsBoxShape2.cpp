#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsBoxShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone::physics
{
    namespace
    {
        wp_vec2f toWp(const Vector2<real_Num> &v)
        {
            wp_vec2f r = { (v.X()), (v.Y()) };
            return r;
        }

        Vector2<real_Num> fromWp(wp_vec2f v)
        {
            return Vector2<real_Num>(v.x, v.y);
        }
    } // namespace

    WPPhysicsBoxShape2::WPPhysicsBoxShape2() : WPPhysicsShape2T<BoxShape2>(WORKPHONE_COLLISION_SHAPE_BOX)
    {
        setAABB(AABB2<real_Num>(Vector2<real_Num>(-0.5, -0.5), Vector2<real_Num>(0.5, 0.5)));
    }

    WPPhysicsBoxShape2::~WPPhysicsBoxShape2() = default;

    Sphere2<real_Num> WPPhysicsBoxShape2::getSphere() const
    {
        const auto halfSize = m_aabb.getHalfSize();
        const auto radius = std::max(halfSize.X(), halfSize.Y());
        return Sphere2<real_Num>(m_aabb.getCenter(), radius);
    }

    AABB2<real_Num> WPPhysicsBoxShape2::getAABB() const
    {
        return m_aabb;
    }

    void WPPhysicsBoxShape2::setAABB(const AABB2<real_Num> &box)
    {
        m_aabb = box;
        m_aabb.repair();
        WP_ASSERT(m_aabb.isValid());
        const auto halfSize = m_aabb.getHalfSize();
        WP_ASSERT(halfSize.X() > static_cast<real_Num>( 0 ));
        WP_ASSERT(halfSize.Y() > static_cast<real_Num>( 0 ));
        wp_collision_shape2_set_box_half_extents(getShape(), toWp(halfSize));
        auto center = toWp(m_aabb.getCenter());
        wp_vec3f localPosition = { center.x, center.y, 0.0f };
        wp_collision_shape_set_local_position(getShape(), localPosition);
        WP_ASSERT(fromWp( wp_collision_shape2_get_box_half_extents( getShape() ) ) == halfSize);
    }

    void WPPhysicsBoxShape2::getPoints(Array<Vector2<real_Num>> &points) const
    {
        points.clear();
        points.push_back(m_aabb.getMin());
        points.push_back(Vector2<real_Num>(m_aabb.getMax().X(), m_aabb.getMin().Y()));
        points.push_back(m_aabb.getMax());
        points.push_back(Vector2<real_Num>(m_aabb.getMin().X(), m_aabb.getMax().Y()));
    }

    void WPPhysicsBoxShape2::computeMass(SmartPtr<IMassData2> massData, real_Num density) const
    {
        WP_ASSERT(massData);
        WP_ASSERT(density >= static_cast<real_Num>( 0 ));
        if(!massData)
        {
            return;
        }

        const auto size = m_aabb.getSize();
        const auto area = size.X() * size.Y();
        const auto mass = area * std::max(density, static_cast<real_Num>(0));
        const auto inertia =
            mass * (size.X() * size.X() + size.Y() * size.Y()) / static_cast<real_Num>(12);
        massData->setMass(mass);
        massData->setCenter(m_aabb.getCenter());
        massData->setInertia(inertia);
        WP_ASSERT(massData->getMass() >= 0.0f);
    }

    SmartPtr<Properties> WPPhysicsBoxShape2::getProperties() const
    {
        auto properties = WPPhysicsShape2T<BoxShape2>::getProperties();
        properties->setProperty("center", m_aabb.getCenter());
        properties->setProperty("size", m_aabb.getSize());
        return properties;
    }

    void WPPhysicsBoxShape2::setProperties(SmartPtr<Properties> properties)
    {
        WPPhysicsShape2T<BoxShape2>::setProperties(properties);
        if(!properties)
        {
            return;
        }

        auto center = m_aabb.getCenter();
        auto size = m_aabb.getSize();
        properties->getPropertyValue("center", center);
        properties->getPropertyValue("size", size);
        size.X() = Math<real_Num>::Abs(size.X());
        size.Y() = Math<real_Num>::Abs(size.Y());
        const auto halfSize = size * 0.5;
        setAABB(AABB2<real_Num>(center - halfSize, center + halfSize));
    }

    bool WPPhysicsBoxShape2::handleStateChanged(const SmartPtr<IStateMessage> &)
    {
        return false;
    }

    bool WPPhysicsBoxShape2::handleStateChanged(SmartPtr<IState> &)
    {
        return false;
    }
} // namespace workphone::physics
