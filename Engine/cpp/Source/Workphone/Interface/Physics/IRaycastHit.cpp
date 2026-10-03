#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IRaycastHit, ISharedObject );

    IRaycastHit::~IRaycastHit() = default;

}  // namespace workphone::physics
