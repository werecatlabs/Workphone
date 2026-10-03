#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ISkyAtmosphere.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, ISkyAtmosphere, ISharedObject );

    ISkyAtmosphere::~ISkyAtmosphere() = default;
}  // namespace workphone::procedural
