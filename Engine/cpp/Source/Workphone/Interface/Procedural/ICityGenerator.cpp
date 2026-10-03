#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICityGenerator, IProceduralGenerator );

    ICityGenerator::~ICityGenerator() = default;

}  // namespace workphone::procedural
