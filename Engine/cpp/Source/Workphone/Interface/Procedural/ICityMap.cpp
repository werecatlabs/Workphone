#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ICityMap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICityMap, ISharedObject );

    ICityMap::~ICityMap() = default;

}  // namespace workphone::procedural
