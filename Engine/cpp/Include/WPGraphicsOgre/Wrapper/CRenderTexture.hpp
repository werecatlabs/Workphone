#ifndef CRenderTexture_h__
#define CRenderTexture_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <WPGraphicsOgre/Wrapper/CRenderTargetOgre.hpp>
#include <Workphone/Graphics/RenderTexture.hpp>
#include <sstream>

namespace workphone
{
    namespace render
    {

        /** Implements IRenderTexture interface for Ogre. */
        class CRenderTexture : public CRenderTargetOgre<RenderTexture>
        {
        public:
            /** Constructor. */
            CRenderTexture();

            /** Destructor. */
            ~CRenderTexture() override;

            /** @copydoc IRenderTexture::update */
            void update() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CRenderTexture_h__
