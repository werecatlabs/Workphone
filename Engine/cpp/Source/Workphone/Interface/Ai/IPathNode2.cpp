#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IPathNode2.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IPathNode2, ISharedObject );

    IPathNode2::~IPathNode2() = default;

}  // namespace workphone
