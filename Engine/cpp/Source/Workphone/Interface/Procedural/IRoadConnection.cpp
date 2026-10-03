#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadConnection.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadConnection, IProceduralObject );

    IRoadConnection::~IRoadConnection() = default;
}  // namespace workphone::procedural
