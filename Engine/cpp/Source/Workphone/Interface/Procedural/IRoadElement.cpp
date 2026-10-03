#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadElement.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadElement, ISharedObject );

    IRoadElement::~IRoadElement() = default;
}  // namespace workphone::procedural
