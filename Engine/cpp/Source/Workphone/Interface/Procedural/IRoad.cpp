#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoad.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoad, IProceduralObject );

    IRoad::~IRoad() = default;
}  // namespace workphone::procedural
