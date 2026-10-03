#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadConnectionData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadConnectionData, ISharedObject );

    IRoadConnectionData::~IRoadConnectionData() = default;
}  // namespace workphone::procedural
