#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadHitPoint.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{

    WP_CLASS_REGISTER_DERIVED( workphone, IRoadHitPoint, ISharedObject );

    IRoadHitPoint::~IRoadHitPoint() = default;

}  // namespace workphone::procedural
