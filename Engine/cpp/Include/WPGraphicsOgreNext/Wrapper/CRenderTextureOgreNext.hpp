#ifndef CRenderTextureOgreNext_h__
#define CRenderTextureOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTargetOgreNext.hpp>
#include <Workphone/Graphics/RenderTexture.hpp>

namespace workphone
{
    namespace render
    {

        class CRenderTextureOgreNext : public CRenderTargetOgreNext<RenderTexture>
        {
        public:
            CRenderTextureOgreNext();
            ~CRenderTextureOgreNext();

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            void createStateObject();
        };

    }  // namespace render
}  // namespace workphone

#endif  // CRenderTextureOgreNext_h__
