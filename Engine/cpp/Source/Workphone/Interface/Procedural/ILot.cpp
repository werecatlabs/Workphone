#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ILot.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ILot, ISharedObject );

    ILot::~ILot() = default;

}  // namespace workphone::procedural
