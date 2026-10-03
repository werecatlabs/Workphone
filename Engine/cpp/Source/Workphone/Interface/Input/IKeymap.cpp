#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IKeymap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IKeymap, ISharedObject );

    IKeymap::~IKeymap() = default;

}  // namespace workphone
