#ifndef CMaterialTextureOgre_h__
#define CMaterialTextureOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IMaterialTexture.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace render
    {
        class CMaterialTextureOgre : public MaterialTexture
        {
        public:
            class MaterialTextureOgreStateListener : public MaterialTextureStateListener
            {
            public:
                MaterialTextureOgreStateListener();
                ~MaterialTextureOgreStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                WP_CLASS_REGISTER_DECL;
            };

            class TextureListener : public IEventListener
            {
            public:
                TextureListener();
                ~TextureListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                CMaterialTextureOgre *getOwner() const;

                void setOwner( CMaterialTextureOgre *owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                CMaterialTextureOgre *m_owner = nullptr;
            };

            CMaterialTextureOgre();
            ~CMaterialTextureOgre() override;

            /** @copydoc IObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IMaterialTexture::initialise */
            void initialise( Ogre::TextureUnitState *textureUnitState );

            /** @copydoc IMaterialTexture::setTexture */
            void setTexture( SmartPtr<ITexture> texture ) override;

            /** @copydoc IMaterialTexture::_getObject */
            void _getObject( void **ppObject ) override;

            /** @copydoc IResource::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IResource::getTextureType */
            u32 getTextureType() const override;

            /** @copydoc IResource::setTextureType */
            void setTextureType( u32 textureType ) override;

            Ogre::TextureUnitState *getTextureUnitState() const;
            void setTextureUnitState( Ogre::TextureUnitState *textureUnitState );

            WP_CLASS_REGISTER_DECL;

        protected:
            void createTextureUnitState() override;

            SmartPtr<TextureListener> m_textureListener;

            RawPtr<Ogre::TextureUnitState> m_textureUnitState = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CTextureUnit_h__
