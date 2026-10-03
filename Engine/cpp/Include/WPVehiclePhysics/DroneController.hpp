#ifndef DroneController_h__
#define DroneController_h__

#include "Workphone/Interface/Memory/ISharedObject.hpp"

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API DroneController : public ISharedObject
        {
        public:
            /*
            DroneController();
            ~DroneController();

            void update(const int& task, const double& t, const double& dt);

            RawPtr<Model> getModel() const;
            void setModel(RawPtr<Model> model);

            RawPtr<MultiRotorCtrl> getController() const;
            void setController(RawPtr<MultiRotorCtrl> controller);

            RawPtr<data::drone_stab_settings> getStabSettings() const;
            void setStabSettings(RawPtr<data::drone_stab_settings> stabSettings);

            RawPtr<data::stab_settings_data> getSettingsData() const;

            RigidbodyPtr getModelBody() const;
            void setModelBody(RigidbodyPtr modelBody);

            virtual void load(SmartPtr<ISharedObject> data) override;

            virtual void readSettings();

            void setPitchRate(f32 rate);
            void setRollRate(f32 rate);
            void setYawRate(f32 rate);

            void setPitchRateP(f32 pitchRateP);
            void setPitchRateI(f32 pitchRateI);
            void setPitchRateD(f32 pitchRateD);

            void setRollRateP(f32 rollRateP);
            void setRollRateI(f32 rollRateI);
            void setRollRateD(f32 rollRateD);

            void setYawRateP(f32 yawRateP);
            void setYawRateI(f32 yawRateI);
            void setYawRateD(f32 yawRateD);

            void setPitchExpo(f32 pitchExpo);
            void setRollExpo(f32 rollExpo);
            void setYawExpo(f32 yawExpo);
            void setThrottleExpo(f32 throttleExpo);

            virtual bool isAirmode() const { return false; }
            virtual void setAirmode(bool airmode) {}

        protected:
            RigidbodyPtr m_modelBody;
            RawPtr<Model> m_model;
            RawPtr<MultiRotorCtrl> m_controller;
            RawPtr<data::drone_stab_settings> m_stabSettings;
            */
        };
    } // namespace vehicle
} // namespace workphone

#endif // DroneController_h__
