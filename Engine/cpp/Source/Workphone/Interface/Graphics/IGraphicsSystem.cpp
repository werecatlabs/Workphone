#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsSystem, ISharedObject );

    const hash_type IGraphicsSystem::FRAME_EVENT_PRE_RENDER = StringUtil::getHash( "pre_render" );
    const hash_type IGraphicsSystem::FRAME_EVENT_RENDER_QUEUED = StringUtil::getHash( "render_queued" );
    const hash_type IGraphicsSystem::FRAME_EVENT_POST_RENDER = StringUtil::getHash( "post_render" );

    IGraphicsSystem::~IGraphicsSystem() = default;
    void IGraphicsSystem::setRendererType( RenderApi )
    {
    }

    SmartPtr<IGraphicsPipeline> IGraphicsSystem::getGraphicsPipeline() const
    {
        return nullptr;
    }
}  // namespace workphone::render
