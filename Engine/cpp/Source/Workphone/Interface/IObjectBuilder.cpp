#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IObjectBuilder.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IObjectBuilder, ISharedObject );

    IObjectBuilder::~IObjectBuilder() = default;
}  // namespace workphone
