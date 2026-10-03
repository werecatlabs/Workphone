#ifndef __IUIRenderWindow_h__
#define __IUIRenderWindow_h__

#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIRenderWindow
         * @brief Interface for a UI Render Window, extending IUIWindow and providing additional
         * functionality for rendering
         */
        class WPCore_API IUIRenderWindow : public IUIWindow
        {
        public:
            IUIRenderWindow();

            IUIRenderWindow( u32 poolTypeId );

            /**
             * @brief Virtual destructor
             */
            ~IUIRenderWindow() override;

            /**
             * @brief Gets the associated render window
             * @return A shared pointer to the render::IGraphicsWindow object
             */
            virtual SmartPtr<render::IGraphicsWindow> getWindow() const = 0;

            /**
             * @brief Sets the associated render window
             * @param window A shared pointer to the render::IGraphicsWindow object
             */
            virtual void setWindow( SmartPtr<render::IGraphicsWindow> window ) = 0;

            /**
             * @brief Gets the associated render texture
             * @return A shared pointer to the render::ITexture object
             */
            virtual SmartPtr<render::ITexture> getRenderTexture() const = 0;

            /**
             * @brief Sets the associated render texture
             * @param renderTexture A shared pointer to the render::ITexture object
             */
            virtual void setRenderTexture( SmartPtr<render::ITexture> renderTexture ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIWindow_h__
