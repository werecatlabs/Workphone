#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IWorldGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{

    WP_CLASS_REGISTER_DERIVED( workphone, IWorldGenerator, IProceduralGenerator );

    IWorldGenerator::~IWorldGenerator() = default;

}  // namespace workphone::procedural
