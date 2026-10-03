#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IGameInputMap, ISharedObject );

    IGameInputMap::~IGameInputMap() = default;

}  // namespace workphone
