#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadGenerator, ISharedObject );

    IRoadGenerator::~IRoadGenerator() = default;
}  // namespace workphone::procedural
