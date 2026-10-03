#ifndef IUICollapsingHeader_h__
#define IUICollapsingHeader_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a collapsing header. */
        class WPCore_API IUICollapsingHeader : public IUIElement
        {
        public:
            IUICollapsingHeader();

            IUICollapsingHeader( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUICollapsingHeader() override;

            /** Get the label of the header.
             * @return The label of the header.
             */
            String getLabel() const override = 0;

            /** Set the label of the header.
             * @param label The new label for the header.
             */
            void setLabel( const String &label ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUICollapsingHeader_h__
