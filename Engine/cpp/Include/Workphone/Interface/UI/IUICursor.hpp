#ifndef _IUICursor_H
#define _IUICursor_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class IUICursor
         * @brief Interface for a cursor UI element that displays a cursor in the user interface.
         */
        class WPCore_API IUICursor : public IUIElement
        {
        public:
            IUICursor();

            IUICursor( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUICursor() override;

            /** */
            virtual void setMaterialName( const String &materialName ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
