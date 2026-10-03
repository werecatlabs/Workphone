#ifndef IUIProfileWindow_h__
#define IUIProfileWindow_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIProfileWindow
         * @brief Interface for a UI Profile Window, extending IUIElement and providing functionality for
         * managing a single profile
         */
        class WPCore_API IUIProfileWindow : public IUIElement
        {
        public:
            IUIProfileWindow() : IUIElement( IUIProfileWindow::typeInfo() )
            {
            }

            IUIProfileWindow( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor
             */
            ~IUIProfileWindow() override;

            /**
             * @brief Gets the IProfile associated with the UI Profile Window
             * @return A shared pointer to the IProfile object
             */
            virtual SmartPtr<IProfile> getProfile() const = 0;

            /**
             * @brief Sets the IProfile associated with the UI Profile Window
             * @param profile A shared pointer to the IProfile object to be associated with the window
             */
            virtual void setProfile( SmartPtr<IProfile> profile ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIProfileWindow_h__
