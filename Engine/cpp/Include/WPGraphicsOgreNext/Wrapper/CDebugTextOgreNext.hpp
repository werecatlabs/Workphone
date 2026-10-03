#ifndef CDebugText_h__
#define CDebugText_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IDebugText.hpp>
#include <OgreMaterial.h>

namespace workphone
{
    namespace render
    {

        /** Debug text class for rendering debug information in the scene. */
        class CDebugTextOgreNext : public IDebugText
        {
        public:
            CDebugTextOgreNext();
            ~CDebugTextOgreNext() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            String getText() const override;
            void setText( const String &text ) override;

            SmartPtr<IOverlayElementText> getTextElement() const override;
            void setTextElement( SmartPtr<IOverlayElementText> textElement ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_text;
            SmartPtr<IOverlayElementText> m_textElement;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CDebugText_h__
