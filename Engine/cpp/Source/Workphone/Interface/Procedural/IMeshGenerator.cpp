#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IMeshGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IMeshGenerator, IProceduralGenerator );

    IMeshGenerator::~IMeshGenerator() = default;

}  // namespace workphone::procedural
