#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxErrorOutput.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    PhysxErrorOutput::PhysxErrorOutput() = default;

    PhysxErrorOutput::~PhysxErrorOutput() = default;

    void PhysxErrorOutput::reportError( physx::PxErrorCode::Enum e, const char *message,
                                        const char *file, int line )
    {
        auto messageStr = String( "Error: " ) + String( message ) + String( " File: " ) +
                          String( file ) + String( " Line: " ) + StringUtil::toString( line );
        WP_LOG_ERROR( messageStr );
    }
} // namespace workphone::physics
