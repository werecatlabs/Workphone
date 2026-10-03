#ifndef IDebugText_h__
#define IDebugText_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /** Interface for a debug text. */
        class WPCore_API IDebugText : public ISharedObject
        {
        public:
            /** Destructor. */
            ~IDebugText() override;

            /** Returns the text string.
             * @return The text string.
             */
            virtual String getText() const = 0;

            /** Sets the text string.
             * @param text The text string.
             */
            virtual void setText( const String &text ) = 0;

            /** Returns the underlying overlay text element.
             * @return The underlying overlay text element.
             */
            virtual SmartPtr<IOverlayElementText> getTextElement() const = 0;

            /** Sets the underlying overlay text element.
             * @param textElement The overlay text element to set.
             */
            virtual void setTextElement( SmartPtr<IOverlayElementText> textElement ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IDebugText_h__
