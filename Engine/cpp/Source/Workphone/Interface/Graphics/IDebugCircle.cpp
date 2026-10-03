#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDebugCircle.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDebugCircle, ISharedObject );

    IDebugCircle::~IDebugCircle() = default;

}  // namespace workphone::render
