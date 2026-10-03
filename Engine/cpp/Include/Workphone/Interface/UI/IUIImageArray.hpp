#ifndef IGUIImageArray_h__
#define IGUIImageArray_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for an image array. */
        class WPCore_API IUIImageArray : public IUIElement
        {
        public:
            IUIImageArray();

            IUIImageArray( u32 poolTypeId );

            /** Destructor. */
            ~IUIImageArray() override;

            /** */
            virtual SmartPtr<IUIImage> getImage( u32 index ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIImageArray_h__
