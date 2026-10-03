#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralCity.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralCity, ISharedObject );

    IProceduralCity::~IProceduralCity() = default;

}  // namespace workphone::procedural
