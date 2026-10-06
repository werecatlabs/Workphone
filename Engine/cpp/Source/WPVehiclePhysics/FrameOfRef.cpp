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
            m_xAxis = other.m_xAxis;
            m_yAxis = other.m_yAxis;
            m_zAxis = other.m_zAxis;
            return *this;
        }

    } // namespace vehicle
} // namespace workphone
