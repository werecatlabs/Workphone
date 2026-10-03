#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadNetwork, ISharedObject );

    IRoadNetwork::~IRoadNetwork() = default;

}  // namespace workphone::procedural
