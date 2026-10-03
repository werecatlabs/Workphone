#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadMeshElement.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{

    WP_CLASS_REGISTER_DERIVED( workphone, IRoadMeshElement, ISharedObject );

    IRoadMeshElement::~IRoadMeshElement() = default;

}  // namespace workphone::procedural
