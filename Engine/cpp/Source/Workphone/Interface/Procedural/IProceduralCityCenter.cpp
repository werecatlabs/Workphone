#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralCityCenter.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralCityCenter, ISharedObject );

    IProceduralCityCenter::~IProceduralCityCenter() = default;

}  // namespace workphone::procedural
