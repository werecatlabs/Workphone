#ifndef CVehicleAi_h__
#define CVehicleAi_h__

#include <cstddef>
#include <memory>

#include <Workphone/Interface/Ai/IVehicleAi.hpp>

namespace workphone
{
    class Learning;

    class WPCore_API VehicleAi : public IVehicleAi
    {
    public:
        struct VehicleAiAction
        {
            double steeringAngle = 0.0;
            double throttle = 0.0;  // Normalized to the range [0.0, 1.0].
        };

        struct DynamicVehicleAiState
        {
            static constexpr std::size_t stateVariableCount = 7;

            double pdGameState[stateVariableCount] = {};
        };

        VehicleAi();
        ~VehicleAi() override;

        void update( f64 t, f64 dt );
        void load( SmartPtr<ISharedObject> data ) override;
        void reload( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        void reset();

        SmartPtr<vehicle::IVehicle> getVehicleController() const override;
        void setVehicleController( SmartPtr<vehicle::IVehicle> vehicleController ) override;

        SmartPtr<IVehicleAiManager> getVehicleManager() const override;
        void setVehicleManager( SmartPtr<IVehicleAiManager> vehicleManager ) override;

    protected:
        SmartPtr<vehicle::IVehicle> m_vehicleController;
        SmartPtr<IVehicleAiManager> m_vehicleManager;
        SharedPtr<Learning> m_learning;
    };
}  // namespace workphone

#endif  // CVehicleAi_h__
