#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderer.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CRenderer, IRenderer );

    CRenderer::CRenderer()
    {
    }

    CRenderer::~CRenderer()
    {
    }

    void CRenderer::load( SmartPtr<ISharedObject> data )
    {
    }

    void CRenderer::unload( SmartPtr<ISharedObject> data )
    {
    }

    void CRenderer::beginRender()
    {
    }

    void CRenderer::endRender()
    {
    }

    void CRenderer::flush()
    {
    }

    void CRenderer::clear( const ColourF &colour )
    {
    }

    void CRenderer::setRenderTarget( SmartPtr<IRenderTarget> renderTarget )
    {
    }

    SmartPtr<IRenderTarget> CRenderer::getRenderTarget() const
    {
        return nullptr;
    }

    void CRenderer::setViewport( SmartPtr<IViewport> viewport )
    {
    }

    SmartPtr<IViewport> CRenderer::getViewport() const
    {
        return nullptr;
    }

    void CRenderer::render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<ITexture> &texture,
                            const Matrix4F &transform, const ColourF &colour )
    {
    }

    void CRenderer::render( const SmartPtr<ISharedObject> &renderData,
                            const SmartPtr<IMaterial> &material, const Matrix4F &transform,
                            const ColourF &colour )
    {
    }

    void CRenderer::_getObject( void **ppObject )
    {
    }

    void CRenderer::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            graphicsSystem->unlock();
        }
    }

    bool CRenderer::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            return graphicsSystem->try_lock();
        }

        return false;
    }

    void CRenderer::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            graphicsSystem->lock();
        }
    }

    void CRenderer::setCamera( SmartPtr<IGraphicsCamera> camera )
    {
    }

    SmartPtr<IGraphicsCamera> CRenderer::getCamera() const
    {
        return nullptr;
    }

}  // namespace workphone::render
