#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CVehicleBody.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CVehicleBody::CVehicleBody() = default;

    CVehicleBody::~CVehicleBody()
    {
        unload(nullptr);
    }

    void CVehicleBody::unload(SmartPtr<ISharedObject> data)
    {
        m_parentVehicle = nullptr;
    }

    Vector3<physics_Num> CVehicleBody::getVelocity() const
    {
        return m_velocity;
    }

    void CVehicleBody::setVelocity(const Vector3<physics_Num> &velocity)
    {
        m_velocity = velocity;
    }

    Vector3<physics_Num> CVehicleBody::getAngularVelocity() const
    {
        return m_angularVelocity;
    }

    void CVehicleBody::setAngularVelocity(const Vector3<physics_Num> &angularVelocity)
    {
        m_angularVelocity = angularVelocity;
    }

    Vector3<physics_Num> CVehicleBody::getWorldCenterOfMass() const
    {
        return m_worldCenterOfMass;
    }

    void CVehicleBody::setWorldCenterOfMass(const Vector3<physics_Num> &worldCenterOfMass)
    {
        m_worldCenterOfMass = worldCenterOfMass;
    }

    void CVehicleBody::addLocalForceAtPosition(const Vector3<physics_Num> &force,
                                               const Vector3<physics_Num> &pos,
                                               physics::ForceModeEnum forceMode)
    {
        m_parentVehicle->addLocalForce(0, force, pos);
    }

    void CVehicleBody::addLocalForceAtLocalPosition(const Vector3<physics_Num> &force,
                                                    const Vector3<physics_Num> &pos)
    {
        m_parentVehicle->addLocalForce(0, force, pos);
    }

    void CVehicleBody::addForceAtPosition(const Vector3<physics_Num> &force,
                                          const Vector3<physics_Num> &pos,
                                          physics::ForceModeEnum forceMode)
    {
        m_parentVehicle->addForce(0, force, pos);
    }

    void CVehicleBody::addForceAtPosition(const Vector3<physics_Num> &force,
                                          const Vector3<physics_Num> &pos)
    {
        m_parentVehicle->addForce(0, force, pos);
    }

    bool CVehicleBody::castLocalRay(const Ray3<physics_Num> &ray, SmartPtr<physics::IRaycastHit> &data)
    {
        WP_ASSERT(ray.isValid());
        WP_ASSERT(data);

        auto callback = m_parentVehicle->getVehicleCallback();
        if(callback)
        {
            return callback->castLocalRay(ray, data);
        }

        return false;
    }

    bool CVehicleBody::castWorldRay(const Ray3<physics_Num> &ray, SmartPtr<physics::IRaycastHit> &data)
    {
        WP_ASSERT(ray.isValid());
        WP_ASSERT(data);

        auto callback = m_parentVehicle->getVehicleCallback();
        if(callback)
        {
            return callback->castWorldRay(ray, data);
        }

        return false;
    }

    physics_Num CVehicleBody::getMass() const
    {
        WP_ASSERT(m_mass > 0);
        WP_ASSERT(m_mass < static_cast<physics_Num>( 1e10 ));
        return m_mass;
    }

    void CVehicleBody::setMass(physics_Num mass)
    {
        WP_ASSERT(mass > 0);
        WP_ASSERT(mass < static_cast<physics_Num>( 1e10 ));

        m_mass = mass;

        WP_ASSERT(m_mass > 0);
        WP_ASSERT(m_mass < static_cast<physics_Num>( 1e10 ));
    }

    Vector3<physics_Num> CVehicleBody::getLocalVelocity() const
    {
        return m_localVelocity;
    }

    void CVehicleBody::setLocalVelocity(const Vector3<physics_Num> &localVelocity)
    {
        m_localVelocity = localVelocity;
    }

    Vector3<physics_Num> CVehicleBody::getLocalAngularVelocity() const
    {
        return m_localAngularVelocity;
    }

    void CVehicleBody::setLocalAngularVelocity(const Vector3<physics_Num> &localAngularVelocity)
    {
        m_localAngularVelocity = localAngularVelocity;
    }

    void CVehicleBody::addTorque(const Vector3<physics_Num> &torque)
    {
        m_parentVehicle->addTorque(0, torque);
    }

    void CVehicleBody::addLocalTorque(const Vector3<physics_Num> &torque)
    {
        m_parentVehicle->addLocalTorque(0, torque);
    }

    SmartPtr<IVehicle> &CVehicleBody::getParentVehicle()
    {
        return m_parentVehicle;
    }

    const SmartPtr<IVehicle> &CVehicleBody::getParentVehicle() const
    {
        return m_parentVehicle;
    }

    void CVehicleBody::setParentVehicle(SmartPtr<IVehicle> parentVehicle)
    {
        m_parentVehicle = parentVehicle;
    }

    void CVehicleBody::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        auto task = Thread::getCurrentTask();
        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        m_velocity = m_parentVehicle->getLinearVelocity();
        m_angularVelocity = m_parentVehicle->getAngularVelocity();

        m_localVelocity = m_parentVehicle->getLocalLinearVelocity();
        m_localAngularVelocity = m_parentVehicle->getLocalAngularVelocity();

        WP_ASSERT(m_velocity.length() < 1e4);
        WP_ASSERT(m_angularVelocity.length() < 1e4);

        auto aircraftWorldTransform = m_parentVehicle->getWorldTransform();
        auto worldCenterOfMass = aircraftWorldTransform.transformPoint(m_parentVehicle->getCG());
        setWorldCenterOfMass(worldCenterOfMass);
    }

    Vector3<physics_Num> CVehicleBody::getPointVelocity(const Vector3<physics_Num> &p)
    {
        return m_parentVehicle->getPointVelocity(p);
    }

    bool CVehicleBody::isValid() const
    {
        return m_parentVehicle != nullptr;
    }
} // namespace workphone
