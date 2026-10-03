#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CRenderTexture.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        CRenderTexture::CRenderTexture()
        {
            setupStateObject();
        }

        CRenderTexture::~CRenderTexture()
        {
        }

        void CRenderTexture::update()
        {
            auto size = getSize();
            if( size.x > 0 && size.y > 0 )
            {
                CRenderTargetOgre<RenderTexture>::update();
            }
        }

        void CRenderTexture::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );
                CRenderTargetOgre<RenderTexture>::load( data );
                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CRenderTexture::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState == LoadingState::Loaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    CRenderTargetOgre<RenderTexture>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

    }  // namespace render
}  // namespace workphone
