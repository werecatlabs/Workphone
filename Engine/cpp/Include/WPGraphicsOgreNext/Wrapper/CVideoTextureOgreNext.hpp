#ifndef WPOgreVideoTexture_h__
#define WPOgreVideoTexture_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Graphics/ResourceGraphics.hpp>
//#include <OgreTexture.hpp>

namespace workphone
{
    namespace render
    {

        class CVideoTextureOgreNext : public ResourceGraphics<IVideoTexture>
        {
        public:
            CVideoTextureOgreNext();
            ~CVideoTextureOgreNext() override;

            void initialise( const String &name, const Vector2I &size ) override;

            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            void _copyFrameData( void *frameData, const Vector2I &size );

            void _getObject( void **object );

            void _getObject( void **ppObject ) const override;

            Vector2I getSize() const override;
            void setSize( const Vector2I &size ) override;

            SmartPtr<IRenderTarget> getRenderTarget() const override;

            void copyToTexture( SmartPtr<ITexture> &target ) override;

            void copyData( void *data, const Vector2I &size ) override;

            Vector2I getActualSize() const override;

        protected:
            // Ogre::TexturePtr m_texture;
            Vector2I m_size = Vector2I::zero();
        };
    }  // end namespace render
}  // namespace workphone

#endif  // WPOgreVideoTexture_h__
