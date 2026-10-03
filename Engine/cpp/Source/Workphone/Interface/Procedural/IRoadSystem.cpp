#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IRoadSystem, ISharedObject );

    IRoadSystem::~IRoadSystem() = default;
}  // namespace workphone::procedural
