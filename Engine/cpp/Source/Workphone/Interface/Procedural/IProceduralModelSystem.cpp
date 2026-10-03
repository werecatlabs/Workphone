#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralModelSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralModelSystem, ISharedObject );

    IProceduralModelSystem::~IProceduralModelSystem() = default;
}  // namespace workphone::procedural
