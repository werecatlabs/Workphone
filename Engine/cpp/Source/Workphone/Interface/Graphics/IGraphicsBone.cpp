#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsBone.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsBone, ISharedObject );

    IGraphicsBone::~IGraphicsBone() = default;

}  // namespace workphone::render
