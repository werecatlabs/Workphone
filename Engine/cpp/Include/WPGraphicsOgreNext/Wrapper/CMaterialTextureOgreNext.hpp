#ifndef CMaterialTextureOgreNext_h__
#define CMaterialTextureOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace render
    {
        class CMaterialTextureOgreNext : public MaterialTexture
        {
        public:
            class TextureListener : public IEventListener
            {
            public:
                TextureListener();
                ~TextureListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                SmartPtr<CMaterialTextureOgreNext> getOwner() const;

                void setOwner( SmartPtr<CMaterialTextureOgreNext> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                AtomicWeakPtr<CMaterialTextureOgreNext> m_owner;
            };

            /** Constructor. */
            CMaterialTextureOgreNext();

            /** Destructor. */
            ~CMaterialTextureOgreNext() override;

            /** @copydoc MaterialTexture::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc MaterialTexture::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc MaterialTexture::setTexture */
            void setTexture( SmartPtr<ITexture> texture ) override;

            /** @copydoc MaterialTexture::setScale */
            void setScale( const Vector3F &scale ) override;

            /** @copydoc MaterialTexture::getAnimator */
            SmartPtr<IAnimator> getAnimator() const override;

            /** @copydoc MaterialTexture::setAnimator */
            void setAnimator( SmartPtr<IAnimator> animator ) override;

            /** @copydoc MaterialTexture::_getObject */
            void _getObject( void **ppObject ) override;

            /** @copydoc MaterialTexture::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc MaterialTexture::getTextureType */
            u32 getTextureType() const override;

            /** @copydoc MaterialTexture::setTextureType */
            void setTextureType( u32 textureType ) override;

            Ogre::TextureUnitState *getTextureUnitState() const;
            void setTextureUnitState( Ogre::TextureUnitState *textureUnitState );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Loads an image into a texture. */
            void loadImage( Ogre::TextureGpu *texture, const String &filePath );

            SmartPtr<TextureListener> m_textureListener;
            Ogre::TextureUnitState *m_textureUnitState = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTextureUnit_h__
