#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/RenderTexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowEvent.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/RenderTextureState.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, RenderTexture, RenderTarget<IRenderTexture> );

    RenderTexture::RenderTexture() = default;

    RenderTexture::~RenderTexture() = default;

    SmartPtr<ITexture> RenderTexture::getTexture() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateData<RenderTextureState>() )
            {
                auto p = data->texture.load();
                return p.lock();
            }
        }

        return nullptr;
    }

    void RenderTexture::setTexture( SmartPtr<ITexture> texture )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateData<RenderTextureState>() )
            {
                data->texture = texture;
            }
        }
    }

}  // namespace workphone::render
