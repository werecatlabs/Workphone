#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsPipeline, ISharedObject );

    IGraphicsPipeline::~IGraphicsPipeline() = default;

    IGraphicsPipeline::IGraphicsPipeline() = default;

}  // namespace workphone::render
