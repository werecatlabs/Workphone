#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsUtil.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    Vector3<real_Num> WPPhysicsUtil::absoluteVector(const Vector3<real_Num> &value)
    {
        return Vector3<real_Num>(Math<real_Num>::Abs(value.X()), Math<real_Num>::Abs(value.Y()),
                                 Math<real_Num>::Abs(value.Z()));
    }

    AABB3<real_Num> WPPhysicsUtil::transformBounds(const AABB3<real_Num> &bounds,
                                                   const Transform3<real_Num> &transform)
    {
        if(bounds.isInfinite())
        {
            auto result = AABB3<real_Num>();
            result.setInfinite();
            return result;
        }
        if(bounds.isNull())
        {
            auto result = AABB3<real_Num>();
            result.setNull();
            return result;
        }

        Vector3<real_Num> corners[8];
        bounds.getEdges(corners);
        auto result = AABB3<real_Num>(transform.transformPoint(corners[0]));
        for(u32 i = 1; i < 8; ++i)
        {
            result.merge(transform.transformPoint(corners[i]));
        }
        return result;
    }

    AABB3<real_Num> WPPhysicsUtil::mergeShapeBounds(const Array<SmartPtr<IPhysicsShape3>> &shapes)
    {
        auto result = AABB3<real_Num>();
        result.setNull();
        auto hasFiniteBounds = false;

        for(const auto &shape : shapes)
        {
            if(!shape || !shape->isEnabled())
            {
                continue;
            }

            const auto backendShape = dynamic_cast<WPPhysicsShape3Backend *>(shape.get());
            if(!backendShape)
            {
                continue;
            }

            const auto bounds = backendShape->getAABB();
            if(bounds.isInfinite())
            {
                result.setInfinite();
                return result;
            }
            if(bounds.isNull())
            {
                continue;
            }

            if(!hasFiniteBounds)
            {
                result = bounds;
                hasFiniteBounds = true;
            }
            else
            {
                result.merge(bounds);
            }
        }

        return result;
    }

    AABB3F WPPhysicsUtil::toFloatBounds(const AABB3<real_Num> &bounds)
    {
        auto result = AABB3F();
        if(bounds.isNull())
        {
            result.setNull();
            return result;
        }
        if(bounds.isInfinite())
        {
            result.setInfinite();
            return result;
        }

        const auto minimum = bounds.getMinimum();
        const auto maximum = bounds.getMaximum();
        return AABB3F(Vector3F(minimum.X(), minimum.Y(),
                               minimum.Z()),
                      Vector3F(maximum.X(), maximum.Y(),
                               maximum.Z()));
    }

    wp_vec3f WPPhysicsUtil::toWp(const Vector3<real_Num> &v)
    {
        wp_vec3f r = { (v.X()), (v.Y()),
                       (v.Z()) };
        return r;
    }

    Vector3<real_Num> WPPhysicsUtil::fromWp(wp_vec3f v)
    {
        return Vector3<real_Num>(v.x, v.y,
                                 v.z);
    }

    wp_quatf WPPhysicsUtil::toWp(const Quaternion<real_Num> &q)
    {
        wp_quatf r = { static_cast<wp_f32>(q.w), static_cast<wp_f32>(q.x),
                       static_cast<wp_f32>(q.y), static_cast<wp_f32>(q.z) };
        return r;
    }

    Quaternion<real_Num> WPPhysicsUtil::fromWp(wp_quatf q)
    {
        return Quaternion<real_Num>(q.w, q.x,
                                    q.y, q.z);
    }

    wp_force_mode WPPhysicsUtil::toWp(ForceModeEnum mode)
    {
        switch(mode)
        {
        case ForceModeEnum::Impulse:
            return WORKPHONE_FORCE_MODE_IMPULSE;
        case ForceModeEnum::VelocityChange:
            return WORKPHONE_FORCE_MODE_VELOCITY_CHANGE;
        case ForceModeEnum::Acceleration:
            return WORKPHONE_FORCE_MODE_ACCELERATION;
        default:
            return WORKPHONE_FORCE_MODE_FORCE;
        }
    }
} // namespace workphone::physics
