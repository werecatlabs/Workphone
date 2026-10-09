#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CTruckController.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <WPVehiclePhysics/CVehicleBody.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, CTruckController, CVehicleController<IVehicle>);

    using CWheelControllerDefault = WheelControllerPacejka;

    CTruckController::CTruckController() :
        m_cabAngle(0.0),
        m_trayAngle(0.0),
        m_cabState(static_cast<s32>(CabState::Closed)),
        m_isElectric(false),
        m_enablePowerUnit(true),
        m_emulateBattery(false)
    {
    }

    CTruckController::~CTruckController()
    {
        unload(nullptr);
    }

    f32 CTruckController::getChannel(s32 idx) const
    {
        return CVehicleController<IVehicle>::getChannel(idx);
    }

    void CTruckController::setChannel(s32 idx, f32 channel)
    {
        CVehicleController<IVehicle>::setChannel(idx, channel);
    }

    bool CTruckController::isElectric() const
    {
        return m_isElectric;
    }

    void CTruckController::setElectric(bool bIsElectric)
    {
        m_isElectric = bIsElectric;
    }

    bool CTruckController::getEnablePowerUnit() const
    {
        return m_enablePowerUnit;
    }

    void CTruckController::setEnablePowerUnit(bool enablePowerUnit)
    {
        m_enablePowerUnit = enablePowerUnit;
    }

    bool CTruckController::getEmulateBattery() const
    {
        return m_emulateBattery;
    }

    void CTruckController::setEmulateBattery(bool emulateBattery)
    {
        m_emulateBattery = emulateBattery;
    }

    physics_Num CTruckController::getAirDensity() const
    {
        if(m_wind)
        {
            return m_wind->getAirDensity();
        }

        return 1.225;
    }

    void CTruckController::setAirDensity(physics_Num airDensity)
    {
        if(m_wind)
        {
            m_wind->setAirDensity(airDensity);
        }
    }

    SmartPtr<IBatteryPack> &CTruckController::getBatteryPack()
    {
        return m_pack;
    }

    const SmartPtr<IBatteryPack> &CTruckController::getBatteryPack() const
    {
        return m_pack;
    }

    void CTruckController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_pack = batteryPack;
    }

    SmartPtr<IVehicleCallback> CTruckController::getCallback() const
    {
        return getVehicleCallback();
    }

    void CTruckController::setCallback(SmartPtr<IVehicleCallback> callback)
    {
        setVehicleCallback(callback);
    }

    SmartPtr<IAerodymanicsWind> &CTruckController::getWind()
    {
        return m_wind;
    }

    const SmartPtr<IAerodymanicsWind> &CTruckController::getWind() const
    {
        return m_wind;
    }

    void CTruckController::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    void CTruckController::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel && std::find(m_wheels.begin(), m_wheels.end(), wheel) == m_wheels.end())
        {
            m_wheels.push_back(wheel);

            if(isLoaded())
            {
                wheel->setOwner(getSharedFromThis<CTruckController>());
                if(!wheel->isLoaded())
                {
                    wheel->load(nullptr);
                }
            }
        }
    }

    void CTruckController::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel)
        {
            auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
            if(it != m_wheels.end())
            {
                (*it)->setOwner(nullptr);
                m_wheels.erase(it);
            }
        }
    }

    Array<SmartPtr<IWheelComponent>> CTruckController::getWheels() const
    {
        return m_wheels;
    }

    void CTruckController::setWheels(Array<SmartPtr<IWheelComponent>> wheels)
    {
        for(auto &wheel : m_wheels)
        {
            if(wheel)
            {
                wheel->setOwner(nullptr);
            }
        }

        m_wheels.clear();
        for(auto &wheel : wheels)
        {
            addWheel(wheel);
        }
    }

    Vector3<physics_Num> CTruckController::getPosition() const
    {
        return CVehicleController<IVehicle>::getPosition();
    }

    void CTruckController::setPosition(const Vector3<physics_Num> &position)
    {
        CVehicleController<IVehicle>::setPosition(position);
    }

    bool CTruckController::isUserControlled() const
    {
        return CVehicleController<IVehicle>::isUserControlled();
    }

    void CTruckController::setUserControlled(bool userControlled)
    {
        CVehicleController<IVehicle>::setUserControlled(userControlled);
    }

    physics_Num CTruckController::getMass() const
    {
        return CVehicleController<IVehicle>::getMass();
    }

    void CTruckController::setMass(physics_Num mass)
    {
        CVehicleController<IVehicle>::setMass(mass);
    }

    SmartPtr<IVehicleBody> CTruckController::getBody() const
    {
        return m_body;
    }

    void CTruckController::setBody(SmartPtr<IVehicleBody> body)
    {
        m_body = body;
    }

    Transform3<physics_Num> CTruckController::getWorldTransform() const
    {
        return CVehicleController<IVehicle>::getWorldTransform();
    }

    void CTruckController::setWorldTransform(const Transform3<physics_Num> &worldTransform)
    {
        CVehicleController<IVehicle>::setWorldTransform(worldTransform);
    }

    Transform3<physics_Num> CTruckController::getLocalTransform() const
    {
        return CVehicleController<IVehicle>::getLocalTransform();
    }

    void CTruckController::setLocalTransform(const Transform3<physics_Num> &localTransform)
    {
        CVehicleController<IVehicle>::setLocalTransform(localTransform);
    }

    void CTruckController::displayLocalVector(s32 bodyId, const Vector3<physics_Num> &start,
                                              const Vector3<physics_Num> &end, u32 colour)
    {
        CVehicleController<IVehicle>::displayLocalVector(bodyId, start, end, colour);
    }

    void CTruckController::displayLocalVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                                              const Vector3<physics_Num> &end, u32 colour)
    {
        CVehicleController<IVehicle>::displayLocalVector(bodyId, id, start, end, colour);
    }

    bool CTruckController::getDisplayDebugData() const
    {
        return CVehicleController<IVehicle>::getDisplayDebugData();
    }

    void CTruckController::setDisplayDebugData(bool displayDebugData)
    {
        CVehicleController<IVehicle>::setDisplayDebugData(displayDebugData);
    }

    void CTruckController::openCab()
    {
        if(m_cabState == static_cast<s32>(CabState::Closed))
        {
            m_cabState = static_cast<s32>(CabState::Opening);
        }
    }

    void CTruckController::closeCab()
    {
        if(m_cabState == static_cast<s32>(CabState::Open))
        {
            m_cabState = static_cast<s32>(CabState::Closing);
        }
    }

    void CTruckController::hoiseDown()
    {
        // Note: Method name appears to be a typo for "hoistDown"
        // Implement tray lowering logic here
        m_trayAngle = Math<physics_Num>::clamp(m_trayAngle - static_cast<physics_Num>(0.01),
                                               0.0,
                                               1.0);
    }

    void CTruckController::hoistUp()
    {
        m_trayAngle = Math<physics_Num>::clamp(m_trayAngle + static_cast<physics_Num>(0.01),
                                               0.0,
                                               1.0);
    }

    void CTruckController::drawPoint(s32 body, s32 id, const Vector3<physics_Num> &positon,
                                     u32 color)
    {
        CVehicleController<IVehicle>::drawPoint(body, id, positon, color);
    }

    void CTruckController::drawLocalPoint(s32 body, s32 id, const Vector3<physics_Num> &positon,
                                          u32 color)
    {
        CVehicleController<IVehicle>::drawLocalPoint(body, id, positon, color);
    }

    void CTruckController::addForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                                    const Vector3<physics_Num> &Loc)
    {
        CVehicleController<IVehicle>::addForce(bodyIdx, Force, Loc);
    }

    void CTruckController::addTorque(s32 bodyIdx, const Vector3<physics_Num> &Torque)
    {
        CVehicleController<IVehicle>::addTorque(bodyIdx, Torque);
    }

    void CTruckController::addLocalForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                                         const Vector3<physics_Num> &Loc)
    {
        CVehicleController<IVehicle>::addLocalForce(bodyIdx, Force, Loc);
    }

    void CTruckController::addLocalTorque(s32 bodyIdx, const Vector3<physics_Num> &torque)
    {
        CVehicleController<IVehicle>::addLocalTorque(bodyIdx, torque);
    }

    Vector3<physics_Num> CTruckController::getPointVelocity(const Vector3<physics_Num> &p)
    {
        return CVehicleController<IVehicle>::getPointVelocity(p);
    }

    Vector3<physics_Num> CTruckController::getAngularVelocity()
    {
        return CVehicleController<IVehicle>::getAngularVelocity();
    }

    Vector3<physics_Num> CTruckController::getLinearVelocity()
    {
        return CVehicleController<IVehicle>::getLinearVelocity();
    }

    Vector3<physics_Num> CTruckController::getLocalAngularVelocity()
    {
        return CVehicleController<IVehicle>::getLocalAngularVelocity();
    }

    Vector3<physics_Num> CTruckController::getLocalLinearVelocity()
    {
        return CVehicleController<IVehicle>::getLocalLinearVelocity();
    }

    Vector3<physics_Num> CTruckController::getCG() const
    {
        return CVehicleController<IVehicle>::getCG();
    }

    SmartPtr<IWheelComponent> CTruckController::getWheelController(u32 index) const
    {
        if(index < m_wheels.size())
        {
            return m_wheels[index];
        }

        return nullptr;
    }

    void CTruckController::update()
    {
        CVehicleController<IVehicle>::update();

        constexpr physics_Num CabStep = 0.01;
        switch(static_cast<CabState>(m_cabState))
        {
        case CabState::Opening:
            m_cabAngle =
                Math<physics_Num>::clamp(m_cabAngle + CabStep, 0.0,
                                         1.0);
            if(m_cabAngle >= static_cast<physics_Num>(1.0))
            {
                m_cabState = static_cast<s32>(CabState::Open);
            }
            break;
        case CabState::Closing:
            m_cabAngle =
                Math<physics_Num>::clamp(m_cabAngle - CabStep, 0.0,
                                         1.0);
            if(m_cabAngle <= static_cast<physics_Num>(0.0))
            {
                m_cabState = static_cast<s32>(CabState::Closed);
            }
            break;
        case CabState::Open:
        case CabState::Closed:
        case CabState::Count:
            break;
        }

        for(auto &wheel : m_wheels)
        {
            if(wheel)
            {
                wheel->update();
            }
        }
    }

    void CTruckController::reset()
    {
        CVehicleController<IVehicle>::reset();

        for(auto &w : m_wheels)
        {
            if(w)
            {
                w->reset();
            }
        }

        m_cabAngle = static_cast<physics_Num>(0.0);
        m_trayAngle = static_cast<physics_Num>(0.0);
        m_cabState = static_cast<s32>(CabState::Closed);
    }

    void CTruckController::load(SmartPtr<ISharedObject> data)
    {
        try
        {
            setLoadingState(LoadingState::Loading);

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT(applicationManager);

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT(factoryManager);

            auto pThis = getSharedFromThis<CTruckController>();

            auto body = workphone::make_ptr<CVehicleBody>();
            body->setParentVehicle(pThis);
            setBody(body);

            auto worldTransform = Transform3<physics_Num>();
            setWorldTransform(worldTransform);

            auto localTransform = Transform3<physics_Num>();
            setLocalTransform(localTransform);

            for(auto &wheel : m_wheels)
            {
                if(wheel)
                {
                    wheel->setOwner(pThis);
                    if(!wheel->isLoaded())
                    {
                        wheel->load(data);
                    }
                }
            }

            setLoadingState(LoadingState::Loaded);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void CTruckController::reload(SmartPtr<ISharedObject> data)
    {
        for(auto &wheel : m_wheels)
        {
            if(wheel)
            {
                wheel->reload(data);
            }
        }
    }

    void CTruckController::unload(SmartPtr<ISharedObject> data)
    {
        try
        {
            auto loadingState = getLoadingState();
            if(loadingState != LoadingState::Unloaded)
            {
                setLoadingState(LoadingState::Unloading);

                for(auto &w : m_wheels)
                {
                    if(w)
                    {
                        w->setOwner(nullptr);
                        w->unload(nullptr);
                    }
                }

                m_wheels.clear();

                if(auto body = getBody())
                {
                    auto vehicleBody = dynamic_pointer_cast<CVehicleBody>(body);
                    if(vehicleBody)
                    {
                        vehicleBody->setParentVehicle(nullptr);
                    }
                    body->unload(nullptr);
                    setBody(nullptr);
                }

                m_pack = nullptr;
                setVehicleCallback(nullptr);
                m_wind = nullptr;

                setLoadingState(LoadingState::Unloaded);
            }
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void CTruckController::displayVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                                         const Vector3<physics_Num> &end, u32 colour)
    {
        CVehicleController<IVehicle>::displayVector(bodyId, id, start, end, colour);
    }
}
