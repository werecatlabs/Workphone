#ifndef _COverlayElementText_H
#define _COverlayElementText_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementOgreNext.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {
        class COverlayElementText : public COverlayElementOgreNext<IOverlayElementText>
        {
        public:
            COverlayElementText();
            ~COverlayElementText() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            bool isContainer() const override;

            // IOverlayElementText functions
            void setFontName( const String &fontName ) override;

            void setCharHeight( f32 charHeight ) override;

            void setAlignment( u8 alignment );
            u8 getAlignment() const;

            void setSpaceWidth( f32 width ) override;
            f32 getSpaceWidth() const override;

            bool isValid() const override;

            Ogre::v1::TextAreaOverlayElement *getElementText() const;
            void setElementText( Ogre::v1::TextAreaOverlayElement *elementText );

            WP_CLASS_REGISTER_DECL;

        protected:
            class StateListener : public StateListenerOgre
            {
            public:
                StateListener();
                ~StateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            void createStateContext() override;

            Ogre::v1::TextAreaOverlayElement *m_elementText = nullptr;

            u8 m_horizontalAlignment = 0;
            u8 m_verticalAlignment = 0;
            ColourF m_colour = ColourF::White;

            static u32 m_extId;
        };
    }  // end namespace render
}  // namespace workphone

#endif
