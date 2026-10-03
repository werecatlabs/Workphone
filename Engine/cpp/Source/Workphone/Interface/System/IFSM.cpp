#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFSM, ISharedObject );

    const u32 IFSM::isStateChangeCompleteFlag = ( 1 << 0 );
    const u32 IFSM::autoChangeStateFlag = ( 1 << 1 );
    const u32 IFSM::isPendingFlag = ( 1 << 2 );
    const u32 IFSM::isReadyFlag = ( 1 << 3 );
    const u32 IFSM::isLockedFlag = ( 1 << 4 );
    const u32 IFSM::allowStateChangeFlag = ( 1 << 5 );

    IFSM::~IFSM() = default;
}  // namespace workphone
