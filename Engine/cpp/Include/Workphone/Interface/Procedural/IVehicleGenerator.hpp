#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleGeneratorTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleGenerator : public ISharedObject
    {
    public:
        ~IVehicleGenerator() override;
        virtual GeneratedVehicle generate( const VehicleGenerationConfig &config = {} ) = 0;
        virtual std::vector<VehicleGenerationIssue> validate( const GeneratedVehicle &vehicle ) = 0;
        virtual VehicleMaterialSlot materialSlotFor( VehicleMaterial material ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
