#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/FrameOfRef.hpp"

namespace workphone
{
    namespace vehicle
    {

        FrameOfRef::FrameOfRef() = default;

        FrameOfRef::FrameOfRef( const FrameOfRef &other )
        {
            *this = other;
        }

        FrameOfRef::~FrameOfRef() = default;

        workphone::vehicle::FrameOfRef &FrameOfRef::operator=( const FrameOfRef &other )
        {
            XAxis = other.XAxis;
            YAxis = other.YAxis;
            ZAxis = other.ZAxis;
            return *this;
        }

    } // namespace vehicle
} // namespace workphone
