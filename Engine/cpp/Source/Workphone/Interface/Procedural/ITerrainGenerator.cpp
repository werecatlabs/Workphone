#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ITerrainGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{

    WP_CLASS_REGISTER_DERIVED( workphone, ITerrainGenerator, IProceduralGenerator );

    ITerrainGenerator::~ITerrainGenerator() = default;

}  // namespace workphone::procedural
