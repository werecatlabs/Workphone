#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProceduralObject, ISharedObject );

    IProceduralObject::~IProceduralObject() = default;
}  // namespace workphone::procedural
