#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFactory, ISharedObject );

    IFactory::~IFactory() = default;
}  // namespace workphone
