#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Memory/IData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IData, ISharedObject );

    IData::~IData() = default;
}  // namespace workphone
