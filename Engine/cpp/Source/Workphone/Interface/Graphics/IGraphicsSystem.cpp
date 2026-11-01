#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsSystem, ISharedObject );

    const hash_type IGraphicsSystem::FRAME_EVENT_PRE_RENDER = StringUtil::getHash( "pre_render" );
    const hash_type IGraphicsSystem::FRAME_EVENT_RENDER_QUEUED = StringUtil::getHash( "render_queued" );
    const hash_type IGraphicsSystem::FRAME_EVENT_POST_RENDER = StringUtil::getHash( "post_render" );

    IGraphicsSystem::~IGraphicsSystem() = default;
    std::shared_ptr<resource::ResourceLeafProviderFactory> IGraphicsSystem::createRuntimeLeafProviderFactory(
        std::shared_ptr<resource::ResourcePayloadMount>,String &error)
    {error.clear();return {};}
    std::shared_ptr<const resource::RuntimeResource> IGraphicsSystem::captureRuntimeResourceSnapshot(
        std::shared_ptr<const resource::RuntimeResource> resource,std::shared_ptr<resource::ResourcePayloadMount>,String &error)
    {error.clear();return resource;}

    SmartPtr<IMaterial> IGraphicsSystem::createRuntimeMaterial(
        std::shared_ptr<const resource::RuntimeResource>,String &error,const String &,bool,std::shared_ptr<resource::ResourcePayloadMount>)
    {error="Typed runtime materials are unavailable in this graphics backend";return nullptr;}
    SmartPtr<ITexture> IGraphicsSystem::createRuntimeTexture(
        std::shared_ptr<const resource::RuntimeResource>,String &error,const String &,bool,std::shared_ptr<resource::ResourcePayloadMount>)
    {error="Typed runtime textures are unavailable in this graphics backend";return nullptr;}
    SmartPtr<IFont> IGraphicsSystem::createRuntimeFont(std::shared_ptr<const resource::RuntimeResource>,String &error,
        const String &,bool,std::shared_ptr<resource::ResourcePayloadMount>)
    {error="Typed runtime fonts are unavailable in this graphics backend";return nullptr;}

    void IGraphicsSystem::setRendererType( RenderApi )
    {
    }

    SmartPtr<IGraphicsPipeline> IGraphicsSystem::getGraphicsPipeline() const
    {
        return nullptr;
    }
}  // namespace workphone::render
