#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/IVehicleGeometry.hpp>

namespace workphone::procedural
{
    class WPProcedural_API CVehicleGeometry : public IVehicleGeometry, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;

        VehicleGeometry generate( const VehicleGeometryConfig &config ) override;
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone::procedural
