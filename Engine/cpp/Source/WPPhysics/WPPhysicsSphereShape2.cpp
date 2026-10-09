#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsSphereShape2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    WPPhysicsSphereShape2::WPPhysicsSphereShape2() : WPPhysicsShape2T<SphereShape2>(
        WORKPHONE_COLLISION_SHAPE_SPHERE)
    {
        setRadius(0.5);
    }

    WPPhysicsSphereShape2::~WPPhysicsSphereShape2() = default;

    void WPPhysicsSphereShape2::setRadius(real_Num radius)
    {
        WP_ASSERT(radius > static_cast<real_Num>( 0 ));
        const auto safeRadius = std::max(radius, static_cast<real_Num>(0));
        wp_collision_shape2_set_sphere_radius(getShape(), safeRadius);
        WP_ASSERT(getRadius() >= static_cast<real_Num>( 0 ));
    }

    real_Num WPPhysicsSphereShape2::getRadius() const
    {
        return wp_collision_shape2_get_sphere_radius(getShape());
    }

    Sphere2<real_Num> WPPhysicsSphereShape2::getSphere() const
    {
        return Sphere2<real_Num>(Vector2<real_Num>::ZERO, getRadius());
    }

    AABB2<real_Num> WPPhysicsSphereShape2::getAABB() const
    {
        const auto radius = getRadius();
        return AABB2<real_Num>(Vector2<real_Num>(-radius, -radius),
                               Vector2<real_Num>(radius, radius));
    }

    void WPPhysicsSphereShape2::getPoints(Array<Vector2<real_Num>> &points) const
    {
        points.clear();
        constexpr u32 numPoints = 16;
        points.reserve(numPoints);
        const auto radius = getRadius();
        constexpr auto twoPi = static_cast<real_Num>(6.28318530717958647692);
        for(u32 i = 0; i < numPoints; ++i)
        {
            const auto angle = twoPi * static_cast<real_Num>(i) / static_cast<real_Num>(numPoints);
            points.emplace_back(std::cos(angle) * radius,
                                std::sin(angle) * radius);
        }
    }

    void WPPhysicsSphereShape2::computeMass(SmartPtr<IMassData2> massData, real_Num density) const
    {
        WP_ASSERT(massData);
        WP_ASSERT(density >= static_cast<real_Num>( 0 ));
        if(!massData)
        {
            return;
        }

        const auto radius = getRadius();
        constexpr auto pi = static_cast<real_Num>(3.14159265358979323846);
        const auto mass = pi * radius * radius * std::max(density, static_cast<real_Num>(0));
        const auto inertia = static_cast<real_Num>(0.5) * mass * radius * radius;
        massData->setMass(mass);
        massData->setCenter(Vector2<real_Num>::ZERO);
        massData->setInertia(inertia);
        WP_ASSERT(massData->getMass() >= 0.0f);
    }

    SmartPtr<Properties> WPPhysicsSphereShape2::getProperties() const
    {
        auto properties = WPPhysicsShape2T<SphereShape2>::getProperties();
        properties->setProperty("radius", getRadius());
        return properties;
    }

    void WPPhysicsSphereShape2::setProperties(SmartPtr<Properties> properties)
    {
        WPPhysicsShape2T<SphereShape2>::setProperties(properties);
        if(!properties)
        {
            return;
        }

        auto radius = getRadius();
        properties->getPropertyValue("radius", radius);
        setRadius(radius);
    }

    bool WPPhysicsSphereShape2::handleStateChanged(const SmartPtr<IStateMessage> &)
    {
        return false;
    }

    bool WPPhysicsSphereShape2::handleStateChanged(SmartPtr<IState> &)
    {
        return false;
    }
} // namespace workphone::physics
