#pragma once
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/VehicleGeometryTypes.hpp>

namespace workphone::procedural
{
    /// Replaceable service; inputs and CPU results belong to Workphone.
    class WPCore_API IVehicleGeometry : public ISharedObject
    {
    public:
        ~IVehicleGeometry() override;
        virtual VehicleGeometry generate( const VehicleGeometryConfig &config = {} ) = 0;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
