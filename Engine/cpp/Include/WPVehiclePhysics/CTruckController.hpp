#ifndef CTruckController_h__
#define CTruckController_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <WPVehiclePhysics/CVehicleController.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CTruckController : public CVehicleController<IVehicle>
    {
    public:
        enum class CabState
        {
            Open,
            Closed,
            Opening,
            Closing,

            Count
        };

        CTruckController();
        ~CTruckController() override;

        void reset() override;

        void update() override;

        void load(SmartPtr<ISharedObject> data) override;
        void reload(SmartPtr<ISharedObject> data) override;
        void unload(SmartPtr<ISharedObject> data) override;

        f32 getChannel(s32 idx) const override;
        void setChannel(s32 idx, f32 channel) override;

        bool isElectric() const override;
        void setElectric(bool bIsElectric) override;

        bool getEnablePowerUnit() const override;
        void setEnablePowerUnit(bool enablePowerUnit) override;

        bool getEmulateBattery() const override;
        void setEmulateBattery(bool emulateBattery) override;

        physics_Num getAirDensity() const;
        void setAirDensity(physics_Num airDensity);

        SmartPtr<IBatteryPack> &getBatteryPack();
        const SmartPtr<IBatteryPack> &getBatteryPack() const;
        void setBatteryPack(SmartPtr<IBatteryPack> batteryPack);

        SmartPtr<IVehicleCallback> getCallback() const;
        void setCallback(SmartPtr<IVehicleCallback> callback);

        SmartPtr<IAerodymanicsWind> &getWind();
        const SmartPtr<IAerodymanicsWind> &getWind() const;
        void setWind(SmartPtr<IAerodymanicsWind> wind);

        void addWheel(SmartPtr<IWheelComponent> wheel);
        void removeWheel(SmartPtr<IWheelComponent> wheel);

        Array<SmartPtr<IWheelComponent>> getWheels() const;
        void setWheels(Array<SmartPtr<IWheelComponent>> wheels);

        Vector3<physics_Num> getPosition() const override;
        void setPosition(const Vector3<physics_Num> &position) override;

        bool isUserControlled() const override;
        void setUserControlled(bool userControlled) override;

        physics_Num getMass() const override;
        void setMass(physics_Num mass) override;

        SmartPtr<IVehicleBody> getBody() const override;

        void setBody(SmartPtr<IVehicleBody> body) override;

        Transform3<physics_Num> getWorldTransform() const override;
        void setWorldTransform(const Transform3<physics_Num> &worldTransform) override;

        Transform3<physics_Num> getLocalTransform() const override;
        void setLocalTransform(const Transform3<physics_Num> &localTransform) override;

        void displayLocalVector(s32 bodyId, const Vector3<physics_Num> &start,
                                const Vector3<physics_Num> &end, u32 colour) override;
        void displayVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                           const Vector3<physics_Num> &end, u32 colour) override;
        void displayLocalVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                                const Vector3<physics_Num> &end, u32 colour) override;

        bool getDisplayDebugData() const override;
        void setDisplayDebugData(bool displayDebugData) override;

        void openCab();
        void closeCab();

        void hoiseDown();
        void hoistUp();

        void drawPoint(s32 body, s32 id, const Vector3<physics_Num> &positon, u32 color) override;
        void drawLocalPoint(s32 body, s32 id, const Vector3<physics_Num> &positon,
                            u32 color) override;

        void addForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                      const Vector3<physics_Num> &Loc) override;

        void addTorque(s32 bodyIdx, const Vector3<physics_Num> &Torque) override;

        void addLocalForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                           const Vector3<physics_Num> &Loc) override;

        void addLocalTorque(s32 bodyIdx, const Vector3<physics_Num> &Torque) override;

        Vector3<physics_Num> getPointVelocity(const Vector3<physics_Num> &p) override;

        Vector3<physics_Num> getAngularVelocity() override;

        Vector3<physics_Num> getLinearVelocity() override;

        Vector3<physics_Num> getLocalAngularVelocity() override;

        Vector3<physics_Num> getLocalLinearVelocity() override;

        Vector3<physics_Num> getCG() const override;

        SmartPtr<IWheelComponent> getWheelController(u32 index) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // smart_ptr<CarDriveTrain> m_carDriveTrain;
        // RigidbodyPtr m_modelRigidBody;
        // RawPtr<physx::PxMaterial> m_material;

        // Array<smart_ptr<WheelControllerPacejka>> m_wheelsPacejka;
        Array<String> m_wheelNames;

        FixedArray<WheelControllerArcade, 4> m_wheelControllers;

        Array<SmartPtr<IWheelComponent>> m_wheels;
        SmartPtr<IBatteryPack> m_pack;
        Transform3<physics_Num> m_worldTransform;
        Transform3<physics_Num> m_localTransform;
        AtomicSmartPtr<IVehicleBody> m_body;
        SmartPtr<IAerodymanicsWind> m_wind;
        physics_Num m_cabAngle;
        physics_Num m_trayAngle;
        s32 m_cabState;
        bool m_isElectric;
        bool m_enablePowerUnit;
        bool m_emulateBattery;
    };
}

#endif // CTruckController_h__
