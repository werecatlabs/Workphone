#ifndef _COverlayElementText_H
#define _COverlayElementText_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementOgre.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {
        class COverlayElementText : public COverlayElementOgre<IOverlayElementText>
        {
        public:
            COverlayElementText();
            ~COverlayElementText() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            bool isContainer() const override;

            void _getObject( void **ppObject ) const override;

            // IOverlayElementText functions
            void setFontName( const String &fontName ) override;

            void setCharHeight( f32 charHeight ) override;

            void setVerticalAlignment( u8 alignment ) override;
            u8 getVerticalAlignment() const override;

            virtual void setHorizontalAlignment( u8 alignment );
            virtual u8 getHorizontalAlignment() const;

            void setSpaceWidth( f32 width ) override;
            f32 getSpaceWidth() const override;

            bool isValid() const override;

            Ogre::TextAreaOverlayElement *getElementText() const;
            void setElementText( Ogre::TextAreaOverlayElement *elementText );

            WP_CLASS_REGISTER_DECL;

        protected:
            class StateListener : public StateListenerOgre
            {
            public:
                StateListener() = default;
                ~StateListener() override = default;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            void createStateContext() override;

            Ogre::TextAreaOverlayElement *m_elementText = nullptr;

            u8 m_horizontalAlignment = 0;
            u8 m_verticalAlignment = 0;
            ColourF m_colour = ColourF::White;

            String m_name;
        };
    }  // end namespace render
}  // namespace workphone

#endif
