#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDragSource, IEventListener );

    IUIDragSource::~IUIDragSource() = default;

}  // namespace workphone::ui
