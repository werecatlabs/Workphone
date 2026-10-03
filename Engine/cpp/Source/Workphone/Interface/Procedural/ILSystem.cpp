#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ILSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ILSystem, ISharedObject );

    ILSystem::~ILSystem() = default;

}  // namespace workphone::procedural
