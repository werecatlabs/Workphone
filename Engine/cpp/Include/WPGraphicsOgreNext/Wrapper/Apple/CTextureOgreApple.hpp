#ifndef _CTextureOgreApple_H
#define _CTextureOgreApple_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>

namespace workphone
{
    namespace render
    {

        class CTextureOgreApple : public CTextureOgreNext
        {
        public:
            void getTextureFinal(void **ppTexture) const;
        };

    } // end namespace render
}     // end namespace fb

#endif
