#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysicsMaterial2.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsMaterial2::WPPhysicsMaterial2() : m_restitution(static_cast<real_Num>(1.0))
    {
        m_friction.resize(2);

        m_static_friction = static_cast<real_Num>(1.95);
        m_dynamic_friction = static_cast<real_Num>(0.95);
        m_elasticCoef = static_cast<real_Num>(0.91);
        m_collisionID = 0;

        // default values
        mode = dContactBounce | dContactSlip1 | dContactSlip2;
        mu = 0.0f;
        mu2 = 0.0f;
        bounce = static_cast<real_Num>(0.25);
        bounce_vel = static_cast<real_Num>(1.0);

        soft_erp = static_cast<real_Num>(0.25);
        soft_cfm = static_cast<real_Num>(0.25);

        motion1 = static_cast<real_Num>(0.01);
        motion2 = static_cast<real_Num>(0.01);

        slip1 = static_cast<real_Num>(0.5);
        slip2 = static_cast<real_Num>(0.5);
    }

    SmartPtr<IPhysicsBody2D> WPPhysicsMaterial2::getPhysicsBodyB() const
    {
        return m_bodyB;
    }

    void WPPhysicsMaterial2::setPhysicsBodyB(SmartPtr<IPhysicsBody2D> body)
    {
        m_bodyB = body;
    }

    SmartPtr<IPhysicsBody2D> WPPhysicsMaterial2::getPhysicsBodyA() const
    {
        return m_bodyA;
    }

    void WPPhysicsMaterial2::setPhysicsBodyA(SmartPtr<IPhysicsBody2D> body)
    {
        m_bodyA = body;
    }

    void WPPhysicsMaterial2::setPhysicsBodyA(IPhysicsBody2D *body)
    {
        m_bodyA = body;
    }

    Vector2<real_Num> WPPhysicsMaterial2::getContactNormal() const
    {
        return m_normal;
    }

    void WPPhysicsMaterial2::setContactNormal(const Vector2<real_Num> &normal)
    {
        m_normal = normal;
    }

    Vector2<real_Num> WPPhysicsMaterial2::getContactPosition() const
    {
        return m_position;
    }

    void WPPhysicsMaterial2::setContactPosition(const Vector2<real_Num> &position)
    {
        m_position = position;
    }

    real_Num WPPhysicsMaterial2::getRestitution() const
    {
        return m_restitution;
    }

    void WPPhysicsMaterial2::setRestitution(real_Num restitution)
    {
        m_restitution = restitution;
    }

    real_Num WPPhysicsMaterial2::getFriction(s32 direction) const
    {
        return m_friction[direction];
    }

    void WPPhysicsMaterial2::setFriction(real_Num friction, s32 direction)
    {
        m_friction[direction] = friction;
    }

    WPPhysicsMaterial2::~WPPhysicsMaterial2()
    {
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::setProperty(hash_type id, const Parameter &param)
    {
        if(id == StringUtil::getHash("Restitution"))
        {
            m_material->setRestitution(static_cast<real_Num>(param.getF64()));
        }

        return 0;
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::setProperty(hash_type id, const Parameters &params)
    {
        return 0;
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::getProperty(hash_type id, Parameter &param) const
    {
        if(id == StringUtil::getHash("Restitution"))
        {
            param.setF64(m_material->getRestitution());
        }

        return 0;
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::getProperty(hash_type id, Parameters &params) const
    {
        return 0;
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::getProperty(hash_type hash, void *param) const
    {
        return 0;
    }

    s32 WPPhysicsMaterial2::ScriptReceiver::setProperty(hash_type hash, void *param)
    {
        return 0;
    }

    WPPhysicsMaterial2::ScriptReceiver::ScriptReceiver(WPPhysicsMaterial2 *material) :
        m_material(material)
    {
    }
} // namespace workphone::physics
